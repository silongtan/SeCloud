# SeCloud

## Setup
1. Install prerequisites: `sudo apt-get install cmake build-essential autoconf libtool pkg-config clang-fmt`
2. Install boost library: `sudo apt-get install libboost-all-dev`
3. Install protobuf compiler: `sudo apt-get install protobuf-compiler libprotobuf-dev`
4. Install systemd dependency: `sudo apt install libsystemd-dev`

## Build
1. `mkdir build`
2. `cd build`
3. `cmake ..`
4. `make -j 8`
5. `./SeCloud`

Or use vscode `CMake Tools` extension to build.

