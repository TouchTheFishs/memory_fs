#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>

#include <map>
#include <string>

#include "mem_fs_file.h"
using namespace std;

std::map<std::string, MemoryFile> files;

static std::string find_parent_dir(const std::string& path)
{
	size_t last_slash = path.find_last_of('/');
	return path.substr(0, last_slash);
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
	(void)offset;  // 明确标记未使用（避免编译器警告）
	(void)fi;

	printf("readdir %s\n", path);

	if (string(path) != "/") {
		printf("readdir root\n");
		// 父目录的内容
		struct stat stbuf;
		stat_by_path(find_parent_dir(path), &stbuf);
		filler(buf, "..", &stbuf, 0, static_cast<fuse_fill_dir_flags>(0));
	}

	// 当前目录
	struct stat stbuf;
	stat_by_path(path, &stbuf);
	filler(buf, ".", &stbuf, 0, static_cast<fuse_fill_dir_flags>(0));

	auto dir = files.find(path);
	if (dir == files.end()) {
		return -ENOENT;
	}
	if (dir->second.children == nullptr) {
		return 0;
	}

	for (const auto& child : *dir->second.children) {
		for (const auto& file : files) {
			if (child == file.second.ino) {
				struct stat* stbuf = new struct stat;
				stat_by_path(path, stbuf);
				filler(buf, file.first.c_str(), stbuf, 0, static_cast<fuse_fill_dir_flags>(0));
			}
		}
	}
	return 0;
}

static int memfs_mkdir(const char* path, mode_t mode)
{
	auto parent = files.find(find_parent_dir(path));
	if (parent == files.end()) {
		return -ENOENT;
	}
	if (parent->second.children == nullptr) {
		parent->second.children = new std::set<uint64_t>();
	}
	MemoryFile new_dir;
	new_dir.name = path;
	new_dir.mode = S_IFDIR | mode;
	new_dir.size = 4096;
	new_dir.mtime = time(nullptr);
	new_dir.ctime = new_dir.mtime;
	new_dir.ino = new_dir.ctime;
	new_dir.children = nullptr;
	files[path] = new_dir;
	parent->second.children->insert(new_dir.ino);
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
	files["/"] = root;
	// 启动 FUSE
	return fuse_main(argc, argv, &memfs_ops, nullptr);
}