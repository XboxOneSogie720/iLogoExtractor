//
//  utilities.cpp
//  iLogoExtractor
//
//  Created by Karson Eskind on 8/11/23.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <vector>
#include <plist/plist.h>
#include "utilities.hpp"

using namespace std;

const char* ile_strerror(ile_error_t error) {
    switch (error) {
        case ILE_SUCCESS:
            return "No error";
        case ILE_E_OUTPUT_DIR_ALREADY_EXISTS:
            return "The output directory already exists";
        case ILE_E_COULD_NOT_MAKE_OUTPUT_DIR:
            return "Could not make the output directory";
        case ILE_E_COULD_NOT_MAKE_WORK_DIR:
            return "Could not make the work directory";
        case ILE_E_COULD_NOT_DELETE_WORK_DIR:
            return "Could not delete the work directory";
        case ILE_E_IPSW_DOES_NOT_EXIST:
            return "The IPSW does not exist";
        case ILE_E_UNABLE_TO_VERIFY_IPSW_TYPE:
            return "Unable to verify that the IPSW is a zip archive";
        case ILE_E_FAILED_TO_OPEN_IPSW:
            return "Failed to open the IPSW";
        case ILE_E_FAILED_TO_GET_ZIP_INDEX:
            return "Failed to find an expected file in the IPSW. Is your IPSW valid?";
        case ILE_E_FAILED_TO_HANDLE_ZIP:
            return "Failed to read/handle the IPSW";
        case ILE_E_OUT_OF_MEMORY:
            return "Out of memory";
        case ILE_E_PLIST_CONVERSION_FAILED:
            return "Failed to convert to a plist for parsing";
        case ILE_E_PLIST_OBJECT_NOT_FOUND:
            return "An expected object was not found in the plist";
        case ILE_E_FAILED_TO_GET_PLIST_STR_VAL:
            return "Failed to get the string value for an object in the plist";
        case ILE_E_FAILED_TO_ITERATE_OVER_PLIST:
            return "Failed to iterate over items in the plist";
        case ILE_E_FAILED_TO_INITIALIZE_CURL:
            return "Failed to initialize cURL";
        case ILE_E_CURL_PERFORM_FAILED:
            return "Failed to get firmware keys. Check your network connection.";
        case ILE_E_CURL_BAD_RESPONSE:
            return "Wikiproxy is having problems right now, try again later";
        case ILE_E_MISSING_KEYS:
            return "There are missing keys to decrypt firmware images";
        case ILE_E_FAILED_TO_GET_FILE_TYPE:
            return "Failed to get the file type while extracting images";
        case ILE_E_FAILED_TO_OPEN_FILE_FOR_WRITING:
            return "Failed to open a file for writing";
        case ILE_E_FAILED_TO_OPEN_FILE_FOR_READING:
            return "Failed to open a file for reading";
        case ILE_E_IBOOTIM_CORRUPT:
            return "Failed to extract an iBootim image because it was corrupted";
        case ILE_E_THIRD_PARTY_ERROR:
            return "Something went wrong out of this program's scope";
        case ILE_E_FAILED_TO_CREATE_PLIST_OBJECT:
            return "Failed to create a plist object while writing the report";
        case ILE_E_FAILED_TO_WRITE_OUT_REPORT:
            return "Failed to write out the report";
    }
}

void log_message(message_class_t message_class, const char* message) {
    switch (message_class) {
        case LOG:
            printf("[LOG] %s\n", message);
            return; // No syntax colors so we can return early
        case INFO:
            printf("[INFO] %s\n", message);
            return; // No syntax colors so we can return early
        case WARNING:
            printf("%s[WARNING] %s", WARNING_COLOR, message);
            break;
        case ERROR:
            printf("%s[ERROR] %s", ERROR_COLOR, message);
            break;
    }
    
    /* Reset the colors */
    printf("%s\n", RESET_COLOR);
}

bool valid_magic(const char* filename, const char* expected_magic) {
    /* Open the file */
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        return false;
    }
    const size_t read_len = (strlen(expected_magic) - 1);
    
    /* Make sure the file is larger than or equal to the size to be read */
    struct stat fpstat;
    if (fstat(fileno(fp), &fpstat) != 0) {
        fclose(fp);
        return false;
    }
    if (fpstat.st_size < read_len) {
        fclose(fp);
        return false;
    }
    
    /* Read the magic */
    char file_magic[read_len];
    if (fread(file_magic, 1, read_len, fp) != read_len) {
        fclose(fp);
        return false;
    }
    fclose(fp);
    
    /* Compare */
    if (!strncmp(expected_magic, file_magic, (read_len - 1))) { // -1 to ignore null terminator
        return true;
    } else {
        return false;
    }
}

