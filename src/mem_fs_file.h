#ifndef MEM_FS_FILE_H
#define MEM_FS_FILE_H
#include <stdint.h>
#include <string>
#include <sys/stat.h>
struct MemoryFileData {
    char* data = nullptr;
    uint64_t size = 0;
    mode_t mode = S_IFREG | 0111;
    time_t ctime = 0;
    time_t mtime = 0;
};
struct MemoryFile {
    std::string path;
    MemoryFileData *data;
};
#endif //MEM_FS_FILE_H