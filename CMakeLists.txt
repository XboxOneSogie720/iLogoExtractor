# Set versions
cmake_minimum_required(VERSION 3.11)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Project
project(iLogoExtractor)

# Set SRC files
set(iLogoExtractor_src
    main.cpp
    include/utilities.cpp
    include/ipsw.cpp
    include/extraction.cpp
    include/api.cpp
    include/3rdparty/ibootim/ibootim.c
    include/3rdparty/ibootim/lzss.c
)

# Add executable
add_executable(iLogoExtractor ${iLogoExtractor_src})

# Set include & library search paths
target_include_directories(iLogoExtractor PRIVATE /usr/local/include)
target_link_directories(iLogoExtractor PRIVATE /usr/local/lib)

# Link libraries - both macOS and Linux
target_link_libraries(iLogoExtractor PRIVATE
    z
    zip
    crypto
    ssl
    curl
    "-lgeneral"
    img3tool
    img4tool
    plist-2.0
    png
)

# Link libraries - macOS libs and frameworks
if(APPLE)
    target_link_libraries(iLogoExtractor PRIVATE
        lzma
        bz2
        compression
    )
    target_link_libraries(iLogoExtractor PRIVATE
        "-framework SystemConfiguration"
        "-framework CoreFoundation"
        "-framework LDAP"
    )
endif()

# Link libraries - linux libs
if(UNIX AND NOT APPLE)
    target_link_libraries(iLogoExtractor PRIVATE
        lzfse
    )
endif()

# Install
install(TARGETS iLogoExtractor DESTINATION /usr/local/bin)