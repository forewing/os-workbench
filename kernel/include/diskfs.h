#ifndef __DISKFS_H__
#define __DISKFS_H__

#include <common.h>
#include <kmt.h>

#define SECTOR_SIZE 512

typedef struct disk_info diskinfo_t;
typedef struct diskfs_info diskfs_t;
typedef union diskfs_node disknode_t;
typedef struct disk_inode diskinode_t;
typedef char sector_t[SECTOR_SIZE];
typedef sector_t* disk_t;

enum {
    DISKNODE_FREE,
    DISKNODE_FILE,
    DISKNODE_DIR,
};

//====================
// DISK: sec0 | sec1 | sec....
// disk info | valid table sec0,1,2... | data sec0,1,2...
//====================

struct disk_info {
    char name[32];
    int secSz;
    int secNr;
    int dataSt;
    int root;
};

struct diskfs_info {
    diskinfo_t* info;
    int size;
    int dataNr;
    spinlock_t lock;
};

union diskfs_node {
    sector_t raw;
    struct {
        int type;
        int size;
        char name[32];
        int data[];  // For file, the parts; for dir, the children.
    };
};

struct disk_inode {
    int sec;
};

filesystem_t* mkfs(device_t* dev, const char* name);

#endif