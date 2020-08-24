#include <common.h>
#include <devices.h>
#include <diskfs.h>
#include <klib.h>
#include <kmt.h>
#include <util.h>

#define MOUNT_NR 256

#define FP(__FP_FD) task_current[_cpu()]->files[__FP_FD]

#define VALID_FD(__FD_NO)                                                \
    {                                                                    \
        if (__FD_NO < 0 || __FD_NO >= NOFILE) {                          \
            util_log("vfs: fd overflow.", 0, LOG_ERROR, LOG_NNONE);      \
            return -1;                                                   \
        }                                                                \
        if (FP(__FD_NO) == NULL) {                                       \
            util_log("vfs_read: fd not open.", 0, LOG_ERROR, LOG_NNONE); \
            return -1;                                                   \
        }                                                                \
    }

spinlock_t mount_lock;
struct {
    int valid;
    const char* path;
    filesystem_t* fs;
} mount_list[MOUNT_NR];

filesystem_t* rootfs;

// Return the length of first part of str
// /mnt -> 4
// /mnt/fuck1 -> 4
int parse_path(const char* str) {
    int len = strlen(str);
    for (int i = 1; i < len; i++) {
        if (str[i] == '/') {
            return i;
        }
    }
    return len;
}

static filesystem_t* _parse_fs(const char* str) {
    for (int i = 0; i < MOUNT_NR; i++) {
        if (mount_list[i].valid == 1) {
            int len1 = parse_path(str);
            int len2 = parse_path(mount_list[i].path);
            if (len1 == len2 && strncmp(str, mount_list[i].path, len1) == 0) {
                return mount_list[i].fs;
            }
        }
    }
    return NULL;
}

filesystem_t* parse_fs(const char* str) {
    kmt->spin_lock(&mount_lock);
    filesystem_t* ret = _parse_fs(str);
    if (ret == NULL) {
        ret = rootfs;
    }
    kmt->spin_unlock(&mount_lock);
    return ret;
}

filesystem_t* devfs_init();
filesystem_t* procfs_init();
void fatfs_init();
static void init() {
    kmt->spin_init(&mount_lock, "mount");
    for (int i = 0; i < MOUNT_NR; i++) {
        mount_list[i].valid = 0;
    }

    vfs->mount("/dev", devfs_init());
    vfs->mount("/proc", procfs_init());

    vfs->mount("/mnt", mkfs(dev_lookup("ramdisk1"), "ramdisk1"));
    rootfs = mkfs(dev_lookup("ramdisk0"), "ramdisk0");
}

static int access(const char* path, int mode) {
    return 0;
}

static int _mount(const char* path, filesystem_t* fs) {
    for (int i = 0; i < MOUNT_NR; i++) {
        if (mount_list[i].valid == 1) {
            if (strcmp(path, mount_list[i].path) == 0) {
                util_log("mount: path already mounted.", 0, LOG_ERROR,
                         LOG_NNONE);
                return -1;
            }
        }
    }

    int ptr = -1;
    for (int i = 0; i < MOUNT_NR; i++) {
        if (mount_list[i].valid == 0) {
            ptr = i;
            break;
        }
    }

    if (ptr == -1) {
        util_log("mount: no more free point.", 0, LOG_ERROR, LOG_NNONE);
        return -1;
    }

    mount_list[ptr].valid = 1;
    mount_list[ptr].path = path;
    mount_list[ptr].fs = fs;

    return 0;
}

static inline int mount(const char* path, filesystem_t* fs) {
    kmt->spin_lock(&mount_lock);
    int ret = _mount(path, fs);
    kmt->spin_unlock(&mount_lock);
    return ret;
}

static int _unmount(const char* path) {
    int ret = -1;
    for (int i = 0; i < MOUNT_NR; i++) {
        if (mount_list[i].valid == 1) {
            if (strcmp(path, mount_list[i].path) == 0) {
                mount_list[i].valid = 0;
                ret = 0;
            }
        }
    }

    if (ret == -1) {
        util_log("unmount: not mounted.", 0, LOG_ERROR, LOG_NNONE);
    }

    return ret;
}

static inline int unmount(const char* path) {
    kmt->spin_lock(&mount_lock);
    int ret = _unmount(path);
    kmt->spin_unlock(&mount_lock);
    return ret;
}

static int mkdir(const char* path) {
    filesystem_t* fs = parse_fs(path);
    if (fs == NULL) {
        util_log("vfs_mkdir: no such fs.", 0, LOG_ERROR, LOG_NNONE);
        return -1;
    }

    return fs->mkdir(fs, path + parse_path(path));
}

static int rmdir(const char* path) {
    filesystem_t* fs = parse_fs(path);
    if (fs == NULL) {
        util_log("vfs_rmdir: no such fs.", 0, LOG_ERROR, LOG_NNONE);
        return -1;
    }

    return fs->rmdir(fs, path + parse_path(path));
}

static int link(const char* oldpath, const char* newpath) {
    util_log("vfs_link: link not supported.", 0, LOG_ERROR, LOG_NNONE);
    return -1;
}

static int unlink(const char* path) {
    util_log("vfs_unlink: unlink not supported.", 0, LOG_ERROR, LOG_NNONE);
    return -1;
}

static int open(const char* path, int flags) {
    filesystem_t* fs = parse_fs(path);
    if (fs == NULL) {
        util_log("vfs_open: no such fs.", 0, LOG_ERROR, LOG_NNONE);
        return -1;
    }

    int fd = -1;
    for (int i = 0; i < NOFILE; i++) {
        if (FP(i) == NULL) {
            fd = i;
            FP(fd) = pmm->alloc(sizeof(file_t));
            break;
        }
    }
    if (fd == -1) {
        util_log("vfs_open: no free file slot.", 0, LOG_ERROR, LOG_NNONE);
        return -1;
    }
    FP(fd)->fspath = path + parse_path(path);
    FP(fd)->offset = 0;
    FP(fd)->fs = fs;

    if (fs->open(FP(fd), flags) != 0) {
        pmm->free(FP(fd));
        FP(fd) = NULL;
        util_log("vfs_open: no such file.", 0, LOG_ERROR, LOG_NNONE);
        return -1;
    }

    return fd;
}

static ssize_t read(int fd, void* buf, size_t nbyte) {
    VALID_FD(fd);

    int ret = FP(fd)->fs->read(FP(fd), buf, nbyte);

    if (ret != -1) {
        FP(fd)->offset += ret;
    }
    return ret;
}

static ssize_t write(int fd, void* buf, size_t nbyte) {
    VALID_FD(fd);

    int ret = FP(fd)->fs->write(FP(fd), buf, nbyte);

    if (ret != -1) {
        FP(fd)->offset += ret;
    }
    return ret;
}

static off_t lseek(int fd, off_t offset, int whence) {
    VALID_FD(fd);

    return FP(fd)->fs->lseek(FP(fd), offset, whence);
}

static int close(int fd) {
    VALID_FD(fd);

    FP(fd)->fs->close(FP(fd));

    pmm->free(FP(fd));
    FP(fd) = NULL;
    return 0;
}

MODULE_DEF(vfs){
    .init = init,
    .access = access,
    .mount = mount,
    .unmount = unmount,
    .mkdir = mkdir,
    .rmdir = rmdir,
    .link = link,
    .unlink = unlink,
    .open = open,
    .read = read,
    .write = write,
    .lseek = lseek,
    .close = close,
};