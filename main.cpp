//
//  main.cpp
//  iLogoExtractor
//
//  Created by Karson Eskind on 8/11/23.
//

#include <stdio.h>
#include <stdlib.h>
#include "include/utilities.hpp"
#include "include/ipsw.hpp"
#include "include/api.hpp"
#include "include/extraction.hpp"

int main(int argc, char* argv[]) {
    /* Windows is not supported yet. Let the user know and abort. */
    #ifdef _WIN32
        printf("iLogoExtractor does not currently support running on Windows due to API differences between it, Linux, and macOS.\n");
        return -1;
    #endif

    /* Check Usage */
    if (argc != 3) {
        printf("A utility to extract iBoot images from an IPSW\n");
        printf("Usage: %s <IPSW> <Output Folder>\n", argv[0]);
        return -1;
    }
    
    /* Main Program */
    ile_error_t ret             = ILE_SUCCESS;
    ipsw_archive_t ipsw         = { NULL, argv[1] };
    const char* output_dir_path = argv[2];
    char* work_dir_path         = NULL;
    
    /* Pre Checks */
    ret = check_io_setup(ipsw.path, output_dir_path);
    if (ret != ILE_SUCCESS) {
        log_message(ERROR, ile_strerror(ret));
        return -1;
    }
    
    /* Setup a work environment and open the IPSW */
    ret = open_work(output_dir_path, &work_dir_path);
    if (ret != ILE_SUCCESS) {
        log_message(ERROR, ile_strerror(ret));
        return -1;
    }
    log_message(LOG, "Opening IPSW...");
    ret = ipsw_open(&ipsw);
    if (ret != ILE_SUCCESS) {
        log_message(ERROR, ile_strerror(ret));
        ret = close_work(&work_dir_path); if (ret != ILE_SUCCESS) { log_message(WARNING, ile_strerror(ret)); }
        return -1;
    }
    
    /* Parse the build manifest */
    log_message(LOG, "Parsing the build manifest...");
    build_manifest_t build_manifest = { NULL };
    ret = parse_build_manifest(ipsw, &build_manifest);
    if (ret != ILE_SUCCESS) {
        log_message(ERROR, ile_strerror(ret));
        ret = close_work(&work_dir_path); if (ret != ILE_SUCCESS) { log_message(WARNING, ile_strerror(ret)); }
        return -1;
    }
    
    /* Extract any appropriate images */
    ret = extract_to_output_dir(build_manifest, ipsw, work_dir_path, output_dir_path);
    if (ret != ILE_SUCCESS) {
        log_message(ERROR, ile_strerror(ret));
        ret = close_work(&work_dir_path); if (ret != ILE_SUCCESS) { log_message(WARNING, ile_strerror(ret)); }
        return -1;
    }
    
    ret = write_report(ipsw, build_manifest, output_dir_path);
    if (ret != ILE_SUCCESS) {
        log_message(WARNING, ile_strerror(ret));
    }
    
    /* Tear down */
    log_message(LOG, "Tearing down...");
    ipsw_close(&ipsw);
    build_manifest.paths.clear(); build_manifest.paths.shrink_to_fit();
    build_manifest.manifest_component_names.clear(); build_manifest.manifest_component_names.shrink_to_fit();
    ret = close_work(&work_dir_path); if (ret != ILE_SUCCESS) { log_message(WARNING, ile_strerror(ret)); }
    
    return 0;
}
