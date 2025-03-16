#!/bin/bash
SCRIPT_DIR=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
echo "编译 memory_fs..."
cd "$SCRIPT_DIR/../.." || exit
if [ ! -d "build" ]; then
    mkdir -p build
fi
cd build || { echo "Failed to enter build directory"; exit 1; }
cmake .. || { echo "cmake failed"; exit 1; }
make -j$(nproc) || { echo "make failed"; exit 1; }
echo "编译完成"