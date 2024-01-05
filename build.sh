#!/bin/bash

# Check if the build folder exists and remove it if it does
if [ -d "build" ]; then
    echo "Removing existing build folder"
    rm -rf build
fi

# Make a build folder
mkdir -p build

# Compile iBootim portion
echo "Compiling iBootim portion"
gcc -std=c11 -c include/3rdparty/ibootim/ibootim.c -o build/ibootim.o
gcc -std=c11 -c include/3rdparty/ibootim/lzss.c -o build/lzss.o

# Compile iLogoExtractor portion
echo "Compiling iLogoExtractor"
g++ -std=c++17 -c include/utilities.cpp -o build/utilities.o
g++ -std=c++17 -c include/ipsw.cpp -o build/ipsw.o
g++ -std=c++17 -c include/api.cpp -o build/api.o
g++ -std=c++17 -c include/extraction.cpp -o build/extraction.o
g++ -std=c++17 -c main.cpp -o build/main.o

# Link and compile the binary
echo "Linking"

if [[ $(uname) == "Darwin" ]]; then
    # macOS
    g++ -std=c++17 build/api.o build/extraction.o build/ibootim.o build/ipsw.o build/lzss.o build/main.o build/utilities.o -lz -lzip -lcrypto -lssl -framework SystemConfiguration -framework CoreFoundation -framework LDAP -lcurl -lgeneral -limg3tool -limg4tool -lplist-2.0 -lpng -llzma -lbz2 -lcompression -o iLogoExtractor

elif [[ $(uname) == "Linux" ]]; then
    # Linux
    g++ -std=c++17 build/api.o build/extraction.o build/ibootim.o build/ipsw.o build/lzss.o build/main.o build/utilities.o -lpng -lplist-2.0 -lzip -lz -limg3tool -limg4tool -lcurl -lssl -lcrypto -lgeneral -llzfse -o iLogoExtractor
else
    # Some other OS
    echo "[ERROR] Unable to compile because this OS is not supported"
    exit 1

fi

echo "Deleting build folder"
rm -rf build