#include <cerrno>
#include <cstdint>
#include <fcntl.h>
#include <fstream>
#include <mutex>
#include <shared_mutex>
#include <vector>
#define FUSE_USE_VERSION 31
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <fuse3/fuse.h>
#include <filesystem>
#include <iostream>
#include <map>

#include "mem_fs_file.h"
#include "log_utils.h"
#include "timer.h"

using namespace std;
namespace fs = std::filesystem;

std::map<std::string, MemoryFile*> files;
std::shared_mutex rw_mutex;
std::vector<Fd> fd_vec;
string real_path_perfix;

static string get_real_path(const std::string& path)
{
	return real_path_perfix + path;
}

static string get_relative_path(const std::string& path)
{
	if (path.find(real_path_perfix) == 0) {
		return path.substr(real_path_perfix.length());
	}
	return path;
}

static std::string find_parent_dir(const std::string& path)
{
	size_t last_slash = path.find_last_of('/');
	if (last_slash == std::string::npos || last_slash == 0) {
		return "/";
	}
	return path.substr(0, last_slash);
}

static std::string get_name_from_path(const std::string& path)
{
	size_t last_slash = path.find_last_of('/');
	if (last_slash == std::string::npos) {
		return path;
	}
	if (last_slash == path.length() - 1) {
		size_t prev_slash = path.find_last_of('/', last_slash - 1);
		return path.substr(prev_slash + 1, last_slash - prev_slash - 1);
	}
	return path.substr(last_slash + 1);
}

static MemoryFile* get_file_by_path_with_on_lock(const std::string& path)
{
	auto file = files.find(path);
	if (file == files.end()) {
		return nullptr;
	}
	return file->second;
}

static MemoryFile* get_file_by_path(const std::string& path)
{
	std::shared_lock<std::shared_mutex> lock(rw_mutex);
	return get_file_by_path_with_on_lock(path);
}

static void stat_by_file(MemoryFile* file, struct stat* stbuf)
{
	memset(stbuf, 0, sizeof(struct stat));
	stbuf->st_size = file->size;
	stbuf->st_mode = file->mode;
	stbuf->st_ctime = file->ctime;
	stbuf->st_mtime = file->mtime;
	stbuf->st_nlink = S_ISDIR(stbuf->st_mode) ? 2 : 1;
	stbuf->st_size = S_ISDIR(stbuf->st_mode) ? 4096 : file->size;
}

static int32_t stat_by_path(const std::string& path, struct stat* stbuf)
{
	LOGD("stat %s\n", path.c_str());
	auto file = get_file_by_path(path);
	if (file == nullptr) {
		return -ENOENT;
	}
	stat_by_file(file, stbuf);
	return 0;
}

static int32_t init_fd(const std::string& path, const mode_t mode, struct MemoryFile* file)
{
	if (file == nullptr) {
		LOGE("init fd failed, file is nullptr\n");
		return -1;
	}
	for (size_t i = 0; i < fd_vec.size(); i++) {
		if (fd_vec[i].used == false) {
			fd_vec[i].used = true;
			fd_vec[i].mode = mode;
			fd_vec[i].file = file;
			LOGD("init fd success, fd is %zu\n", i);
			return i;
		}
	}
	Fd fd;
	fd.used = true;
	fd.mode = mode;
	fd.file = file;
	fd_vec.push_back(fd);
	LOGD("init fd success, fd is %zu\n", fd_vec.size() - 1);
	return fd_vec.size() - 1;
}

static uint64_t read_file_to_memory(const std::string& path, char*& data)
{
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file) {
		LOGE("Failed to open file: %s\n", path.c_str());
		return 0;
	}
	LOGD("read file, file size: %zu\n", file.tellg());
	uint64_t size = file.tellg();
	LOGI("read file, file size: %zu\n", size);
	data = new char[size + 1];
	file.seekg(0, std::ios::beg);
	file.read(data, size);
	file.close();
	data[size] = '\0';
	return size;
}

