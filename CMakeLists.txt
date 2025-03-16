cmake_minimum_required(VERSION 3.8)  # 确保CMake版本支持C++17
project(my_project LANGUAGES CXX)    # 明确指定C++项目


# 使用 Debug 模式，确保生成调试符号
set(CMAKE_BUILD_TYPE Debug)

# 添加 -rdynamic 链接选项以确保符号可用
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")

# 设置C++17标准，禁用编译器扩展
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 添加可执行文件
add_executable(memory_fs
    src/timer.cpp
    src/mem_fs_main.cpp
    src/log_utils.cpp
)

# 添加测试可执行文件
add_executable(test_fs_operations test/test_fs_operations.cpp)
add_executable(test_performance test/test_performance.cpp)
add_executable(test_stress test/test_stress.cpp)

# 添加测试
enable_testing()
add_test(NAME FsOperationsTest COMMAND ${CMAKE_BINARY_DIR}/test_path_utils/test_fs_operations)
add_test(NAME PerformanceTest COMMAND ${CMAKE_BINARY_DIR}/test_path_utils/test_performance)
add_test(NAME StressTest COMMAND ${CMAKE_BINARY_DIR}/test_path_utils/test_stress)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set_target_properties(test_fs_operations PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test_path_utils")
set_target_properties(test_performance PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test_path_utils")
set_target_properties(test_stress PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test_path_utils")

add_custom_target(run_all_tests
    COMMAND ${CMAKE_COMMAND} -E echo "Running memory_fs all tests..."
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/test/scripts/run_all_tests.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running memory_fs all tests..."
)

add_custom_target(operations_test
    COMMAND ${CMAKE_COMMAND} -E echo "Running operations_test tests..."
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/test/scripts/test_fs_operations.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running operations_test tests"
)

add_custom_target(performance_test
    COMMAND ${CMAKE_COMMAND} -E echo "Running performance tests..."
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/test/scripts/test_performance.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running performance tests"
)

add_custom_target(stress_test
    COMMAND ${CMAKE_COMMAND} -E echo "Running stress tests..."
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/test/scripts/test_stress.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running stress tests"
)

add_dependencies(run_all_tests operations_test performance_test stress_test test_fs_operations test_performance test_stress)

target_link_libraries(memory_fs PRIVATE fuse3 dl)
