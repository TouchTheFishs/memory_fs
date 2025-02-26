cmake_minimum_required(VERSION 3.8)  # 确保CMake版本支持C++17
project(my_project LANGUAGES CXX)    # 明确指定C++项目

# 设置构建类型为 Debug（确保生成调试符号）
set(CMAKE_BUILD_TYPE Debug)  # 可选，也可通过命令行指定：cmake -DCMAKE_BUILD_TYPE=Debug ..

# 设置C++17标准，禁用编译器扩展
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 添加可执行文件
add_executable(mem_fs
    src/mem_fs_main.cpp
    src/mem_fs_utils.cpp
)

target_link_libraries(mem_fs PRIVATE fuse3)