static int32_t init_local_files_to_fs(const std::string& real_path, const std::string& relative_path)
{
	LOGI("init local file to fs, relative path is %s, real path is %s\n", relative_path.c_str(), real_path.c_str());
	auto dir = get_file_by_path_with_on_lock(relative_path);
	if (dir == nullptr) {
		LOGE("Failed to find relative dir: %s\n", relative_path.c_str());
		return -ENOENT;
	}
	if (!fs::exists(real_path)) {
		LOGE("Failed to find local dir: %s\n", real_path.c_str());
		return -ENOENT;
	}

	if (!fs::is_directory(real_path)) {
		LOGE("local is not dir: %s\n", real_path.c_str());
		return -ENOTDIR;
	}
	auto parent_dir = fs::directory_iterator(real_path);
	if (dir->children == nullptr) {
		dir->children = new std::set<std::string>();
	}
	struct stat statbuf;
	for (auto& file : parent_dir) {
		LOGD("file path: %s\n", file.path().c_str());
		MemoryFile* file_ptr = new MemoryFile();
		stat(file.path().c_str(), &statbuf);
		file_ptr->name = file.path().filename().string();
		file_ptr->mode = statbuf.st_mode;
		file_ptr->mtime = statbuf.st_mtime;
		file_ptr->ctime = statbuf.st_ctime;
		file_ptr->atime = statbuf.st_atime;
		if (!(file_ptr->mode & S_IFDIR)) {
			file_ptr->size = statbuf.st_size;
			read_file_to_memory(file.path().string(), file_ptr->data);
		} else {
			file_ptr->size = 4096;
		}
		string file_relative_path;
		if (relative_path.find_last_of('/') == relative_path.length() - 1) {
			file_relative_path = relative_path + file_ptr->name;
		} else {
			file_relative_path = relative_path + "/" + file_ptr->name;
		}
		dir->children->insert(file_ptr->name);
		// not need lock for rw_mutex
		files[file_relative_path] = std::move(file_ptr);
		LOGD("init file to fs success, file path is %s\n", file_relative_path.c_str());
	}
	return 0;
}

static int memfs_getattr(const char* path, struct stat* stbuf, struct fuse_file_info* fi)
{
	(void)fi;
	int32_t ret = stat_by_path(path, stbuf);
	if (ret != 0) {
		LOGE("stat fail, ret is %d\n", ret);
		return ret;
	}
	return 0;
}

static int memfs_readdir(const char* path,
						 void* buf,
						 fuse_fill_dir_t filler,
						 off_t offset,
						 struct fuse_file_info* fi,
						 fuse_readdir_flags flags)
{
	(void)offset;
	(void)fi;
	LOGD("readdir %s\n", path);
	if (string(path) != "/") {
		filler(buf, "..", nullptr, 0, static_cast<fuse_fill_dir_flags>(0));
	}
	filler(buf, ".", nullptr, 0, static_cast<fuse_fill_dir_flags>(0));
	auto dir = get_file_by_path(path);
	if (dir == nullptr) {
		return -ENOENT;
	}
	if (dir->children == nullptr) {
		return 0;
	}
	std::vector<string> find_names;
	for (const auto& child_name : *dir->children) {
		string child_path;
		if (string(path).find_last_of('/') == string(path).length() - 1) {
			child_path = string(path) + child_name;
		} else {
			child_path = string(path) + "/" + child_name;
		}
		LOGD("readdir child path is %s\n", child_path.c_str());
		{
			std::shared_lock<std::shared_mutex> lock(rw_mutex);
			for (const auto& file : files) {
				if (child_path == file.first) {
					unique_lock<std::shared_mutex> lock(file.second->rw_mutex);
					if ((file.second->mode & S_IFDIR) && file.second->is_init == false) {
						auto real_path = get_real_path(child_path);
						file.second->is_init = true;
						init_local_files_to_fs(real_path, child_path);
					}
					find_names.push_back(file.second->name);
					break;
				}
			}
		}
	}
	for (const auto& name : find_names) {
		LOGD("readdir file path is %s\n", name.c_str());
		filler(buf, name.c_str(), nullptr, 0, static_cast<fuse_fill_dir_flags>(0));
	}
	if (find_names.size() != dir->children->size()) {
		LOGE("find count is not equal to children size\n");
		return -EIO;
	}
	return 0;
}

