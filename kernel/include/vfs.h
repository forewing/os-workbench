#ifndef __VFS_H__
#define __VFS_H__

#include <common.h>

#define NOFILE 32

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define MODE_R 1
#define MODE_W 2

typedef struct file {
    const char* fspath;
    uint64_t offset;
    filesystem_t* fs;
    void* data;
} file_t;

typedef struct filesystem {
    const char* name;
    device_t* dev;
    void* data;
    int (*open)(file_t* file, int flag);
    int (*close)(file_t* file);
    ssize_t (*read)(file_t* file, char* buf, size_t size);
    ssize_t (*write)(file_t* file, const char* buf, size_t size);
    off_t (*lseek)(file_t* file, off_t offset, int whence);
    int (*mkdir)(filesystem_t* fs, const char* path);
    int (*rmdir)(filesystem_t* fs, const char* path);
} filesystem_t;

int parse_path(const char* str);

#endif