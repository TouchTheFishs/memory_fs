#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <chrono>
#include <vector>
#include <random>
#include <algorithm>

namespace fs = std::filesystem;
using namespace std::chrono;

// 测试配置
const std::string MOUNT_POINT = fs::absolute("test/mount_point").string();
const std::string NATIVE_DIR = fs::absolute("test/native_dir").string();
const size_t FILE_SIZE_SMALL = 4 * 1024;        // 4KB
const size_t FILE_SIZE_MEDIUM = 1 * 1024 * 1024; // 1MB
const size_t FILE_SIZE_LARGE = 10 * 1024 * 1024; // 10MB
const int NUM_FILES = 10;
const int NUM_ITERATIONS = 5;

// 生成随机数据
std::vector<char> generate_random_data(size_t size) {
    std::vector<char> data(size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    for (size_t i = 0; i < size; i++) {
        data[i] = static_cast<char>(dis(gen));
    }

    return data;
}

// 写入文件
double test_write_performance(const std::string& dir, const std::vector<char>& data, int num_files) {
    auto start = high_resolution_clock::now();

    for (int i = 0; i < num_files; i++) {
        std::string file_path = dir + "/perf_test_" + std::to_string(i) + ".bin";
        std::ofstream file(file_path, std::ios::binary);
        file.write(data.data(), data.size());
        file.close();
    }

    auto end = high_resolution_clock::now();
    return duration_cast<milliseconds>(end - start).count() / 1000.0;
}

// 读取文件
double test_read_performance(const std::string& dir, size_t data_size, int num_files) {
    auto start = high_resolution_clock::now();

    for (int i = 0; i < num_files; i++) {
        std::string file_path = dir + "/perf_test_" + std::to_string(i) + ".bin";
        std::ifstream file(file_path, std::ios::binary);
        std::vector<char> buffer(data_size);
        file.read(buffer.data(), data_size);
        file.close();
    }

    auto end = high_resolution_clock::now();
    return duration_cast<milliseconds>(end - start).count() / 1000.0;
}

// 删除文件
double test_delete_performance(const std::string& dir, int num_files) {
    auto start = high_resolution_clock::now();

    for (int i = 0; i < num_files; i++) {
        std::string file_path = dir + "/perf_test_" + std::to_string(i) + ".bin";
        fs::remove(file_path);
    }

    auto end = high_resolution_clock::now();
    return duration_cast<milliseconds>(end - start).count() / 1000.0;
}

// 创建目录
double test_mkdir_performance(const std::string& dir, int num_dirs) {
    auto start = high_resolution_clock::now();

    for (int i = 0; i < num_dirs; i++) {
        std::string dir_path = dir + "/perf_dir_" + std::to_string(i);
        fs::create_directory(dir_path);
    }

    auto end = high_resolution_clock::now();
    return duration_cast<milliseconds>(end - start).count() / 1000.0;
}

// 删除目录
double test_rmdir_performance(const std::string& dir, int num_dirs) {
    auto start = high_resolution_clock::now();

    for (int i = 0; i < num_dirs; i++) {
        std::string dir_path = dir + "/perf_dir_" + std::to_string(i);
        fs::remove(dir_path);
    }

    auto end = high_resolution_clock::now();
    return duration_cast<milliseconds>(end - start).count() / 1000.0;
}

// 运行所有性能测试
void run_performance_tests() {
    std::cout << "=== 性能测试开始 ===" << std::endl;

    // 创建测试目录
    fs::create_directories(MOUNT_POINT);
    fs::create_directories(NATIVE_DIR);

    // 生成测试数据
    std::vector<char> small_data = generate_random_data(FILE_SIZE_SMALL);
    std::vector<char> medium_data = generate_random_data(FILE_SIZE_MEDIUM);
    std::vector<char> large_data = generate_random_data(FILE_SIZE_LARGE);

    // 测试小文件写入性能
    double memfs_small_write_time = 0;
    double native_small_write_time = 0;

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        memfs_small_write_time += test_write_performance(MOUNT_POINT, small_data, NUM_FILES);
        native_small_write_time += test_write_performance(NATIVE_DIR, small_data, NUM_FILES);

        // 清理文件
        test_delete_performance(MOUNT_POINT, NUM_FILES);
        test_delete_performance(NATIVE_DIR, NUM_FILES);
    }

    memfs_small_write_time /= NUM_ITERATIONS;
    native_small_write_time /= NUM_ITERATIONS;

    std::cout << "小文件(4KB)写入性能:" << std::endl;
    std::cout << "  Memory FS: " << memfs_small_write_time << " 秒" << std::endl;
    std::cout << "  本地文件系统: " << native_small_write_time << " 秒" << std::endl;
    std::cout << "  性能比: " << (native_small_write_time / memfs_small_write_time) << "x" << std::endl;

    // 测试中等文件写入性能
    double memfs_medium_write_time = 0;
    double native_medium_write_time = 0;

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        memfs_medium_write_time += test_write_performance(MOUNT_POINT, medium_data, NUM_FILES);
        native_medium_write_time += test_write_performance(NATIVE_DIR, medium_data, NUM_FILES);

        // 清理文件
        test_delete_performance(MOUNT_POINT, NUM_FILES);
        test_delete_performance(NATIVE_DIR, NUM_FILES);
    }

    memfs_medium_write_time /= NUM_ITERATIONS;
    native_medium_write_time /= NUM_ITERATIONS;

    std::cout << "中等文件(1MB)写入性能:" << std::endl;
    std::cout << "  Memory FS: " << memfs_medium_write_time << " 秒" << std::endl;
    std::cout << "  本地文件系统: " << native_medium_write_time << " 秒" << std::endl;
    std::cout << "  性能比: " << (native_medium_write_time / memfs_medium_write_time) << "x" << std::endl;

    // 测试大文件写入性能
    double memfs_large_write_time = 0;
    double native_large_write_time = 0;

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        memfs_large_write_time += test_write_performance(MOUNT_POINT, large_data, NUM_FILES);
        native_large_write_time += test_write_performance(NATIVE_DIR, large_data, NUM_FILES);

        // 清理文件
        test_delete_performance(MOUNT_POINT, NUM_FILES);
        test_delete_performance(NATIVE_DIR, NUM_FILES);
    }

    memfs_large_write_time /= NUM_ITERATIONS;
    native_large_write_time /= NUM_ITERATIONS;

    std::cout << "大文件(10MB)写入性能:" << std::endl;
    std::cout << "  Memory FS: " << memfs_large_write_time << " 秒" << std::endl;
    std::cout << "  本地文件系统: " << native_large_write_time << " 秒" << std::endl;
    std::cout << "  性能比: " << (native_large_write_time / memfs_large_write_time) << "x" << std::endl;

    // 测试读取性能
    // 先写入文件
    test_write_performance(MOUNT_POINT, medium_data, NUM_FILES);
    test_write_performance(NATIVE_DIR, medium_data, NUM_FILES);

    double memfs_read_time = 0;
    double native_read_time = 0;

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        memfs_read_time += test_read_performance(MOUNT_POINT, FILE_SIZE_MEDIUM, NUM_FILES);
        native_read_time += test_read_performance(NATIVE_DIR, FILE_SIZE_MEDIUM, NUM_FILES);
    }

    memfs_read_time /= NUM_ITERATIONS;
    native_read_time /= NUM_ITERATIONS;

    std::cout << "文件读取性能(1MB):" << std::endl;
    std::cout << "  Memory FS: " << memfs_read_time << " 秒" << std::endl;
    std::cout << "  本地文件系统: " << native_read_time << " 秒" << std::endl;
    std::cout << "  性能比: " << (native_read_time / memfs_read_time) << "x" << std::endl;

    // 清理文件
    test_delete_performance(MOUNT_POINT, NUM_FILES);
    test_delete_performance(NATIVE_DIR, NUM_FILES);

    // 测试目录操作性能
    double memfs_mkdir_time = 0;
    double native_mkdir_time = 0;

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        memfs_mkdir_time += test_mkdir_performance(MOUNT_POINT, NUM_FILES);
        native_mkdir_time += test_mkdir_performance(NATIVE_DIR, NUM_FILES);

        // 清理目录
        test_rmdir_performance(MOUNT_POINT, NUM_FILES);
        test_rmdir_performance(NATIVE_DIR, NUM_FILES);
    }

    memfs_mkdir_time /= NUM_ITERATIONS;
    native_mkdir_time /= NUM_ITERATIONS;

    std::cout << "目录创建性能:" << std::endl;
    std::cout << "  Memory FS: " << memfs_mkdir_time << " 秒" << std::endl;
    std::cout << "  本地文件系统: " << native_mkdir_time << " 秒" << std::endl;
    std::cout << "  性能比: " << (native_mkdir_time / memfs_mkdir_time) << "x" << std::endl;

    std::cout << "=== 性能测试完成 ===" << std::endl;
}

int main() {
    run_performance_tests();
    return 0;
}