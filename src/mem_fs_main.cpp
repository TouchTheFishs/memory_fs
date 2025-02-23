#define FUSE_USE_VERSION 31
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <fuse3/fuse.h>

#include <map>
#include <string>

#include "mem_fs_file.h"
using namespace std;

std::map<std::string, MemoryFile> files;

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

static void stat_by_path(const std::string& path, struct stat* stbuf)
{
	printf("stat %s\n", path.c_str());
	auto file = files.find(path);
	if (file == files.end()) {
		stbuf->st_nlink = 0;
		return;
	}
	memset(stbuf, 0, sizeof(struct stat));
	stbuf->st_size = file->second.size;
	stbuf->st_mode = file->second.mode;
	stbuf->st_ctime = file->second.ctime;
	stbuf->st_mtime = file->second.mtime;
	stbuf->st_nlink = S_ISDIR(stbuf->st_mode) ? 2 : 1;
	stbuf->st_size = S_ISDIR(stbuf->st_mode) ? 4096 : file->second.size;
}

static int memfs_getattr(const char* path, struct stat* stbuf, struct fuse_file_info* fi)
{
	(void)fi;
	stat_by_path(path, stbuf);
	if (stbuf->st_nlink == 0) {
		printf("getattr not found\n");
		return -ENOENT;
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
	auto parent = files.find(find_parent_dir(path));
	if (parent == files.end()) {
		return -ENOENT;
	}
	if (parent->second.children == nullptr) {
		parent->second.children = new std::set<std::string>();
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
	parent->second.children->insert(new_dir.name);
	return 0;
}

static int memfs_rmdir(const char* path)
{
	printf("rmdir %s\n", path);
	auto dir = files.find(path);
	if (dir == files.end()) {
		return -ENOENT;
	}

	if (dir->second.children != nullptr && dir->second.children->size() > 0) {
		return -ENOTEMPTY;
	}

	auto dir_parent = files.find(find_parent_dir(path));
	if (dir_parent == files.end()) {
		return -ENOENT;
	}

	if (dir_parent->second.children != nullptr) {
		auto it = dir_parent->second.children->find(dir->second.name);
		if (it != dir_parent->second.children->end()) {
			dir_parent->second.children->erase(it);
		}
	}
	if (dir->second.data != nullptr) {
		delete dir->second.data;
	}
	files.erase(path);
	return 0;
}

static int memfs_rename(const char* from, const char* to, unsigned int flags)
{
	printf("rename %s to %s\n", from, to);
	auto src_file = files.find(from);
	if (src_file == files.end()) {
		return -ENOENT;
	}
	auto dst_file = files.find(to);
	if (dst_file != files.end()) {
		return -EEXIST;
	}
	auto dst_parent = files.find(find_parent_dir(to));
	if (dst_parent == files.end()) {
		return -ENOENT;
	}

	auto src_parent = files.find(find_parent_dir(from));
	if (src_parent == files.end()) {
		return -ENOENT;
	}

	src_file->second.name = get_name_from_path(to);

	if (src_parent->second.children != nullptr) {
		auto it = src_parent->second.children->find(get_name_from_path(from));
		if (it != src_parent->second.children->end()) {
			src_parent->second.children->erase(it);
		}
	}

	if (dst_parent->second.children == nullptr) {
		dst_parent->second.children = new std::set<std::string>();
	}

	dst_parent->second.children->insert(src_file->second.name);

	files[to] = src_file->second;
	files.erase(from);
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
	.rmdir = memfs_rmdir,
	.rename = memfs_rename,
	.readdir = memfs_readdir,
	.init = memfs_init,
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