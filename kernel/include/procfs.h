#ifndef __PROCFS_H__
#define __PROCFS_H__

#include <common.h>

enum {
    PROCFS_PROC,
    PROCFS_MEMINFO,
    PROCFS_CPUINFO,
};

typedef struct procfs_info {
    int type;
    uint32_t pid;
} procfs_t;

#endif