#include <common.h>
#include <devices.h>
#include <klib.h>
#include <util.h>
#include <vfs.h>

static filesystem_t devfs;

static int devfs_open(file_t* file, int flag) {
    if (strlen(file->fspath) <= 0) {
        util_log("devfs_check: empty path.", 0, LOG_ERROR, LOG_NNONE);
        return -1;
    }
    file->data = dev_lookup(file->fspath + 1);
    if (file->data == NULL) {
        return -1;
    }
    return 0;
}

static int devfs_close(file_t* file) {
    return 0;
}

static ssize_t devfs_read(file_t* file, char* buf, size_t size) {
    device_t* dev = (device_t*)file->data;
    int ret = dev->ops->read(dev, file->offset, buf, size);
    buf[ret] = '\0';
    return ret;
}

static ssize_t devfs_write(file_t* file, const char* buf, size_t size) {
    device_t* dev = (device_t*)file->data;
    return dev->ops->write(dev, file->offset, buf, size);
}

extern char initrd_start, initrd_end;
static ssize_t devfs_lseek(file_t* file, off_t offset, int whence) {
    int type = 0;
    if (strcmp(file->fspath, "/ramdisk0") == 0) {
        type = 1;
    } else if (strcmp(file->fspath, "/ramdisk1") == 0) {
        type = 2;
    }
    if (type == 0) {
        util_log("devfs_lseek: lseek no support on pipe.", 0, LOG_ERROR,
                 LOG_NNONE);
        return -1;
    }

    switch (whence) {
        case SEEK_SET:
            file->offset = offset;
            break;

        case SEEK_CUR:
            file->offset += offset;
            break;

        case SEEK_END:
            if (type == 1) {
                file->offset = offset + (ssize_t)(&initrd_end - &initrd_start);
            } else if (type == 2) {
                file->offset = offset + RD_SIZE;
            }
            break;

        default:
            util_log("devfs_lseek: invalid whence.", 0, LOG_ERROR, LOG_NNONE);
            return -1;
            break;
    }

    return file->offset;
}

int devfs_dir(filesystem_t* fs, const char* path) {
    util_log("devfs_dir: dir not support in devfs.", 0, LOG_ERROR, LOG_NNONE);
    return -1;
}

filesystem_t* devfs_init() {
    devfs.dev = NULL;

    devfs.name = "devfs";
    devfs.open = devfs_open;
    devfs.close = devfs_close;
    devfs.read = devfs_read;
    devfs.write = devfs_write;
    devfs.lseek = devfs_lseek;
    devfs.mkdir = devfs_dir;
    devfs.rmdir = devfs_dir;

    return &devfs;
}