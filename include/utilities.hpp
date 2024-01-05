//
//  utilities.hpp
//  iLogoExtractor
//
//  Created by Karson Eskind on 8/11/23.
//

#ifndef utilities_hpp
#define utilities_hpp

#include <stdio.h>
#include <vector>
#include <cstdint>

using namespace std;

/* File Magics */
#define IPSW_MAGIC     "PK"
#define IMG3_MAGIC     "3gmI"
#define IM4P_MAGIC     ""
#define ALL_FLASH_PATH "Firmware/all_flash/"

/* ANSI Escape Codes */
#define WARNING_COLOR "\033[33m"
#define ERROR_COLOR   "\033[31m"
#define RESET_COLOR   "\033[0m"

typedef enum {
    ILE_SUCCESS                           = 0,
    ILE_E_OUTPUT_DIR_ALREADY_EXISTS       = -1,
    ILE_E_COULD_NOT_MAKE_OUTPUT_DIR       = -2,
    ILE_E_COULD_NOT_MAKE_WORK_DIR         = -3,
    ILE_E_COULD_NOT_DELETE_WORK_DIR       = -4,
    ILE_E_IPSW_DOES_NOT_EXIST             = -5,
    ILE_E_UNABLE_TO_VERIFY_IPSW_TYPE      = -6,
    ILE_E_FAILED_TO_OPEN_IPSW             = -7,
    ILE_E_FAILED_TO_GET_ZIP_INDEX         = -8,
    ILE_E_FAILED_TO_HANDLE_ZIP            = -9,
    ILE_E_OUT_OF_MEMORY                   = -10,
    ILE_E_PLIST_CONVERSION_FAILED         = -11,
    ILE_E_PLIST_OBJECT_NOT_FOUND          = -12,
    ILE_E_FAILED_TO_GET_PLIST_STR_VAL     = -13,
    ILE_E_FAILED_TO_ITERATE_OVER_PLIST    = -14,
    ILE_E_FAILED_TO_INITIALIZE_CURL       = -15,
    ILE_E_CURL_PERFORM_FAILED             = -16,
    ILE_E_CURL_BAD_RESPONSE               = -17,
    ILE_E_MISSING_KEYS                    = -18,
    ILE_E_FAILED_TO_GET_FILE_TYPE         = -19,
    ILE_E_FAILED_TO_OPEN_FILE_FOR_WRITING = -20,
    ILE_E_FAILED_TO_OPEN_FILE_FOR_READING = -21,
    ILE_E_IBOOTIM_CORRUPT                 = -22,
    ILE_E_THIRD_PARTY_ERROR               = -23,
    ILE_E_FAILED_TO_CREATE_PLIST_OBJECT   = -24,
    ILE_E_FAILED_TO_WRITE_OUT_REPORT      = -25
} ile_error_t;

typedef enum {
    LOG     = 1,
    INFO    = 2,
    WARNING = 3,
    ERROR   = 4
} message_class_t;

/**
 Returns a human readable error code
 @param error The ile_error error code
 @return The human readable error code
 */
const char* ile_strerror(ile_error_t error);

/**
 Logs a message to the console with a choice of format
 @param message_class The type of message (log info warning error)
 @param message The raw message
 */
void log_message(message_class_t message_class, const char* message);

/**
 Compares a file's actual magic with the expected magic
 @param filename The name of the file
 @param expected_magic The expected magic
 @return True if the magics are the same, false if not or if there's an error
 */
bool valid_magic(const char* filename, const char* expected_magic);

/**
 Makes sure I/O is ready to go, with an IPSW that exists and is a zip, and an output dir that doesn't exist (which is created by this function)
 @param ipsw_path Path to the IPSW
 @param output_dir_path Path to the output directory
 @return ile_error_t error code
 */
ile_error_t check_io_setup(const char* ipsw_path, const char* output_dir_path);

/**
 Opens a work environment and returns ownership of the full path
 @param output_dir_path Path to the output directory
 @param work_dir_path Pointer to the char* that will hold the full path to the work directory
 @return ile_error_t error code
 */
ile_error_t open_work(const char* output_dir_path, char** work_dir_path);

/**
 Closes a work environment
 @param work_dir_path Path to the work directory, is pointer so it can be freed
 @return ile_error_t error code
 */
ile_error_t close_work(char** work_dir_path);

/**
 Checks if a key already exists in a vectory (type char*)
 @param vector The vector of char* to be checked
 @param key The key to be searched for in the vector
 @return True if the key already exists, false if not
 */
bool is_duplicate_vector_key(vector<const char*> vector, const char* key);

/**
 Fixes up component names so that wikiproxy's response will recognize the key
 @param name The original component's name
 @return The fixed name
 */
const char* manifest_component_name_fixup(const char* name);

/**
 Writes out an img3 vector (vector of uint8_t)
 @param vector The vector source
 @param output_path The path to save the file
 @return ile_error_t error code
 */
ile_error_t fwrite_img3_vector(vector<uint8_t> vector, const char* output_path);

/**
 Writes out a char* buffer
 @param buffer The buffer
 @param size The buffer size
 @param output_path The filename
 @return ile_error_t error code
 */
ile_error_t fwrite_im4p_buffer(const void* buffer, size_t size, const char* output_path);

/**
 Returns just the filename from a path
 @param path The path to the file
 @return The filename without the path
 */
const char* get_file_name_from_path(const char* path);

/**
 Replaces the newline character with a null terminator if necessary to not mess up the keys from user input
 @param string The user input container
 */
void clean_user_input(char* string);

#endif /* utilities_hpp */
