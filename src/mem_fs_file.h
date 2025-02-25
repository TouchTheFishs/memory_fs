#ifndef MEM_FS_FILE_H
#define MEM_FS_FILE_H
#include <cstdint>
#include <set>
#include <stdint.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
struct MemoryFileData {
};
struct MemoryFile {
	std::string name;
	char* data = nullptr;
	uint64_t size = 0;
	uint64_t data_size = 0;
	mode_t mode = S_IFREG | 0111;
	time_t ctime = 0;
	time_t mtime = 0;
	time_t atime = 0;
	bool is_init = false;
	std::set<std::string>* children;
};

struct Fd {
	bool used;
	mode_t mode;
	MemoryFile* file;
};
#endif	//MEM_FS_FILE_H