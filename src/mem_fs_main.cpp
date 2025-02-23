#include <cstring>
#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>

#include <map>
#include <string>

#include "mem_fs_file.h"
using namespace std;

std::map<std::string, MemoryFile> files;

static int memfs_getattr(const char* path, struct stat* stbuf, struct fuse_file_info* fi)
{
	(void)fi;
	auto file = files.find(path);
	if (file == files.end()) {
		return -ENOENT;
	}
	memset(stbuf, 0, sizeof(struct stat));
	stbuf->st_size = file->second.data->size;
	stbuf->st_mode = file->second.data->mode;
	stbuf->st_size = file->second.data->size;
	stbuf->st_ctime = file->second.data->ctime;
	stbuf->st_mtime = file->second.data->mtime;
	return 0;
}

// 实现 FUSE 操作
static struct fuse_operations memfs_ops = {.getattr = memfs_getattr};

int main(int argc, char* argv[])
{
	// 初始化根目录
	MemoryFile root;
	root.path = "/";
	MemoryFileData* root_data = new MemoryFileData;
	root_data->mode = S_IFDIR | 0755;
	root_data->mtime = time(nullptr);
	root_data->ctime = root_data->mtime;
	root.data = root_data;
	files["/"] = root;
	// 启动 FUSE
	return fuse_main(argc, argv, &memfs_ops, nullptr);
}