#!/bin/bash
set -e

BUILD_TYPE="Release"

for arg in "$@"; do
    case $arg in
        Release|RelWithDebInfo|Debug)
            BUILD_TYPE=$arg
            ;;
        clean)
            echo "Cleaning build..."
            rm -rf build
            ;;
    esac
done

echo "Building: $BUILD_TYPE"

mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..
make -j$(nproc)

echo "Build complete"