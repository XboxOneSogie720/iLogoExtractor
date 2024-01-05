//
//  ipsw.hpp
//  iLogoExtractor
//
//  Created by Karson Eskind on 8/11/23.
//

#ifndef ipsw_hpp
#define ipsw_hpp

#include <stdio.h>
#include <vector>
#include <zip.h>

using namespace std;

typedef struct {
    zip* data;
    const char* path;
} ipsw_archive_t;

typedef struct {
    bool available;
    const char* iv;
    const char* key;
} firmware_key_t;

typedef struct {
    /* Required for making the API request */
    char* product_type;
    char* device_class;
    char* product_build_version;
    
    /* Resources for either parsing the API response or actually performing on the files themselves */
    uint32_t file_count;
    vector<const char*> paths;
    vector<const char*> manifest_component_names;
    
    /* Key Related */
    vector<firmware_key_t> keys;
} build_manifest_t;

/**
 Opens an IPSW zip archive
 @param archive Pointer to the IPSW archive
 @return ile_error_t error code
 */
ile_error_t ipsw_open(ipsw_archive_t* archive);

/**
 Closes an IPSW archive
 @param archive Pointer to the IPSW archive
 */
void ipsw_close(ipsw_archive_t* archive);

/**
 Extracts a file from a zip archive directly into memory
 @param archive The IPSW archive
 @param filename The name of the file to extract
 @param buffer Pointer to a return buffer
 @param size Pointer to a return size
 @return ile_error_t error code
 */
ile_error_t extract_ipsw_file_to_memory(ipsw_archive_t archive, const char* filename, char** buffer, size_t* size);

/**
 Parses a build manifest to get info required for a wikiproxy API request and gets useful info about paths
 @param archive The IPSW archive
 @param build_manifest Pointer to the build manifest structure that will be populated with the parsed data
 @return ile_error_t error code
 */
ile_error_t parse_build_manifest(ipsw_archive_t archive, build_manifest_t* build_manifest);

/**
 Writes out a report of the program's analysis of the IPSW
 @param ipsw The ipsw archive
 @param build_manifest The build manifest
 @param output_dir_path The output folder path
 @return ile_error_t error code
 */
ile_error_t write_report(ipsw_archive_t ipsw, build_manifest_t build_manifest, const char* output_dir_path);

#endif /* ipsw_hpp */
