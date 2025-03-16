#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <cassert>
#include <cstring>
#include <vector>

namespace fs = std::filesystem;

// 测试配置
const std::string MOUNT_POINT = fs::absolute("../test/mount_point").string();
const std::string TARGET_DIR = fs::absolute("../test/target_dir").string();

// 文件操作RAII包装类
class FileGuard
{
  public:
	FileGuard(const std::string& path)
		: path_(path)
	{
	}
	~FileGuard()
	{
		if (fs::exists(path_))
			fs::remove(path_);
	}

  private:
	std::string path_;
};

// 测试辅助函数
void create_test_file(const std::string& path, const std::string& content)
{
	std::ofstream file(path);
	if (!file.is_open()) {
		throw std::runtime_error("无法创建文件: " + path);
	}
	file << content;
	file.close();
	if (!fs::exists(path)) {
		throw std::runtime_error("文件创建失败: " + path);
	}
}

bool verify_file_content(const std::string& path, const std::string& expected_content)
{
	std::ifstream file(path);
	if (!file.is_open()) {
		std::cerr << "无法打开文件: " << path << std::endl;
		return false;
	}

	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();

	if (content == expected_content) {
		return true;
	}
	std::cerr << "文件内容不匹配: " << path << std::endl;
	std::cerr << "期望内容: " << expected_content << std::endl;
	std::cerr << "实际内容: " << content << std::endl;
	return false;
}

// 测试基本文件操作
bool test_basic_file_operations()
{
	std::cout << "=== 测试基本文件操作 ===" << std::endl;

	try {
		// 测试创建文件
		std::string test_file = MOUNT_POINT + "/test_file.txt";
		FileGuard guard(test_file);
		std::string test_content = "这是测试内容";
		create_test_file(test_file, test_content);

		// 验证文件是否创建成功
		if (!fs::exists(test_file)) {
			std::cerr << "创建文件失败: " << test_file << std::endl;
			return false;
		}
		std::cout << "✓ 文件创建成功" << std::endl;

		// 验证文件内容
		if (!verify_file_content(test_file, test_content)) {
			std::cerr << "文件内容验证失败" << std::endl;
			return false;
		}
		std::cout << "✓ 文件内容验证成功" << std::endl;

		// 测试修改文件
		std::string updated_content = "这是更新后的内容";
		create_test_file(test_file, updated_content);

		// 验证文件内容是否更新
		if (!verify_file_content(test_file, updated_content)) {
			std::cerr << "文件内容更新验证失败" << std::endl;
			return false;
		}
		std::cout << "✓ 文件内容更新成功" << std::endl;

		// 测试删除文件
		fs::remove(test_file);
		if (fs::exists(test_file)) {
			std::cerr << "删除文件失败: " << test_file << std::endl;
			return false;
		}
		std::cout << "✓ 文件删除成功" << std::endl;

		return true;
	} catch (const std::exception& e) {
		std::cerr << "测试基本文件操作失败: " << e.what() << std::endl;
		return false;
	}
}

// 测试目录操作
bool test_directory_operations()
{
	std::cout << "=== 测试目录操作 ===" << std::endl;

	try {
		// 测试创建目录
		std::string test_dir = MOUNT_POINT + "/test_dir";
		if (!fs::create_directory(test_dir)) {
			throw std::runtime_error("无法创建目录: " + test_dir);
		}

		// 验证目录是否创建成功
		if (!fs::exists(test_dir)) {
			std::cerr << "创建目录失败: " << test_dir << std::endl;
			return false;
		}
		std::cout << "✓ 目录创建成功" << std::endl;

		// 测试在目录中创建文件
		std::string test_file = test_dir + "/test_file.txt";
		std::string test_content = "这是目录中的测试文件";
		create_test_file(test_file, test_content);

		// 验证文件是否创建成功
		if (!fs::exists(test_file)) {
			std::cerr << "在目录中创建文件失败: " << test_file << std::endl;
			return false;
		}
		std::cout << "✓ 在目录中创建文件成功" << std::endl;

		// 验证文件内容
		if (!verify_file_content(test_file, test_content)) {
			std::cerr << "目录中的文件内容验证失败" << std::endl;
			return false;
		}
		std::cout << "✓ 目录中的文件内容验证成功" << std::endl;

		// 测试删除目录中的文件
		fs::remove(test_file);
		if (fs::exists(test_file)) {
			std::cerr << "删除目录中的文件失败: " << test_file << std::endl;
			return false;
		}
		std::cout << "✓ 删除目录中的文件成功" << std::endl;

		// 测试删除目录
		fs::remove(test_dir);
		if (fs::exists(test_dir)) {
			std::cerr << "删除目录失败: " << test_dir << std::endl;
			return false;
		}
		std::cout << "✓ 删除目录成功" << std::endl;

		return true;
	} catch (const std::exception& e) {
		std::cerr << "测试目录操作失败: " << e.what() << std::endl;
		return false;
	}
}

