@echo off
echo 编译 memory_fs...
cd ..\..\
mkdir -p build
cd build
cmake ..
cmake --build . --config Release
echo 编译完成