static int memfs_mkdir(const char* path, mode_t mode)
{
	LOGD("mkdir %s\n", path);
	auto parent = get_file_by_path(find_parent_dir(path));
	if (parent == nullptr) {
		return -ENOENT;
	}
	if (parent->children == nullptr) {
		parent->children = new std::set<std::string>();
	}
	string dir_name = get_name_from_path(path);
	MemoryFile* new_dir = new MemoryFile();
	new_dir->name = dir_name;
	new_dir->mode = S_IFDIR | mode;
	new_dir->size = 4096;
	new_dir->mtime = time(nullptr);
	new_dir->ctime = new_dir->mtime;
	new_dir->children = nullptr;
	parent->children->insert(new_dir->name);
	unique_lock<std::shared_mutex> lock(rw_mutex);
	files[path] = std::move(new_dir);
	return 0;
}

static int memfs_rmdir(const char* path)
{
	LOGD("rmdir %s\n", path);
	auto dir = get_file_by_path(path);
	if (dir == nullptr) {
		return -ENOENT;
	}
	if (dir->children != nullptr && dir->children->size() > 0) {
		return -ENOTEMPTY;
	}

	auto dir_parent = get_file_by_path(find_parent_dir(path));
	if (dir_parent == nullptr) {
		return -ENOENT;
	}

	if (dir_parent->children != nullptr) {
		auto it = dir_parent->children->find(dir->name);
		if (it != dir_parent->children->end()) {
			dir_parent->children->erase(it);
		}
	}

	if (dir->data != nullptr) {
		delete dir->data;
	}
	unique_lock<std::shared_mutex> lock(rw_mutex);
	files.erase(path);
	return 0;
}

static int memfs_rename(const char* from, const char* to, unsigned int flags)
{
	LOGD("rename %s to %s\n", from, to);
	auto src_file = get_file_by_path(from);
	if (src_file == nullptr) {
		return -ENOENT;
	}
	auto dst_file = get_file_by_path(to);
	if (dst_file != nullptr) {
		return -EEXIST;
	}
	auto dst_parent = get_file_by_path(find_parent_dir(to));
	if (dst_parent == nullptr) {
		return -ENOENT;
	}

	auto src_parent = get_file_by_path(find_parent_dir(from));
	if (src_parent == nullptr) {
		return -ENOENT;
	}

	src_file->name = get_name_from_path(to);

	if (src_parent->children != nullptr) {
		auto it = src_parent->children->find(get_name_from_path(from));
		if (it != src_parent->children->end()) {
			src_parent->children->erase(it);
		}
	}

	if (dst_parent->children == nullptr) {
		dst_parent->children = new std::set<std::string>();
	}

	dst_parent->children->insert(src_file->name);
	unique_lock<std::shared_mutex> lock(rw_mutex);
	files[to] = std::move(src_file);
	files.erase(from);
	return 0;
}

static int memfs_open(const char* path, struct fuse_file_info* fi)
{
	LOGD("open %s\n", path);

	auto file = get_file_by_path(path);
	if (file == nullptr && fi->flags & O_CREAT) {
		file = new MemoryFile();
		file->name = get_name_from_path(path);
		file->mode = S_IFREG | 0644;
		file->ctime = time(nullptr);
		file->mtime = file->ctime;
		file->children = nullptr;
		auto parent_dir = get_file_by_path(find_parent_dir(path));
		if (parent_dir == nullptr) {
			return -ENOENT;
		}
		if (parent_dir->children == nullptr) {
			parent_dir->children = new std::set<std::string>();
		}
		parent_dir->children->insert(file->name);
		unique_lock<std::shared_mutex> lock(rw_mutex);
		files[path] = std::move(file);
	}

	shared_lock<shared_mutex> lock(file->rw_mutex);
	struct stat stbuf;
	stat_by_file(file, &stbuf);

	int access_mode = fi->flags & O_ACCMODE;

	if (S_IFDIR & (stbuf.st_mode)) {
		if (access_mode != O_RDONLY) {
			return -EACCES;
		}
		return 0;
	}

	if (access_mode == O_RDONLY) {
		if (!(stbuf.st_mode & S_IRUSR)) {
			return -EACCES;
		}
	} else if (access_mode == O_WRONLY || access_mode == O_RDWR) {
		if (!(stbuf.st_mode & S_IWUSR)) {
			return -EACCES;
		}
	} else if (access_mode == O_RDWR) {
		if (!(stbuf.st_mode & S_IWUSR) || !(stbuf.st_mode & S_IRUSR)) {
			return -EACCES;
		}
	}
	fi->fh = init_fd(path, fi->flags, file);
	return 0;
}

