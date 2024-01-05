//
//  ipsw.cpp
//  iLogoExtractor
//
//  Created by Karson Eskind on 8/11/23.
//

#include <zip.h>
#include <vector>
#include <string.h>
#include <plist/plist.h>
#include "utilities.hpp"
#include "ipsw.hpp"
#include "api.hpp"

using namespace std;

ile_error_t ipsw_open(ipsw_archive_t* archive) {
    archive->data = zip_open(archive->path, ZIP_RDONLY, 0);
    if (!archive->data) {
        return ILE_E_FAILED_TO_OPEN_IPSW;
    }
    
    return ILE_SUCCESS;
}

void ipsw_close(ipsw_archive_t* archive) {
    zip_close(archive->data);
}

ile_error_t extract_ipsw_file_to_memory(ipsw_archive_t archive, const char* filename, char** buffer, size_t* size) {
    /* Locate the file */
    size_t zip_index = zip_name_locate(archive.data, filename, 0);
    if (zip_index < 0) {
        return ILE_E_FAILED_TO_GET_ZIP_INDEX;
    }
    
    /* Get statistics */
    struct zip_stat zstat;
    zip_stat_init(&zstat);
    if (zip_stat_index(archive.data, zip_index, 0, &zstat) != 0) {
        return ILE_E_FAILED_TO_HANDLE_ZIP;
    }
    
    /* Open the index and get it's size */
    struct zip_file* zfile = zip_fopen_index(archive.data, zip_index, 0);
    if (!zfile) {
        return ILE_E_FAILED_TO_HANDLE_ZIP;
    }
    *size = zstat.size;
    
    /* Allocate memory and read into the buffer */
    *buffer = (char*)malloc((*size + 1));
    if (!*buffer) {
        zip_fclose(zfile);
        return ILE_E_OUT_OF_MEMORY;
    }
    if (zip_fread(zfile, *buffer, *size) != *size) {
        zip_fclose(zfile);
        free(*buffer);
        return ILE_E_FAILED_TO_HANDLE_ZIP;
    }
    
    /* Null terminate, cleanup, and return */
    (*buffer)[*size] = '\0';
    zip_fclose(zfile);
    return ILE_SUCCESS;
}

ile_error_t parse_build_manifest(ipsw_archive_t archive, build_manifest_t* build_manifest) {
    /* Load the BuildManifest into memory */
    char* buffer = NULL;
    size_t size  = 0;
    ile_error_t ret = extract_ipsw_file_to_memory(archive, "BuildManifest.plist", &buffer, &size);
    if (ret != ILE_SUCCESS) {
        /* Older models say BuildManifesto instead of BuildManifest - Try again */
        ret = extract_ipsw_file_to_memory(archive, "BuildManifesto.plist", &buffer, &size);
        if (ret != ILE_SUCCESS) {
            return ret;
        }
    }
    
    /* Convert it into a plist from an XML doc */
    plist_t root_node = NULL;
    if (plist_from_xml(buffer, (uint32_t)size, &root_node) != PLIST_ERR_SUCCESS) {
        free(buffer);
        return ILE_E_PLIST_CONVERSION_FAILED;
    }
    free(buffer);
    
    /* Getting Basic Info - product_build_version */
    plist_t product_build_version = plist_dict_get_item(root_node, "ProductBuildVersion");
    if (!product_build_version) {
        plist_free(root_node);
        return ILE_E_PLIST_OBJECT_NOT_FOUND;
    } else {
        /* Get the string value */
        plist_get_string_val(product_build_version, &build_manifest->product_build_version);
        if (!build_manifest->product_build_version) {
            plist_free(root_node);
            return ILE_E_FAILED_TO_GET_PLIST_STR_VAL;
        }
    }
    
    /* Getting Basic Info - product_type */
    plist_t supported_product_types = plist_dict_get_item(root_node, "SupportedProductTypes");
    if (!supported_product_types) {
        plist_free(root_node);
        return ILE_E_PLIST_OBJECT_NOT_FOUND;
    } else {
        /* For any arrays where we need info, we'll always just get the first index */
        plist_t product_type = plist_array_get_item(supported_product_types, 0);
        if (!product_type) {
            plist_free(root_node);
            return ILE_E_PLIST_OBJECT_NOT_FOUND;
        } else {
            plist_get_string_val(product_type, &build_manifest->product_type);
            if (!build_manifest->product_type) {
                plist_free(root_node);
                return ILE_E_FAILED_TO_GET_PLIST_STR_VAL;
            }
        }
    }
    
    /* Getting Basic Info - Device Class */
    plist_t build_identities = plist_dict_get_item(root_node, "BuildIdentities");
    plist_t build_identity = NULL;
    if (!build_identities) {
        plist_free(root_node);
        return ILE_E_PLIST_OBJECT_NOT_FOUND;
    } else {
        build_identity = plist_array_get_item(build_identities, 0);
        if (!build_identity) {
            plist_free(root_node);
            return ILE_E_PLIST_OBJECT_NOT_FOUND;
        } else {
            plist_t info = plist_dict_get_item(build_identity, "Info");
            if (!info) {
                plist_free(root_node);
                return ILE_E_PLIST_OBJECT_NOT_FOUND;
            } else {
                plist_t device_class = plist_dict_get_item(info, "DeviceClass");
                if (!device_class) {
                    plist_free (root_node);
                    return ILE_E_PLIST_OBJECT_NOT_FOUND;
                } else {
                    plist_get_string_val(device_class, &build_manifest->device_class);
                    if (!build_manifest->device_class) {
                        plist_free(root_node);
                        return ILE_E_FAILED_TO_GET_PLIST_STR_VAL;
                    }
                }
            }
        }
    }
    
    /* Prepare to iterate */
    plist_t manifest = plist_dict_get_item(build_identity, "Manifest");
    if (!manifest) {
        plist_free(root_node);
        return ILE_E_PLIST_OBJECT_NOT_FOUND;
    } else {
        /* Create an iterator */
        plist_dict_iter manifest_iter = NULL;
        plist_dict_new_iter(manifest, &manifest_iter);
        if (!manifest_iter) {
            return ILE_E_FAILED_TO_ITERATE_OVER_PLIST;
        } else {
            /* We'll use this size for the for loop itself, while inside the for loop is where we'll incriment the file_count in the ret build_manifest */
            const uint32_t MANIFEST_SIZE = plist_dict_get_size(manifest);
            
            /* Iterate */
            for (uint32_t i = 0; i < MANIFEST_SIZE; i++) {
                /* Get the next item */
                char* manifest_component_name = NULL;
                plist_t manifest_component = NULL;
                plist_dict_next_item(manifest, manifest_iter, &manifest_component_name, &manifest_component);
                
                if (!manifest_component_name || plist_get_node_type(manifest_component) != PLIST_DICT) {
                    free(manifest_iter);
                    plist_free(root_node);
                    return ILE_E_PLIST_OBJECT_NOT_FOUND;
                } else {
                    /* Go into the info dictionary */
                    plist_t info = plist_dict_get_item(manifest_component, "Info");
                    if (!info) {
                        /* Skip this item */
                        continue;
                    } else {
                        /* Get the path value */
                        plist_t path = plist_dict_get_item(info, "Path");
                        if (!path) {
                            /* Skip this item */
                            continue;
                        } else {
                            char* path_string_value = NULL;
                            plist_get_string_val(path, &path_string_value);
                            if (!path_string_value) {
                                /* Skip this item */
                                continue;
                            } else {
                                /* Check if it starts with Firmware/all_flash/, only these paths are allowed */
                                char* p = strstr(path_string_value, ALL_FLASH_PATH);
                                if (!p) {
                                    /* Skip */
                                    continue;
                                } else if (p && !is_duplicate_vector_key(build_manifest->paths, path_string_value) && !is_duplicate_vector_key(build_manifest->manifest_component_names, manifest_component_name)) {
                                    /* We have a good path, now we can update the ret build_manifest */
                                    build_manifest->paths.push_back(path_string_value);
                                    build_manifest->manifest_component_names.push_back(manifest_component_name_fixup(manifest_component_name));
                                    build_manifest->file_count++;
                                }
                            }
                        }
                    }
                }
            }
        }
        /* Cleanup */
        free(manifest_iter);
        plist_free(root_node);
    }
    
    /* Attempt to append keys */
    ret = append_keys_to_build_manifest(build_manifest);
    if (ret != ILE_SUCCESS) {
        return ret;
    }
    
    return ILE_SUCCESS;
}

