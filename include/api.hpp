//
//  api.hpp
//  iLogoExtractor
//
//  Created by Karson Eskind on 8/12/23.
//

#ifndef api_hpp
#define api_hpp

#include <stdlib.h>
#include <vector>
#include "ipsw.hpp"

using namespace std;

#define WIKIPROXY_SERVER_ERROR "Internal Server Error"

typedef struct {
    char* contents;
    size_t size;
} api_response_t;

/**
 Callback function for curl to write data
 @param contents Contents
 @param size Size
 @param nmemb Nmemb
 @param response Poitner to the response struct to update
 @return The total size of the new data
 */
size_t curl_write_cb(void* contents, size_t size, size_t nmemb, api_response_t* response);

/**
 Function to call the wikiproxy api to get firmware keys
 @param build_manifest The build manifest (must be parsed first)
 @param response Pointer to the resposne structure
 @return ile_error_t error code
 */
ile_error_t call_api(build_manifest_t build_manifest, api_response_t* response);

/**
 Makes the wikiproxy api request and appends the keys found to the build manifest structure
 @param build_manifest Pointer to the build manifest structure
 @return ile_error_t error code
 */
ile_error_t append_keys_to_build_manifest(build_manifest_t* build_manifest);

#endif /* api_hpp */