static int memfs_create(const char* path, mode_t mode, struct fuse_file_info* fi)
{
	LOGD("create %s\n", path);
	auto file = get_file_by_path(path);
	if (file == nullptr) {
		file = new MemoryFile();
		file->name = get_name_from_path(path);
		file->mode = S_IFREG | 0644;
		file->ctime = time(nullptr);
		file->mtime = file->ctime;
		file->children = nullptr;
		auto parent_dir = get_file_by_path(find_parent_dir(path));
		if (parent_dir == nullptr) {
			return -ENOENT;
		}
		if (parent_dir->children == nullptr) {
			parent_dir->children = new std::set<std::string>();
		}
		parent_dir->children->insert(file->name);
		unique_lock<std::shared_mutex> lock(rw_mutex);
		files[path] = std::move(file);
	}
	return 0;
}

static int memfs_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi)
{
	LOGD("read %s\n", path);
	if (fi->fh == -1 || fi->fh >= fd_vec.size() || fd_vec[fi->fh].file == nullptr) {
		return -EBADF;
	}
	MemoryFile* file = fd_vec[fi->fh].file;
	if (file == nullptr) {
		return -EBADF;
	}
	shared_lock<shared_mutex> lock(file->rw_mutex);
	if (file->size != 0 && file->data == nullptr) {
		return -EIO;
	}
	if (offset >= file->size) {
		return 0;
	}
	size_t bytes_to_read = std::min(size, file->size - offset);
	if (bytes_to_read > 0) {
		memcpy(buf, file->data + offset, bytes_to_read);
	}
	return bytes_to_read;
}

static int memfs_write(const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi)
{
	LOGD("write %s\n", path);
	if (fi->fh == -1 || fi->fh >= fd_vec.size() || fd_vec[fi->fh].file == nullptr) {
		return -EBADF;
	}
	MemoryFile* file = fd_vec[fi->fh].file;
	if (file == nullptr) {
		return -EBADF;
	}
	unique_lock<shared_mutex> lock(file->rw_mutex);
	if (file->data == nullptr) {
		file->data = new char[size + offset];
		memset(file->data, 0, sizeof(char) * (size + offset));
		file->data_size = size + offset;
	} else {
		if (offset + size > file->data_size) {
			uint64_t new_size = offset + size > file->data_size * 1.5 ? offset + size : file->data_size * 1.5;
			char* new_data = new char[new_size];
			memset(new_data, 0, sizeof(char) * new_size);
			std::memcpy(new_data, file->data, file->size);
			delete[] file->data;
			file->data = new_data;
			file->data_size = new_size;
			file->need_flush = true;
		}
	}
	if (offset + size > file->size) {
		file->size = offset + size;
	}
	std::memcpy(file->data + offset, buf, size);
	return size;
}

static int memfs_utimens(const char* path, const struct timespec ts[2], struct fuse_file_info* fi)
{
	(void)fi;
	LOGD("utimens %s\n", path);
	auto file = get_file_by_path(path);
	if (file == nullptr) {
		return -ENOENT;
	}
	unique_lock<std::shared_mutex> lock(file->rw_mutex);
	file->atime = ts[0].tv_sec;
	file->mtime = ts[1].tv_sec;
	return 0;
}

static int memfs_flush(const char* path, struct fuse_file_info* fi)
{
	(void)fi;
	LOGD("flush %s\n", path);
	return 0;
}

