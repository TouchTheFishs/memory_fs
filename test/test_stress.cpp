#include <condition_variable>
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>
#include <random>
#include <chrono>

namespace fs = std::filesystem;

// 测试配置
const std::string MOUNT_POINT = "../test/mount_point";
const int NUM_THREADS = 8;
const int NUM_OPERATIONS = 100;
const int MAX_FILE_SIZE = 1024 * 1024; // 1MB

// 线程安全的随机数生成
class RandomGenerator {
private:
    std::mt19937 gen;
    std::mutex mutex;

public:
    RandomGenerator() : gen(std::random_device()()) {}

    int getRandomInt(int min, int max) {
        std::lock_guard<std::mutex> lock(mutex);
        std::uniform_int_distribution<> dis(min, max);
        return dis(gen);
    }

    std::string getRandomString(size_t length) {
        static const char alphanum[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";
        std::lock_guard<std::mutex> lock(mutex);
        std::string result;
        result.reserve(length);
        for (size_t i = 0; i < length; ++i) {
            result += alphanum[std::uniform_int_distribution<>(0, sizeof(alphanum) - 2)(gen)];
        }
        return result;
    }
};

// 操作类型
enum class OperationType {
    CREATE_FILE,
    READ_FILE,
    WRITE_FILE,
    DELETE_FILE,
    CREATE_DIR,
    DELETE_DIR,
    RENAME_FILE,
    RENAME_DIR,
    LIST_DIR
};

// 全局变量
std::atomic<int> total_operations_completed(0);
std::atomic<int> failed_operations(0);
std::mutex cout_mutex;
std::mutex operation_mutex;
std::condition_variable operation_cv;
RandomGenerator random_gen;

// 创建随机文件
bool create_random_file(const std::string& path, int size) {
    try {
        std::ofstream file(path);
        if (!file.is_open()) {
            return false;
        }
        std::string content = random_gen.getRandomString(size);
        file << content;
        file.close();
        return true;
    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << "创建文件失败: " << path << " - " << e.what() << std::endl;
        return false;
    }
}

// 读取文件
bool read_file(const std::string& path) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            return false;
        }
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        return true;
    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << "读取文件失败: " << path << " - " << e.what() << std::endl;
        return false;
    }
}

// 写入文件
bool write_file(const std::string& path, int size) {
    try {
        std::ofstream file(path, std::ios::app);
        if (!file.is_open()) {
            return false;
        }
        std::string content = random_gen.getRandomString(size);
        file << content;
        file.close();
        return true;
    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << "写入文件失败: " << path << " - " << e.what() << std::endl;
        return false;
    }
}

// 删除文件
bool delete_file(const std::string& path) {
    try {
        return fs::remove(path);
    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << "删除文件失败: " << path << " - " << e.what() << std::endl;
        return false;
    }
}

// 创建目录
bool create_directory(const std::string& path) {
    try {
        return fs::create_directory(path);
    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << "创建目录失败: " << path << " - " << e.what() << std::endl;
        return false;
    }
}

// 删除目录
bool delete_directory(const std::string& path) {
    try {
        return fs::remove(path);
    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << "删除目录失败: " << path << " - " << e.what() << std::endl;
        return false;
    }
}

// 重命名文件
bool rename_file(const std::string& old_path, const std::string& new_path) {
    try {
        fs::rename(old_path, new_path);
        return true;
    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << "重命名文件失败: " << old_path << " -> " << new_path << " - " << e.what() << std::endl;
        return false;
    }
}

// 列出目录内容
bool list_directory(const std::string& path) {
    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            // 只需遍历，不需要做任何事情
        }
        return true;
    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << "列出目录失败: " << path << " - " << e.what() << std::endl;
        return false;
    }
}

// 执行随机操作
void perform_random_operation(int thread_id) {
    // 创建线程专用目录
    std::string thread_dir = MOUNT_POINT + "/thread_" + std::to_string(thread_id);
    std::unique_lock<std::mutex> lock(operation_mutex);
    create_directory(thread_dir);
    operation_cv.notify_one();
    lock.unlock();

    for (int i = 0; i < NUM_OPERATIONS; i++) {
        // 选择随机操作
        int op = random_gen.getRandomInt(0, 8);
        OperationType operation = static_cast<OperationType>(op);

        bool success = false;
        std::string file_path = thread_dir + "/file_" + std::to_string(random_gen.getRandomInt(0, 9)) + ".txt";
        std::string dir_path = thread_dir + "/dir_" + std::to_string(random_gen.getRandomInt(0, 9));

        switch (operation) {
            case OperationType::CREATE_FILE: {
                int size = random_gen.getRandomInt(10, MAX_FILE_SIZE);
                success = create_random_file(file_path, size);
                break;
            }
            case OperationType::READ_FILE: {
                success = read_file(file_path);
                break;
            }
            case OperationType::WRITE_FILE: {
                int size = random_gen.getRandomInt(10, 1024);
                success = write_file(file_path, size);
                break;
            }
            case OperationType::DELETE_FILE: {
                success = delete_file(file_path);
                break;
            }
            case OperationType::CREATE_DIR: {
                success = create_directory(dir_path);
                break;
            }
            case OperationType::DELETE_DIR: {
                success = delete_directory(dir_path);
                break;
            }
            case OperationType::RENAME_FILE: {
                std::string new_file_path = thread_dir + "/renamed_file_" + std::to_string(random_gen.getRandomInt(0, 9)) + ".txt";
                success = rename_file(file_path, new_file_path);
                break;
            }
            case OperationType::RENAME_DIR: {
                std::string new_dir_path = thread_dir + "/renamed_dir_" + std::to_string(random_gen.getRandomInt(0, 9));
                success = rename_file(dir_path, new_dir_path);
                break;
            }
            case OperationType::LIST_DIR: {
                success = list_directory(thread_dir);
                break;
            }
        }

        total_operations_completed++;
        if (!success) {
            failed_operations++;
        }
    }

    // 清理线程目录
    try {
        fs::remove_all(thread_dir);
    } catch (...) {
        // 忽略清理错误
    }
}

// 运行压力测试
void run_stress_test() {
    std::cout << "=== 开始压力测试 ===" << std::endl;
    std::cout << "线程数: " << NUM_THREADS << std::endl;
    std::cout << "每线程操作数: " << NUM_OPERATIONS << std::endl;

    // 创建挂载点目录
    fs::create_directories(MOUNT_POINT);

    // 启动计时器
    auto start_time = std::chrono::high_resolution_clock::now();

    // 创建并启动线程
    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back(perform_random_operation, i);
    }

    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }

    // 停止计时器
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    // 输出结果
    std::cout << "压力测试完成" << std::endl;
    std::cout << "总操作数: " << total_operations_completed << std::endl;
    std::cout << "失败操作数: " << failed_operations << std::endl;
    std::cout << "成功率: " << (100.0 - (failed_operations * 100.0 / total_operations_completed)) << "%" << std::endl;
    std::cout << "总耗时: " << duration / 1000.0 << " 秒" << std::endl;
    std::cout << "每秒操作数: " << (total_operations_completed * 1000.0 / duration) << std::endl;
}

int main() {
    run_stress_test();
    return 0;
}