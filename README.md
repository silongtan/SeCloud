# SeCloud

## Setup
1. Install prerequisites: `sudo apt-get install cmake build-essential autoconf libtool pkg-config clang-format`
2. Install boost library: `sudo apt-get install libboost-all-dev`
3. Install protobuf compiler: `sudo apt-get install protobuf-compiler libprotobuf-dev`
4. Install systemd dependency: `sudo apt install libsystemd-dev`
5. Install openssl: `sudo apt-get install libssl-dev`
6. Install gtest: `sudo apt-get install libgtest-dev`

## Build
1. `mkdir build`
2. `cd build`
3. `cmake ..`
4. `make -j 8`
5. `./SeCloud`

## Command line options
1. The first time you run SeCloud, you should run with ./SeCloud --mode setup, which would initalize the backup server encrypted blocks.
2. After completing the setup, you can choose from the following modes:


Or use vscode `CMake Tools` extension to build.

