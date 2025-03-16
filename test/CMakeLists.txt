cmake_minimum_required(VERSION 3.10)
project(memory_fs_tests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加测试可执行文件
add_executable(test_fs_operations test_fs_operations.cpp)
add_executable(test_performance test_performance.cpp)
add_executable(test_stress test_stress.cpp)

# 添加测试
enable_testing()
add_test(NAME FsOperationsTest COMMAND test_fs_operations)
add_test(NAME PerformanceTest COMMAND test_performance)
add_test(NAME StressTest COMMAND test_stress)

# 添加自定义测试目标
add_custom_target(run_tests
    COMMAND ${CMAKE_COMMAND} -E echo "Running memory_fs tests..."
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/run_all_tests.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running memory_fs tests"
)