//
//  api.cpp
//  iLogoExtractor
//
//  Created by Karson Eskind on 8/12/23.
//

#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <plist/plist.h>
#include "utilities.hpp"
#include "ipsw.hpp"
#include "api.hpp"

size_t curl_write_cb(void* contents, size_t size, size_t nmemb, api_response_t* response) {
    size_t total_size = (size * nmemb);
    
    /* Increase the buffer size to accomodate for the new data */
    response->contents = (char*)realloc(response->contents, (response->size + total_size + 1));
    if (!response->contents) {
        return 0;
    }
    
    /* Copy the received data into the buffer */
    memcpy(&(response->contents[response->size]), contents, total_size);
    
    /* Update the buffer size and null terminate it for safety */
    response->size += total_size;
    response->contents[response->size] = '\0';
    
    return total_size;
}

ile_error_t call_api(build_manifest_t build_manifest, api_response_t* response) {
    log_message(LOG, "Attempting to get firmware keys with wikiproxy...");
    
    /* Initialize cURL */
    CURL* handle = NULL;
    CURLcode res = CURLE_OK;
    curl_global_init(CURL_GLOBAL_ALL);
    handle = curl_easy_init();
    if (!handle) {
        curl_global_cleanup();
        return ILE_E_FAILED_TO_INITIALIZE_CURL;
    }
    
    /* Initialize the response structure */
    response->size = 0;
    response->contents = (char*)malloc(1);
    if (!response->contents) {
        curl_global_cleanup();
        curl_easy_cleanup(handle);
        return ILE_E_OUT_OF_MEMORY;
    } else {
        response->contents[0] = '\0';
    }
    
    /* Create the URL based on the details from the build manifest */
    char* URL = NULL;
    asprintf(&URL, "https://api.m1sta.xyz/wikiproxy/%s/%s/%s", build_manifest.product_type, build_manifest.device_class, build_manifest.product_build_version);
    if (!URL) {
        curl_global_cleanup();
        curl_easy_cleanup(handle);
        free(response->contents);
        return ILE_E_OUT_OF_MEMORY;
    } else {
        /* Setup cURL options */
        curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, curl_write_cb);
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, response);
        curl_easy_setopt(handle, CURLOPT_URL, URL);
        
        /* Make the API request */
        res = curl_easy_perform(handle);
        if (res != CURLE_OK) {
            curl_global_cleanup();
            curl_easy_cleanup(handle);
            free(response->contents);
            free(URL);
            return ILE_E_CURL_PERFORM_FAILED;
        } else {
            /* Verify the response integrity */
            char* p = strstr(response->contents, WIKIPROXY_SERVER_ERROR);
            
            if (response->size <= 0 || p) {
                curl_global_cleanup();
                curl_easy_cleanup(handle);
                free(response->contents);
                free(URL);
                return ILE_E_CURL_BAD_RESPONSE;
            }
        }
    }
    
    /* Cleanup */
    curl_global_cleanup();
    curl_easy_cleanup(handle);
    free(URL);
    
    return ILE_SUCCESS;
}