ile_error_t check_io_setup(const char* ipsw_path, const char* output_dir_path) {
    /* IPSW */
    if (access(ipsw_path, F_OK) != 0) {
        return ILE_E_IPSW_DOES_NOT_EXIST;
    } else if (!valid_magic(ipsw_path, IPSW_MAGIC)) {
        return ILE_E_UNABLE_TO_VERIFY_IPSW_TYPE;
    }
    log_message(INFO, "The IPSW is ok at first glance");
    
    /* Output Dir */
    DIR* dp = opendir(output_dir_path);
    if (dp) {
        /* We don't want to clutter up an existing directory */
        closedir(dp);
        return ILE_E_OUTPUT_DIR_ALREADY_EXISTS;
    } else {
        /* Create it */
        if (mkdir(output_dir_path, 0777) != 0) {
            return ILE_E_COULD_NOT_MAKE_OUTPUT_DIR;
        }
    }
    log_message(INFO, "The output directory is ok");
    
    return ILE_SUCCESS;
}

ile_error_t open_work(const char* output_dir_path, char** work_dir_path) {
    /* Create the full path to the work directory */
    asprintf(work_dir_path, "%s/work", output_dir_path);
    
    /* Create it */
    if (mkdir(*work_dir_path, 0777) != 0) {
        free(*work_dir_path);
        return ILE_E_COULD_NOT_MAKE_WORK_DIR;
    }
    
    return ILE_SUCCESS;
}

ile_error_t close_work(char** work_dir_path) {
    /* Delete all items in the directory */
    DIR* dp = opendir(*work_dir_path);
    struct dirent* next_file;
    char* filename = NULL;
    
    while ((next_file = readdir(dp)) != NULL) {
        /* Craft the filename */
        asprintf(&filename, "%s/%s", *work_dir_path, next_file->d_name);
        remove(filename);
        free(filename);
    }
    
    /* Delete the directory */
    if (rmdir(*work_dir_path) != 0) {
        free(*work_dir_path);
        printf("%d\n", rmdir(*work_dir_path));
        printf("%s\n", *work_dir_path);
        return ILE_E_COULD_NOT_DELETE_WORK_DIR;
    }
    
    /* Free resources and return */
    free(*work_dir_path);
    return ILE_SUCCESS;
}

bool is_duplicate_vector_key(vector<const char*> vector, const char* key) {
    /* Enumerate over every index in the vector, checking if any equal the key */
    for (size_t i = 0; i < vector.size(); i++) {
        if (!strcmp(vector[i], key)) {
            /* Already exists */
            return true;
        }
    }
    
    /* Loop didn't find anything - doesn't exist yet */
    return false;
}

const char* manifest_component_name_fixup(const char* original_name) {
    if (!strcmp(original_name, "BatteryPlugin")) {
        return "GlyphPlugin";
    } else if (!strcmp(original_name, "BatteryCharging")) {
        return "GlyphCharging";
    } else {
        /* Don't fix */
        return original_name;
    }
}

ile_error_t fwrite_img3_vector(vector<uint8_t> vector, const char* output_path) {
    FILE* fp = fopen(output_path, "wb");
    if (!fp) {
        return ILE_E_FAILED_TO_OPEN_FILE_FOR_WRITING;
    }
    
    /* Write and close */
    fwrite(vector.data(), vector.size(), sizeof(uint8_t), fp);
    fclose(fp);
    
    return ILE_SUCCESS;
}

ile_error_t fwrite_im4p_buffer(const void* buffer, size_t size, const char* output_path) {
    FILE* fp = fopen(output_path, "wb");
    if (!fp) {
        return ILE_E_FAILED_TO_OPEN_FILE_FOR_WRITING;
    }
    
    /* Write and close */
    fwrite(buffer, size, 1, fp);
    fclose(fp);
    
    return ILE_SUCCESS;
}

const char* get_file_name_from_path(const char* path) {
    const char* filename = strrchr(path, '/');
    return (filename != NULL) ? (filename + 1) : path;
}

void clean_user_input(char* string) {
    size_t len = strlen(string);
    if (len > 0 && string[len - 1] == '\n') {
        string[len - 1] = '\0';
    }
}
