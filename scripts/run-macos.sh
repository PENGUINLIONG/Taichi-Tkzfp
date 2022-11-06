#!/bin/bash
set -e

rm -rf build-macos
mkdir build-macos
pushd build-macos
TAICHI_C_API_INSTALL_DIR="${PWD}/../build-taichi-macos/install/c_api" cmake .. \
    -DCMAKE_OSX_ARCHITECTURES="arm64" \
    -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug
popd

TI_LIB_DIR=./build-macos ./build-macos/TaichiAot
