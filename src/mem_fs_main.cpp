#include <cerrno>
#include <cstdint>
#include <fcntl.h>
#include <vector>
#define FUSE_USE_VERSION 31
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <fuse3/fuse.h>
#include <map>

#include "mem_fs_file.h"

using namespace std;

std::map<std::string, MemoryFile> files;
std::vector<Fd> fd_vec;

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

static MemoryFile* get_file_by_path(const std::string& path)
{
	auto file = files.find(path);
	if (file == files.end()) {
		return nullptr;
	}
	return &file->second;
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
	printf("stat %s\n", path.c_str());
	auto file = get_file_by_path(path);
	if (file == nullptr) {
		return -ENOENT;
	}
	stat_by_file(file, stbuf);
	return 0;
}

static int32_t init_fd(const std::string& path, const mode_t mode, struct MemoryFile* file)
{
	for (size_t i = 0; i < fd_vec.size(); i++) {
		if (fd_vec[i].used == false) {
			fd_vec[i].used = true;
			fd_vec[i].mode = mode;
			fd_vec[i].file = file;
			return i;
		}
	}
	Fd fd;
	fd.used = true;
	fd.mode = mode;
	fd.file = file;
	fd_vec.push_back(fd);
	return fd_vec.size() - 1;
}

static int memfs_getattr(const char* path, struct stat* stbuf, struct fuse_file_info* fi)
{
	(void)fi;
	int32_t ret = stat_by_path(path, stbuf);
	if (ret != 0) {
		printf("stat fail, ret is %d\n", ret);
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
	printf("readdir %s\n", path);
	if (string(path) != "/") {
		// 父目录的内容
		filler(buf, "..", nullptr, 0, static_cast<fuse_fill_dir_flags>(0));
	} else {
		printf("readdir root\n");
	}
	// 当前目录
	filler(buf, ".", nullptr, 0, static_cast<fuse_fill_dir_flags>(0));
	auto dir = files.find(path);
	if (dir == files.end()) {
		return -ENOENT;
	}
	if (dir->second.children == nullptr) {
		return 0;
	}
	for (const auto& child_name : *dir->second.children) {
		string child_path;
		if (string(path).find_last_of('/') == string(path).length() - 1) {
			child_path = string(path) + child_name;
		} else {
			child_path = string(path) + "/" + child_name;
		}

		printf("readdir child path is %s\n", child_path.c_str());
		for (const auto& file : files) {
			if (child_path == file.first) {
				filler(buf, child_name.c_str(), nullptr, 0, static_cast<fuse_fill_dir_flags>(0));
			}
		}
	}
	return 0;
}

static int memfs_mkdir(const char* path, mode_t mode)
{
	printf("mkdir %s\n", path);
	auto parent = get_file_by_path(find_parent_dir(path));
	if (parent == nullptr) {
		return -ENOENT;
	}
	if (parent->children == nullptr) {
		parent->children = new std::set<std::string>();
	}
	string dir_name = get_name_from_path(path);
	printf("[warn] dir name is %s\n", dir_name.c_str());
	MemoryFile new_dir;
	new_dir.name = dir_name;
	new_dir.mode = S_IFDIR | mode;
	new_dir.size = 4096;
	new_dir.mtime = time(nullptr);
	new_dir.ctime = new_dir.mtime;
	new_dir.children = nullptr;
	files[path] = new_dir;
	parent->children->insert(new_dir.name);
	return 0;
}

static int memfs_rmdir(const char* path)
{
	printf("rmdir %s\n", path);
	auto dir = get_file_by_path(path);
	if (dir == nullptr) {
		return -ENOENT;
	}
	if (dir->children != nullptr && dir->children->size() > 0) {
		return -ENOTEMPTY;
	}

	auto dir_parent = files.find(find_parent_dir(path));
	if (dir_parent == files.end()) {
		return -ENOENT;
	}

	if (dir_parent->second.children != nullptr) {
		auto it = dir_parent->second.children->find(dir->name);
		if (it != dir_parent->second.children->end()) {
			dir_parent->second.children->erase(it);
		}
	}

	if (dir->data != nullptr) {
		delete dir->data;
	}
	files.erase(path);
	return 0;
}

static int memfs_rename(const char* from, const char* to, unsigned int flags)
{
	printf("rename %s to %s\n", from, to);
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

	files[to] = *src_file;
	files.erase(from);
	return 0;
}

static int memfs_open(const char* path, struct fuse_file_info* fi)
{
	printf("open %s\n", path);

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
		files[path] = *file;
	}

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
	printf("create %s\n", path);
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
		files[path] = *file;
	}
	return 0;
}

static int memfs_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi)
{
	printf("read %s\n", path);
	if (fi->fh == -1 || fi->fh >= fd_vec.size() || fd_vec[fi->fh].file == nullptr) {
		return -EBADF;
	}
	auto file = fd_vec[fi->fh].file;
	if (file == nullptr) {
		return -EBADF;
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
	printf("write %s\n", path);
	if (fi->fh == -1 || fi->fh >= fd_vec.size() || fd_vec[fi->fh].file == nullptr) {
		return -EBADF;
	}
	auto file = fd_vec[fi->fh].file;
	if (file == nullptr) {
		return -EBADF;
	}
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
	//打印时间戳
	printf("utimens %s\n", path);
	printf("utimens atime: %ld.%ld\n", ts[0].tv_sec, ts[0].tv_nsec);
	printf("utimens mtime: %ld.%ld\n", ts[1].tv_sec, ts[1].tv_nsec);
	auto file = get_file_by_path(path);
	if (file == nullptr) {
		return -ENOENT;
	}
	file->atime = ts[0].tv_sec;
	file->mtime = ts[1].tv_sec;
	return 0;
}

static int memfs_flush(const char* path, struct fuse_file_info* fi)
{
	(void)fi;
	printf("flush %s\n", path);
	return 0;
}

static int memfs_release(const char* path, struct fuse_file_info* fi)
{
	printf("release %s\n", path);
	if (fi->fh == -1 || fi->fh >= fd_vec.size() || fd_vec[fi->fh].file == nullptr) {
		return -EBADF;
	}
	fd_vec[fi->fh].used = false;
	fi->fh = -1;
	return 0;
}

static int memfs_unlink(const char* path)
{
	printf("unlink %s\n", path);
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
	files.erase(path);
	return 0;
}

static void* memfs_init(struct fuse_conn_info* conn, struct fuse_config* cfg)
{
	(void)conn;
	printf("memfs_init\n");
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

int main(int argc, char* argv[])
{
	// 初始化根目录
	MemoryFile root;
	root.name = "/";
	root.mode = S_IFDIR | 0755;
	root.mtime = time(nullptr);
	root.ctime = root.mtime;
	root.children = nullptr;
	files["/"] = root;
	// 启动 FUSE
	return fuse_main(argc, argv, &memfs_ops, nullptr);
}