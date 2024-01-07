# iLogoExtractor
A utility to automatically extract iBoot images like the Apple logo to a specified output folder

![Build Status](https://img.shields.io/badge/Build_Status-macOS_&_Linux-green)
![Windows Support Progress](https://img.shields.io/badge/Windows_Support-Installing_Bootcamp_to_Start_Testing-yellow)

![Demo](https://github.com/XboxOneSogie720/iLogoExtractor/blob/main/Media/demo.gif)

# Download Release
To download one of the available binaries, see the [releases page](https://github.com/XboxOneSogie720/iLogoExtractor/releases):
* macOS (x86_64)
* macOS (arm64)
* macOS (Universal)
* Linux (x86_64, tested on Ubuntu)

# Build Instructions
### Note: iLogoExtractor does not currently support Windows at all (but I'm working on it)

Install Dependencies (not including deps for these listed):
* macOS & Linux
  * [libz](https://github.com/madler/zlib)
  * [libzip](https://github.com/nih-at/libzip)
  * [libcrypto & libssl (OpenSSL)](https://www.openssl.org/source/)
  * [libcurl](https://curl.se/download.html)
  * [libgeneral](https://github.com/tihmstar/libgeneral)
  * [libimg3tool](https://github.com/tihmstar/img3tool)
  * [libimg4tool](https://github.com/tihmstar/img4tool)
  * [libplist](https://github.com/libimobiledevice/libplist)
  * [libpng](https://github.com/glennrp/libpng)
* macOS Specific:
  * [liblzma](https://github.com/kobolabs/liblzma)
  * [libbz2](https://github.com/libarchive/bzip2)
  * libcompression (Should come with macOS)
* Linux Specific:
  * [liblzfse](https://github.com/lzfse/lzfse)
 
Use CMake to build and install iLogoExtractor:

```bash
mkdir build
cd build
cmake ..
make
sudo make install
```

# Usage
```./iLogoExtractor <IPSW> <Output Folder>```

Know that it will not clutter up an existing folder so make sure it doesn't exist yet

# Features
* Automatic parsing of the contents
* **Fast Performance** - It will parse the BuildManifest to figure out the only files it needs to look at, and manages them from memory instead of extracting
* Automatic Key Grabbing - Using Wikiproxy, it will automatically fetch keys and decrypt if necessary
* Offline Support - If you don't have network access or if Wikiproxy is down, find your IPSW version on [The Apple Wiki](https://theapplewiki.com/wiki/Firmware_Keys) to supply keys manually. Pay attention to the filenames provided to make sure you provide the right keys if you decide to do so.
* Report Generation - It will generate a simple plist that contains the device product information used for the API request, as well as all of the files and the keys used for decryption if necessary

## Credits
[Tihmstar](https://github.com/tihmstar) - img3tool and img4tool (Used for actually decrypting and/or unpacking payloads)

[realnp](https://github.com/realnp) - ibootim (For converting the ibootim images into the .pngs)