static int memfs_release(const char* path, struct fuse_file_info* fi)
{
	LOGD("release %s\n", path);
	if (fi->fh == -1 || fi->fh >= fd_vec.size() || fd_vec[fi->fh].file == nullptr) {
		return -EBADF;
	}
	fd_vec[fi->fh].used = false;
	fi->fh = -1;
	return 0;
}

static int memfs_unlink(const char* path)
{
	LOGD("unlink %s\n", path);
	auto file = get_file_by_path(path);
	if (file == nullptr) {
		return -ENOENT;
	}
	auto parent = get_file_by_path(find_parent_dir(path));
	if (parent == nullptr) {
		return -ENOENT;
	}
	if (parent->children != nullptr) {
		auto it = parent->children->find(file->name);
		if (it != parent->children->end()) {
			parent->children->erase(it);
		}
	}
	if (file->data != nullptr) {
		delete[] file->data;
	}
	unique_lock<std::shared_mutex> lock(rw_mutex);
	files.erase(path);
	return 0;
}

static void* memfs_init(struct fuse_conn_info* conn, struct fuse_config* cfg)
{
	(void)conn;
	LOGD("memfs_init\n");
	return nullptr;
}

// 实现 FUSE 操作
static struct fuse_operations memfs_ops = {
	.getattr = memfs_getattr,
	.mkdir = memfs_mkdir,
	.unlink = memfs_unlink,
	.rmdir = memfs_rmdir,
	.rename = memfs_rename,
	.open = memfs_open,
	.read = memfs_read,
	.write = memfs_write,
	.flush = memfs_flush,
	.release = memfs_release,
	.readdir = memfs_readdir,
	.init = memfs_init,
	.create = memfs_create,
	.utimens = memfs_utimens,
};

static void flush_files()
{
	LOGD("flush files\n");
	shared_lock<std::shared_mutex> lock(rw_mutex);
	for (const auto& file : files) {
		if (file.second->need_flush != false) {
			unique_lock<std::shared_mutex> lock(file.second->rw_mutex);
			if (file.second->data != nullptr) {
				std::ofstream out_file(get_real_path(file.first), std::ios::binary);
				if (!out_file) {
					LOGE("Failed to open file: %s\n", get_real_path(file.first).c_str());
					return;
				}
				out_file.write(file.second->data, file.second->size);
				out_file.close();
				file.second->need_flush = false;
				LOGD("flush file success, file path is %s\n", get_real_path(file.first).c_str());
			}
		}
	}
}

void handleOption(int& argc, char**& argv)
{
	int opt;
	int option_index = 0;
	std::vector<char*> fuse_argv;
	for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], "--fs_log") == 0 && i + 1 < argc) {
			set_log_level(argv[++i]);
		} else if (strcmp(argv[i], "--targe") == 0 && i + 1 < argc) {
			real_path_perfix = fs::absolute(argv[++i]);
		} else {
			fuse_argv.push_back(argv[i]);
		}
	}
	argc = fuse_argv.size();
	std::copy(fuse_argv.begin(), fuse_argv.end(), argv);
}

int main(int argc, char* argv[])
{
	setup_signal_handlers();
	handleOption(argc, argv);
	LOGI("real path is %s\n", real_path_perfix.c_str());
	if (real_path_perfix.find_last_of('/') == real_path_perfix.length() - 1) {
		real_path_perfix = real_path_perfix.substr(0, real_path_perfix.length() - 1);
	}
	LOGI("memfs start\n");
	MemoryFile* root = new MemoryFile();
	root->name = "/";
	root->mode = S_IFDIR | 0755;
	root->mtime = time(nullptr);
	root->ctime = root->mtime;
	root->children = nullptr;
	root->is_init = true;
	{
		unique_lock<std::shared_mutex> lock(rw_mutex);
		files["/"] = std::move(root);
		init_local_files_to_fs(real_path_perfix, "/");
	}
	Timer timer(flush_files, std::chrono::seconds(10));
	timer.start_timer();
	// 启动 FUSE
	return fuse_main(argc, argv, &memfs_ops, nullptr);
}