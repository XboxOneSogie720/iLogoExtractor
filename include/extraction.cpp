//
//  extraction.cpp
//  iLogoExtractor
//
//  Created by Karson Eskind on 8/12/23.
//

#include <img3tool/img3tool.hpp>
#include <img4tool/img4tool.hpp>
#include <string.h>
#include <vector>
#include "extraction.hpp"
#include "utilities.hpp"
#include "ipsw.hpp"

extern "C" {
    #include "3rdparty/ibootim/ibootim.h"
}

using namespace std;
using namespace tihmstar::img3tool;
using namespace tihmstar::img4tool;

ile_error_t get_image_type(ipsw_archive_t archive, const char* path, image_type_t* image_type) {
    char* file_buffer = NULL;
    size_t file_size  = 0;
    ile_error_t ret = extract_ipsw_file_to_memory(archive, path, &file_buffer, &file_size);
    if (ret != ILE_SUCCESS) {
        return ret;
    }

    /* Compare */
    if (!strncmp(IMG3_MAGIC, file_buffer, (strlen(IMG3_MAGIC) - 1))) {
        *image_type = IMG3;
    } else {
        ASN1DERElement working_buffer(file_buffer, file_size);
        if (isIM4P(working_buffer)) {
            *image_type = IM4P;
        } else {
            *image_type = UNKNOWN;
        }
    }
    
    /* Cleanup and return */
    free(file_buffer);
    return ILE_SUCCESS;
}

ile_error_t save_png_from_ibootim(const char* input_ibootim, const char* manifest_component_name, const char* output_dir_path) {
    ibootim* image = NULL;
    unsigned int images_count = ibootim_count_images_in_file(input_ibootim, NULL);
    int rc = 0;
    
    for (unsigned int i = 0; i < images_count; i++) {
        /* Load this image */
        if ((rc = ibootim_load_at_index(input_ibootim, &image, i)) != 0) {
            switch (rc) {
                case ENOMEM:
                    return ILE_E_OUT_OF_MEMORY;
                case EFTYPE:
                    return ILE_E_IBOOTIM_CORRUPT;
                case ENOENT:
                    return ILE_E_FAILED_TO_OPEN_FILE_FOR_READING;
                default:
                    return ILE_E_THIRD_PARTY_ERROR;
            }
        }
        
        /* Make the path index name */
        char* full_output_path = NULL;
        if (images_count == 1) {
            asprintf(&full_output_path, "%s/%s.png", output_dir_path, manifest_component_name);
        } else {
            asprintf(&full_output_path, "%s/%s_%u.png", output_dir_path, manifest_component_name, i);
        }
        
        /* Save the image */
        if (ibootim_write_png(image, full_output_path) != 0) {
            ibootim_close(image);
            free(full_output_path);
            return ILE_E_THIRD_PARTY_ERROR;
        }
        
        /* Cleanup */
        ibootim_close(image);
        free(full_output_path);
    }
    
    return ILE_SUCCESS;
}

ile_error_t extract_to_output_dir(build_manifest_t build_manifest, ipsw_archive_t archive, const char* work_dir_path, const char* output_dir_path) {
    /* For every image, we'll do some checks, extract the payload, convert, and save the image */
    printf("\n");
    log_message(INFO, "iBootim is being used for conversion - Copyright 2015 Pupyshev Nikita | All rights reserved");
    log_message(LOG, "Extracting ibootim images...");
    for (uint32_t i = 0; i < build_manifest.file_count; i++) {
        /* Get the image type */
        image_type_t image_type = UNKNOWN;
        ile_error_t ret = get_image_type(archive, build_manifest.paths[i], &image_type);
        if (ret != ILE_SUCCESS || image_type == UNKNOWN) {
            return ILE_E_FAILED_TO_GET_FILE_TYPE;
        }
        
        if (image_type == IMG3) {
            /* Use img3tool */
            printf("Attempting to extract IMG3 Component [%s]...\n", build_manifest.manifest_component_names[i]);
            
            /* Load into memory */
            char* component_buffer = NULL;
            size_t component_size  = 0;
            ret = extract_ipsw_file_to_memory(archive, build_manifest.paths[i], &component_buffer, &component_size);
            if (ret != ILE_SUCCESS) {
                log_message(WARNING, "A file that was found in BuildManifest.plist was not found in this IPSW");
            } else {
                /* When working with img3, we must use a vector of uint8_t, so we'll need to convert it here */
                vector<uint8_t> img3(component_buffer, component_buffer + component_size);
                free(component_buffer);
                
                /* Extract the payload */
                vector<uint8_t> payload;
                try {
                    payload = getPayloadFromIMG3(img3.data(), img3.size(), build_manifest.keys[i].iv, build_manifest.keys[i].key);
                } catch (...) {
                    return ILE_E_FAILED_TO_OPEN_FILE_FOR_READING;
                }
                
                /* iBootim (libpng really) requires that files be loaded from a file, so we must save this payload in the work folder */
                char* extracted_payload_out_path = NULL;
                asprintf(&extracted_payload_out_path, "%s/%s.ibootim", work_dir_path, build_manifest.manifest_component_names[i]);
                ret = fwrite_img3_vector(payload, extracted_payload_out_path);
                if (ret != ILE_SUCCESS) {
                    free(extracted_payload_out_path);
                    return ret;
                }
                
                /* Save */
                ret = save_png_from_ibootim(extracted_payload_out_path, build_manifest.manifest_component_names[i], output_dir_path);
                if (ret != ILE_SUCCESS) {
                    free(extracted_payload_out_path);
                    return ret;
                }
            }
        } else {
            /* Use img4tool */
            printf("Attempting to extract IM4P Component [%s]...\n", build_manifest.manifest_component_names[i]);
            
            char* component_buffer = NULL;
            size_t component_size = 0;
            ret = extract_ipsw_file_to_memory(archive, build_manifest.paths[i], &component_buffer, &component_size);
            if (ret != ILE_SUCCESS) {
                log_message(WARNING, "A file that was found in BuildManifest.plist was not found in this IPSW");
            } else {
                ASN1DERElement payload;
                try {
                    /* Get the root node */
                    ASN1DERElement im4p(component_buffer, component_size);
                    
                    /* Extract the payload */
                    payload = getPayloadFromIM4P(im4p, build_manifest.keys[i].iv, build_manifest.keys[i].key);
                } catch (...) {
                    free(component_buffer);
                    return ILE_E_FAILED_TO_OPEN_FILE_FOR_READING;
                }
                    
                
                /* iBootim (libpng really) requires that files be loaded from a file, so we must save this payload in the work folder */
                char* extracted_payload_out_path = NULL;
                asprintf(&extracted_payload_out_path, "%s/%s.ibootim", work_dir_path, build_manifest.manifest_component_names[i]);
                ret = fwrite_im4p_buffer(payload.payload(), payload.payloadSize(), extracted_payload_out_path);
                if (ret != ILE_SUCCESS) {
                    free(extracted_payload_out_path);
                    return ret;
                }
                
                /* Save */
                ret = save_png_from_ibootim(extracted_payload_out_path, build_manifest.manifest_component_names[i], output_dir_path);
                if (ret != ILE_SUCCESS) {
                    free(extracted_payload_out_path);
                    return ret;
                }
            }
        }
    }
    
    /* Any debug messages printed by img3tool or img4tool can be separated from the rest of the program */
    log_message(INFO, "Extraction completed successfully\n");
    
    return ILE_SUCCESS;
}