// 测试文件重命名
bool test_rename_operations()
{
	std::cout << "=== 测试重命名操作 ===" << std::endl;

	try {
		// 创建测试文件
		std::string test_file = MOUNT_POINT + "/original.txt";
		std::string test_content = "这是原始文件";
		FileGuard guard(test_file);
		create_test_file(test_file, test_content);

		// 测试重命名文件
		std::string renamed_file = MOUNT_POINT + "/renamed.txt";
		fs::rename(test_file, renamed_file);

		// 验证重命名是否成功
		if (fs::exists(test_file) || !fs::exists(renamed_file)) {
			std::cerr << "重命名文件失败" << std::endl;
			return false;
		}
		std::cout << "✓ 文件重命名成功" << std::endl;

		// 验证重命名后的文件内容
		if (!verify_file_content(renamed_file, test_content)) {
			std::cerr << "重命名后的文件内容验证失败" << std::endl;
			return false;
		}
		std::cout << "✓ 重命名后的文件内容验证成功" << std::endl;

		// 清理
		fs::remove(renamed_file);

		return true;
	} catch (const std::exception& e) {
		std::cerr << "测试重命名操作失败: " << e.what() << std::endl;
		return false;
	}
}

// 测试大文件操作
bool test_large_file_operations()
{
	std::cout << "=== 测试大文件操作 ===" << std::endl;

	// 创建大文件
	std::string large_file = MOUNT_POINT + "/large_file.bin";
	const size_t file_size = 1024 * 1024;  // 1MB

	std::ofstream file(large_file, std::ios::binary);
	if (!file.is_open()) {
		std::cerr << "无法创建大文件: " << large_file << std::endl;
		return false;
	}

	// 生成随机数据
	std::vector<char> data(file_size);
	for (size_t i = 0; i < file_size; i++) {
		data[i] = static_cast<char>(i % 256);
	}

	// 写入数据
	file.write(data.data(), file_size);
	file.close();

	// 验证文件大小
	if (fs::file_size(large_file) != file_size) {
		std::cerr << "大文件大小验证失败" << std::endl;
		return false;
	}
	std::cout << "✓ 大文件创建成功" << std::endl;

	// 读取并验证数据
	std::ifstream read_file(large_file, std::ios::binary);
	if (!read_file.is_open()) {
		std::cerr << "无法打开大文件进行读取: " << large_file << std::endl;
		return false;
	}

	std::vector<char> read_data(file_size);
	read_file.read(read_data.data(), file_size);
	read_file.close();

	// 验证数据一致性
	bool data_match = true;
	for (size_t i = 0; i < file_size; i++) {
		if (data[i] != read_data[i]) {
			data_match = false;
			break;
		}
	}

	if (!data_match) {
		std::cerr << "大文件数据验证失败" << std::endl;
		return false;
	}
	std::cout << "✓ 大文件数据验证成功" << std::endl;

	// 清理
	fs::remove(large_file);

	return true;
}

// 主函数
int main()
{
	std::cout << "开始 memory_fs 功能测试" << std::endl;

	bool all_tests_passed = true;

	// 运行测试
	all_tests_passed &= test_basic_file_operations();
	all_tests_passed &= test_directory_operations();
	all_tests_passed &= test_rename_operations();
	all_tests_passed &= test_large_file_operations();

	if (all_tests_passed) {
		std::cout << "\n所有测试通过！" << std::endl;
		return 0;
	} else {
		std::cerr << "\n测试失败！" << std::endl;
		return 1;
	}
	return 0;
}