ile_error_t append_keys_to_build_manifest(build_manifest_t* build_manifest) {
    /* Try to get firmware keys with wikiproxy */
    api_response_t response;
    ile_error_t ret = call_api(*build_manifest, &response);
    if (ret == ILE_E_CURL_PERFORM_FAILED || ret == ILE_E_CURL_BAD_RESPONSE) {
        log_message(ERROR, ile_strerror(ret));
        
        char user_input[128];
        printf("If you want to try entering keys manually, type 'yes' or anything else to exit: ");
        fgets(user_input, sizeof(user_input), stdin); clean_user_input(user_input);
        if (!strcmp(user_input, "yes")) {
            log_message(WARNING, "iLogoExtractor is assuming all keys are correct which makes it more prone to failure");
            
            /* Setup a for loop to ask the user for every key and iv */
            for (uint32_t i = 0; i < build_manifest->file_count; i++) {
                /* Create a working struct */
                firmware_key_t working_firmware_key_struct = { false, NULL, NULL };
                
                printf("\n[%s]\n[%s]\n", build_manifest->manifest_component_names[i], get_file_name_from_path(build_manifest->paths[i]));
                printf("Type 'yes' if you have keys for this component or 'no' if otherwise: ");
                fgets(user_input, sizeof(user_input), stdin); clean_user_input(user_input);
                if (!strcmp(user_input, "yes")) {
                    /* Keys are available */
                    working_firmware_key_struct.available = true;
                    
                    /* Ask for the IV */
                    printf("Enter the iv for [%s]: ", build_manifest->manifest_component_names[i]);
                    fgets(user_input, sizeof(user_input), stdin); clean_user_input(user_input);
                    working_firmware_key_struct.iv = strdup(user_input);
                    
                    /* Ask for the key */
                    printf("Enter the key for [%s]: ", build_manifest->manifest_component_names[i]);
                    fgets(user_input, sizeof(user_input), stdin); clean_user_input(user_input);
                    working_firmware_key_struct.key = strdup(user_input);
                    
                } else if (!strcmp(user_input, "no")) {
                    working_firmware_key_struct.available = false;
                } else {
                    return ILE_E_MISSING_KEYS;
                }
                
                build_manifest->keys.push_back(working_firmware_key_struct);
                log_message(LOG, "Keys updated successfully");
            }
            
            /* Return early with an assumed success since the user provided keys instead of the API */
            return ILE_SUCCESS;
        } else {
            /* Exit because no keys are available */
            return ILE_E_MISSING_KEYS;
        }
    } else if (ret != ILE_SUCCESS && ret != ILE_E_CURL_PERFORM_FAILED && ret != ILE_E_CURL_BAD_RESPONSE) {
        return ret;
    }
    
    /* Convert the JSON into a plist */
    plist_t root_node = NULL;
    if (plist_from_json(response.contents, (uint32_t)response.size, &root_node) != PLIST_ERR_SUCCESS) {
        free(response.contents);
        return ILE_E_PLIST_CONVERSION_FAILED;
    } else {
        /* We don't need the original buffer anymore */
        free(response.contents);
    }
    
    /* Determine what kind of response it is - Does it have an array? */
    bool response_contains_array = false;
    plist_dict_iter root_iterator = NULL;
    plist_dict_new_iter(root_node, &root_iterator);
    
    plist_t item = NULL; // If an array is found, this will hold that node
    if (!root_iterator) {
        plist_free(root_node);
        return ILE_E_FAILED_TO_ITERATE_OVER_PLIST;
    } else {
        const uint32_t ROOT_NODE_SIZE = plist_dict_get_size(root_node);
        for (uint32_t i = 0; i < ROOT_NODE_SIZE; i++) {
            /* Get the next item */
            char* key = NULL;
            plist_dict_next_item(root_node, root_iterator, &key, &item);
            
            /* Check the type */
            if (plist_get_node_type(item) == PLIST_ARRAY) {
                response_contains_array = true;
                break;
            }
        }
        free(root_iterator);
    }
    
    if (response_contains_array) {
        log_message(INFO, "Response contains array: True");
        
        /* We'll have to do iteration instead of directly looking for the iv and key in the response structure */
        const uint32_t KEYS_ARRAY_SIZE = plist_array_get_size(item);
        
        for (uint32_t i = 0; i < build_manifest->file_count; i++) {
            /* Create a working struct */
            firmware_key_t working_firmware_key_struct = { false, NULL, NULL };
            
            plist_array_iter keys_iterator = NULL;
            plist_array_new_iter(item, &keys_iterator);
            
            if (!keys_iterator) {
                plist_free(root_node);
                return ILE_E_FAILED_TO_ITERATE_OVER_PLIST;
            } else {
                bool component_is_encrypted = false;
                for (uint32_t j = 0; j < KEYS_ARRAY_SIZE; j++) {
                    /* Get the next item */
                    plist_t component_key_node = NULL;
                    plist_array_next_item(item, keys_iterator, &component_key_node);
                    
                    if (!component_key_node) {
                        free(keys_iterator);
                        plist_free(root_node);
                        return ILE_E_PLIST_OBJECT_NOT_FOUND;
                    } else {
                        /* Get the string value of image */
                        plist_t image = plist_dict_get_item(component_key_node, "image");
                        
                        if (!image) {
                            free(keys_iterator);
                            plist_free(root_node);
                            return ILE_E_PLIST_OBJECT_NOT_FOUND;
                        } else {
                            /* Get the string value */
                            char* image_string_value = NULL;
                            plist_get_string_val(image, &image_string_value);
                            
                            if (!image_string_value) {
                                free(keys_iterator);
                                plist_free(root_node);
                                return ILE_E_FAILED_TO_GET_PLIST_STR_VAL;
                            } else {
                                /* Compare it against this index's name */
                                if (!strcmp(build_manifest->manifest_component_names[i], image_string_value)) {
                                    component_is_encrypted = true;
                                    /* Try to get the IV and Key */
                                    plist_t iv  = plist_dict_get_item(component_key_node, "iv");
                                    plist_t key = plist_dict_get_item(component_key_node, "key");
                                    
                                    if (iv && key) {
                                        /* Get their string values */
                                        char* iv_string_value  = NULL; plist_get_string_val(iv, &iv_string_value);
                                        char* key_string_value = NULL; plist_get_string_val(key, &key_string_value);
                                        
                                        if (iv_string_value && key_string_value) {
                                            /* Update the working struct */
                                            working_firmware_key_struct.available = true;
                                            working_firmware_key_struct.iv        = iv_string_value;
                                            working_firmware_key_struct.key       = key_string_value;
                                        } else {
                                            free(keys_iterator);
                                            plist_free(root_node);
                                            return ILE_E_FAILED_TO_GET_PLIST_STR_VAL;
                                        }
                                    }
                                    
                                    break;
                                }
                            }
                        }
                    }
                }
                if (!component_is_encrypted) {
                    working_firmware_key_struct.available = false;
                }
                
                /* Push back */
                build_manifest->keys.push_back(working_firmware_key_struct);
                
                free(keys_iterator);
            }
        }
    } else {
        log_message(INFO, "Response contains array: False");
        
        /* We can re-format the component names and just get them from the response structure */
        for (uint32_t i = 0; i < build_manifest->file_count; i++) {
            /* Create a working struct */
            firmware_key_t working_firmware_key_struct = { false, NULL, NULL };
            
            /* Create the key to get the IV and Key */
            char* iv_plist_key = NULL;
            asprintf(&iv_plist_key,  "%sIV",  build_manifest->manifest_component_names[i]);
            
            char* key_plist_key = NULL;
            asprintf(&key_plist_key, "%sKey", build_manifest->manifest_component_names[i]);
            
            /* Try to find this component element in the response */
            plist_t iv  = plist_dict_get_item(root_node, iv_plist_key);
            plist_t key = plist_dict_get_item(root_node, key_plist_key);
            
            if (iv && key) {
                /* Get the string values for the iv and key */
                char* iv_string_value  = NULL; plist_get_string_val(iv, &iv_string_value);
                char* key_string_value = NULL; plist_get_string_val(key, &key_string_value);
                
                if (iv_string_value && key_string_value) {
                    /* Update the working struct */
                    working_firmware_key_struct.available = true;
                    working_firmware_key_struct.iv        = iv_string_value;
                    working_firmware_key_struct.key       = key_string_value;
                } else {
                    plist_free(root_node);
                    return ILE_E_FAILED_TO_GET_PLIST_STR_VAL;
                }
            } else {
                /* The response doesn't contain this component name */
                working_firmware_key_struct.available = false;
            }
            
            /* Push back */
            build_manifest->keys.push_back(working_firmware_key_struct);
            
            /* Free resources */
            free(iv_plist_key);
            free(key_plist_key);
        }
    }
    
    return ILE_SUCCESS;
}
