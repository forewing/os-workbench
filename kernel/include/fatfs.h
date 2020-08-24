/*

// DEPRECATED

#ifndef __FATFS_H__
#define __FATFS_H__

#include <common.h>
#include <devices.h>

typedef union fat_inode fatinode_t;
typedef union fat_dir fatdir_t;
typedef union fat_file fatfile_t;
typedef union fat_disk fatdisk_t;
typedef struct fat_inodeinfo fatiinfo_t;

#define SECTOR_SIZE 512
#define SECTOR_NR_RAW (RD_SIZE / SECTOR_SIZE)
#define INODE_NAME_SIZE (SECTOR_SIZE - sizeof(fatiinfo_t))
#define DIR_CHILD_NR ((SECTOR_SIZE - sizeof(fatdir_t*)) /
sizeof(fatinode_t*)) #define FILE_SIZE (SECTOR_SIZE - sizeof(fatfile_t*))
typedef char fatvalid_t[SECTOR_NR_RAW];
#define SECTOR_NR ((RD_SIZE - sizeof(fatvalid_t)) / SECTOR_SIZE)

enum {
    INODE_TYPE_FREE,
    INODE_TYPE_FILE,
    INODE_TYPE_DIR,
};

struct fat_inodeinfo {
    int flag;
    int size;
    int secs;
    void* ptr;
};

union fat_inode {
    char raw[SECTOR_SIZE];
    struct {
        char info[sizeof(fatiinfo_t)];
        char name[INODE_NAME_SIZE];
    };
};

union fat_dir {
    char raw[SECTOR_SIZE];
    struct {
        fatdir_t* next;
        fatinode_t* child[DIR_CHILD_NR];
    };
};

union fat_file {
    char raw[SECTOR_SIZE];
    struct {
        fatfile_t* next;
        char* data[FILE_SIZE];
    };
};

union fat_disk {
    char data[RD_SIZE];
    struct {
        fatvalid_t valid;
        char secs[SECTOR_NR][SECTOR_SIZE];
    };
};

#endif

*/