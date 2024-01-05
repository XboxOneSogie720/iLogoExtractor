//
//  extraction.hpp
//  iLogoExtractor
//
//  Created by Karson Eskind on 8/12/23.
//

#ifndef extraction_hpp
#define extraction_hpp

#include "utilities.hpp"
#include "ipsw.hpp"

typedef enum {
    UNKNOWN = -1,
    IMG3    = 0,
    IM4P    = 1
} image_type_t;

/**
 Determines weather a file from the IPSW is an img3, im4p, or should be skipped if it's neither
 @param archive The IPSW archive to get the file contents from
 @param path The path to the file to be examined
 @param image_type Return pointer for the type of image
 @return ile_error_t error code in the event that a helper function fails
 */
ile_error_t get_image_type(ipsw_archive_t archive, const char* path, image_type_t* image_type);

/**
 Saves a png from an ibootim
 @param input_ibootim The path to the input ibootim
 @param manifest_component_name The name of the component being saved
 @param output_dir_path The path to the output directory
 @return ile_error_t error code
 */
ile_error_t save_png_from_ibootim(const char* input_ibootim, const char* manifest_component_name, const char* output_dir_path);

/**
 Extracts the images from the IPSW to the output dir as pngs
 @param build_manifest The build manifest
 @param archive The IPSW archive
 @param work_dir_path The path to the work directory
 @param output_dir_path The output dir path
 @return ile_error_t error code
 */
ile_error_t extract_to_output_dir(build_manifest_t build_manifest, ipsw_archive_t archive, const char* work_dir_path, const char* output_dir_path);

#endif /* extraction_hpp */
