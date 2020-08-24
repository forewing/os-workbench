#include <common.h>
#include <klib.h>
#include <pmm.h>
#include <procfs.h>
#include <util.h>
#include <vfs.h>

static filesystem_t procfs;

static int procfs_open(file_t* file, int flag) {
    int ret = -1;
    int type = -1;
    uint32_t pid = -1;
    if (strlen(file->fspath) == 8) {
        if (strcmp(file->fspath, "/cpuinfo") == 0) {
            ret = 0;
            type = PROCFS_CPUINFO;
        } else if (strcmp(file->fspath, "/meminfo") == 0) {
            ret = 0;
            type = PROCFS_MEMINFO;
        }
    }
    if (ret != 0) {
        pid = util_s2u32(file->fspath + 1, 0);
        if (pid != (uint32_t)-1) {
            ret = 0;
            type = PROCFS_PROC;
        }
    }

    if (ret == 0) {
        file->data = pmm->alloc(sizeof(procfs_t));
        ((procfs_t*)file->data)->type = type;
        ((procfs_t*)file->data)->pid = pid;
    }

    return ret;
}

static int procfs_close(file_t* file) {
    pmm->free(file->data);
    return 0;
}

static char buffer[256];
static ssize_t procfs_read(file_t* file, char* buf, size_t size) {
    procfs_t* proc_info = (procfs_t*)file->data;
    int ret, used;
    switch (proc_info->type) {
        case PROCFS_CPUINFO:
            ret = sprintf(buffer,
                          "total cpus: %d\n"
                          "current cpu: %d\n"
                          "architecture: x86\n",
                          _ncpu(), _cpu());
            if (ret > size) {
                ret = -1;
            }
            strcpy(buf, buffer);
            break;

        case PROCFS_MEMINFO:
            used = total_mem - free_mem;
            ret = sprintf(buffer,
                          "total: %d\n"
                          "free: %d\n"
                          "used: %d\n",
                          total_mem, free_mem, used);
            if (ret > size) {
                ret = -1;
            }
            strcpy(buf, buffer);
            break;

        case PROCFS_PROC:
            ret = sprintf(buffer,
                          "cpu: %d\n"
                          "slot: %d\n",
                          proc_info->pid / MAX_TASK, proc_info->pid % MAX_TASK);
            if (ret > size) {
                ret = -1;
            }
            strcpy(buf, buffer);
            break;

        default:
            ret = -1;
            break;
    }
    return ret;
}

static ssize_t procfs_write(file_t* file, const char* buf, size_t size) {
    return -1;
}

static ssize_t procfs_lseek(file_t* file, off_t offset, int whence) {
    return -1;
}

int procfs_dir(filesystem_t* fs, const char* path) {
    return -1;
}

filesystem_t* procfs_init() {
    procfs.dev = NULL;

    procfs.name = "procfs";
    procfs.open = procfs_open;
    procfs.close = procfs_close;
    procfs.read = procfs_read;
    procfs.write = procfs_write;
    procfs.lseek = procfs_lseek;
    procfs.mkdir = procfs_dir;
    procfs.rmdir = procfs_dir;

    return &procfs;
}