ile_error_t write_report(ipsw_archive_t ipsw, build_manifest_t build_manifest, const char* output_dir_path) {
    log_message(LOG, "Creating report");
    
    /* Create the nodes */
    plist_t root_node        = plist_new_dict();
    plist_t device_info_node = plist_new_dict();
    plist_t files_info_node  = plist_new_dict();
    plist_t files_info_array = plist_new_array();
    
    if (!root_node) {
        return ILE_E_FAILED_TO_CREATE_PLIST_OBJECT;
    } else if (root_node && (!device_info_node || !files_info_node || !files_info_array)) {
        plist_free(root_node);
        return ILE_E_FAILED_TO_CREATE_PLIST_OBJECT;
    }
    
    /* Populate device_info_node */
    plist_dict_set_item(device_info_node, "product_type",          plist_new_string(build_manifest.product_type));
    plist_dict_set_item(device_info_node, "device_class",          plist_new_string(build_manifest.device_class));
    plist_dict_set_item(device_info_node, "product_build_version", plist_new_string(build_manifest.product_build_version));
    
    /* Populate files_info_node */
    plist_dict_set_item(files_info_node, "file_count", plist_new_uint(build_manifest.file_count));
    for (uint32_t i = 0; i < build_manifest.file_count; i++) {
        plist_t entry = plist_new_dict();
        if (!entry) {
            plist_free(root_node);
            return ILE_E_FAILED_TO_CREATE_PLIST_OBJECT;
        }
        
        /* Populate the entry with all info for this index, then update the array */
        plist_dict_set_item(entry, "path",                    plist_new_string(build_manifest.paths[i]));
        plist_dict_set_item(entry, "manifest_component_name", plist_new_string(build_manifest.manifest_component_names[i]));
        plist_dict_set_item(entry, "encrypted",               plist_new_bool(build_manifest.keys[i].available));
        if (build_manifest.keys[i].available) {
            plist_dict_set_item(entry, "iv",                  plist_new_string(build_manifest.keys[i].iv));
            plist_dict_set_item(entry, "key",                 plist_new_string(build_manifest.keys[i].key));
        }
        
        plist_array_append_item(files_info_array, entry);
    }
    plist_dict_set_item(files_info_node, "files", files_info_array);
    
    /* Append the nodes to the root node */
    plist_dict_set_item(root_node, "device_info", device_info_node);
    plist_dict_set_item(root_node, "files_info",  files_info_node);
    
    /* Create the path and write out to a file */
    char* report_output_path = NULL;
    asprintf(&report_output_path, "%s/report.plist", output_dir_path);
    plist_write_to_file(root_node, report_output_path, PLIST_FORMAT_XML, PLIST_OPT_NONE);
    
    /* Cleanup and return */
    plist_free(root_node);
    free(report_output_path);
    
    log_message(LOG, "Report created successfully");
    
    return ILE_SUCCESS;
}
