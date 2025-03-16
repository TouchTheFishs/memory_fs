#!/bin/bash
echo "编译 memory_fs..."
cd ../..
if [ ! -d "build" ]; then
    mkdir -p build
fi
cd build
cmake ..
make -j$(nproc)
echo "编译完成"