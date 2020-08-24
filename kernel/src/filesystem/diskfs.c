#include <common.h>
#include <devices.h>
#include <diskfs.h>
#include <util.h>
#include <vfs.h>

#define WRITE(__WRITE_DEV, __WRITE_OFFSET, __WRITE_BUFFER, __WRITE_COUNT) \
    __WRITE_DEV->ops->write(__WRITE_DEV, (off_t)__WRITE_OFFSET,           \
                            __WRITE_BUFFER, __WRITE_COUNT)

#define READ(__READ_DEV, __READ_OFFSET, __READ_BUFFER, __READ_COUNT)       \
    __READ_DEV->ops->read(__READ_DEV, (off_t)__READ_OFFSET, __READ_BUFFER, \
                          __READ_COUNT)

#define f2d ((diskfs_t*)fs->data)
#define f2i (f2d->info)

static disk_t disk = (sector_t*)0;

// Return the sector nr of path
// root should be access directly, do not using this
static int _parse_route(filesystem_t* fs, int parent, const char* path) {
    if (strlen(path) == 0) {
        return parent;
    }
    if (strlen(path) == 1 && path[0] == '/') {
        return parent;
    }
    if (parent < f2i->dataSt || parent >= f2i->secNr) {
        return -1;
    }

    int ret = -1;

    disknode_t* node = pmm->alloc(f2i->secSz);
    disknode_t* child = pmm->alloc(f2i->secSz);
    READ(fs->dev, disk[parent], node, f2i->secSz);
    if (node->type == DISKNODE_DIR) {
        for (int i = 0; i < f2d->dataNr; i++) {
            if (node->data[i] != -1) {
                READ(fs->dev, disk[node->data[i]], child, f2i->secSz);
                int len1 = strlen(child->name);
                int len2 = parse_path(path);
                if (len1 == len2 - 1 &&
                    strncmp(path + 1, child->name, len1) == 0) {
                    // return _parse_route(fs, parent, path + len2);
                    ret = _parse_route(fs, node->data[i], path + len2);
                    break;
                }
            }
        }
    }
    pmm->free(node);
    pmm->free(child);

    return ret;
}

static int _alloc_sector(filesystem_t* fs) {  // not MT-Safe
    int ret = -1;
    char* valid = pmm->alloc(f2i->secNr);
    READ(fs->dev, disk[1], valid, f2i->secNr);
    for (int i = 0; i < f2i->secNr; i++) {
        if (valid[i] == 0) {
            ret = i;
            char t = 1;
            WRITE(fs->dev, &disk[1][i], &t, 1);
            break;
        }
    }
    pmm->free(valid);
    return ret;
}

static int _free_sector(filesystem_t* fs, int sec) {
    if (sec < f2i->dataSt || sec >= f2i->secNr) {
        return -1;
    }
    char t = 0;
    WRITE(fs->dev, &disk[1][sec], &t, 1);
    return 0;
}

static int _mknode(filesystem_t* fs, int sec, const char* name, int type) {
    if (sec < f2i->dataSt || sec >= f2i->secNr) {
        return -1;
    }
    if (strlen(name) > 31) {
        return -1;
    }

    disknode_t* node = pmm->alloc(f2i->secSz);

    node->type = type;
    strcpy(node->name, name);

    for (int i = 0; i < f2d->dataNr; i++) {
        node->data[i] = -1;
    }

    WRITE(fs->dev, disk[sec], node, f2i->secSz);
    pmm->free(node);

    return 0;
}

static int _add_child(filesystem_t* fs, int parent, int child) {
    if (parent < f2i->dataSt || parent >= f2i->secNr) {
        return -1;
    }

    disknode_t* node = pmm->alloc(f2i->secSz);
    READ(fs->dev, disk[parent], node, f2i->secSz);
    int ptr = -1;
    for (int i = 0; i < f2d->dataNr; i++) {
        if (node->data[i] == -1) {
            ptr = i;
            break;
        }
    }
    if (ptr == -1) {
        pmm->free(node);
        return -1;
    }
    node->data[ptr] = child;
    node->size++;
    WRITE(fs->dev, disk[parent], node, f2i->secSz);
    pmm->free(node);
    return 0;
}

static int _del_child(filesystem_t* fs, int parent, int child) {
    if (parent < f2i->dataSt || parent >= f2i->secNr) {
        return -1;
    }

    disknode_t* node = pmm->alloc(f2i->secSz);
    READ(fs->dev, disk[parent], node, f2i->secSz);
    int ptr = -1;
    for (int i = 0; i < f2d->dataNr; i++) {
        if (node->data[i] == child) {
            ptr = i;
            break;
        }
    }
    if (ptr == -1) {
        pmm->free(node);
        return -1;
    }
    node->data[ptr] = -1;
    node->size--;
    WRITE(fs->dev, disk[parent], node, f2i->secSz);
    pmm->free(node);
    return 0;
}

static int _enlarge_file(filesystem_t* fs, int sec, int size) {
    if (size > f2i->secSz * f2d->dataNr) {
        return -1;
    }

    disknode_t* node = pmm->alloc(sizeof(disknode_t));
    READ(fs->dev, disk[sec], node, sizeof(disknode_t));
    int ptr;
    for (ptr = 0; ptr < f2d->dataNr; ptr++) {
        if (node->data[ptr] == -1) {
            break;
        }
    }
    int sec_new = size / f2d->dataNr;
    for (int i = ptr; i <= sec_new; i++) {
        node->data[i] = _alloc_sector(fs);
    }
    WRITE(fs->dev, disk[sec], node, sizeof(disknode_t));

    pmm->free(node);
    return 0;
}

static int _disk_open(file_t* file, int flag) {
    filesystem_t* fs = file->fs;

    int len = strlen(file->fspath);
    char* buf = pmm->alloc(len + 1);
    char* name = NULL;
    strcpy(buf, file->fspath);

    for (int i = len; i >= 0; i--) {
        if (buf[i] == '/') {
            buf[i] = '\0';
            name = &buf[i + 1];
            break;
        }
    }

    // util_log(buf, 0, LOG_INFO, LOG_NNONE);
    // util_log(name, 0, LOG_INFO, LOG_NNONE);

    int ret = -1;

    int parent = _parse_route(fs, f2i->root, buf);
    // util_log("par", parent, LOG_INFO, LOG_NHEX);
    if (parent != -1) {
        int child = _parse_route(fs, f2i->root, file->fspath);
        // util_log("chils", child, LOG_INFO, LOG_NHEX);

        if (child != -1) {
            ret = 0;
            diskinode_t* inode = pmm->alloc(sizeof(diskinode_t));
            inode->sec = child;
            file->data = inode;
        } else if (flag == MODE_W) {
            ret = 0;
            child = _alloc_sector(fs);
            _mknode(fs, child, name, DISKNODE_FILE);
            _add_child(fs, parent, child);
            diskinode_t* inode = pmm->alloc(sizeof(diskinode_t));
            inode->sec = child;
            file->data = inode;
        }
    }

    pmm->free(buf);
    return ret;
}

static int disk_open(file_t* file, int flag) {
    filesystem_t* fs = file->fs;
    kmt->spin_lock(&f2d->lock);
    int ret = _disk_open(file, flag);
    kmt->spin_unlock(&f2d->lock);
    return ret;
}

static int disk_close(file_t* file) {
    pmm->free(file->data);
    return 0;
}

static ssize_t _disk_read(file_t* file,
                          char* buf,
                          size_t size,
                          disknode_t* node) {
    filesystem_t* fs = file->fs;
    int sec = ((diskinode_t*)file->data)->sec;

    READ(fs->dev, disk[sec], node, sizeof(disknode_t));
    if (file->offset + size > node->size) {
        size = node->size - file->offset;
    }
    if (size <= 0) {
        return -1;
    }

    int start = file->offset;
    int end = start + size;
    // int ptr = 0;

    // _enlarge_file(fs, sec, end);

    char* diskbuf = pmm->alloc(size + 2 * f2i->secSz);

    for (int i = start / f2i->secSz; i < end / f2i->secSz + 1; i++) {
        READ(fs->dev, disk[i], diskbuf + i * f2i->secSz, f2i->secSz);
    }

    memcpy(buf, diskbuf + (start % f2i->secSz), size);

    pmm->free(diskbuf);
    return size;
}

static ssize_t disk_read(file_t* file, char* buf, size_t size) {
    filesystem_t* fs = file->fs;
    disknode_t* node = pmm->alloc(sizeof(disknode_t));
    kmt->spin_lock(&f2d->lock);
    int ret = _disk_read(file, buf, size, node);
    kmt->spin_unlock(&f2d->lock);
    pmm->free(node);
    return ret;
}

static ssize_t _disk_write(file_t* file,
                           const char* buf,
                           size_t size,
                           disknode_t* node) {
    filesystem_t* fs = file->fs;
    int sec = ((diskinode_t*)file->data)->sec;
    int start = file->offset;
    int end = start + size;
    // int ptr = 0;

    _enlarge_file(fs, sec, end);
    READ(fs->dev, disk[sec], node, sizeof(disknode_t));
    node->size += size;
    WRITE(fs->dev, disk[sec], node, sizeof(disknode_t));

    char* diskbuf = pmm->alloc(size + 2 * f2i->secSz);

    for (int i = start / f2i->secSz; i < end / f2i->secSz + 1; i++) {
        READ(fs->dev, disk[i], diskbuf + i * f2i->secSz, f2i->secSz);
    }

    memcpy(diskbuf + (start % f2i->secSz), buf, size);

    for (int i = start / f2i->secSz; i < end / f2i->secSz + 1; i++) {
        WRITE(fs->dev, disk[i], diskbuf + i * f2i->secSz, f2i->secSz);
    }

    pmm->free(diskbuf);

    // WRITE(fs->dev, &disk[file->offset / f2i->secSz][now % f2i->secSz],
    // buf,
    //       f2i->secSz - (now % f2i->secSz));
    // ptr += f2i->secSz - (now % f2i->secSz);
    // util_log("fuck", ptr, LOG_INFO, LOG_NHEX);
    // for (int i = file->offset / f2i->secSz + 1; i < end / f2i->secSz;
    // i++) {
    //     WRITE(fs->dev, disk[i], buf + ptr, f2i->secSz);
    //     ptr += f2i->secSz;
    // }
    // WRITE(fs->dev, disk[end / f2i->secSz], buf + ptr, size - ptr);
    return size;
}

static ssize_t disk_write(file_t* file, const char* buf, size_t size) {
    filesystem_t* fs = file->fs;
    disknode_t* node = pmm->alloc(sizeof(disknode_t));
    kmt->spin_lock(&f2d->lock);
    int ret = _disk_write(file, buf, size, node);
    kmt->spin_unlock(&f2d->lock);
    pmm->free(node);
    return ret;
}

static off_t _disk_lseek(file_t* file,
                         off_t offset,
                         int whence,
                         disknode_t* node) {
    READ(file->fs->dev, disk[((diskinode_t*)file->data)->sec], node,
         sizeof(disknode_t));

    if (node->type != DISKNODE_FILE) {
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
            file->offset = node->size + offset;
            break;

        default:
            util_log("devfs_lseek: invalid whence.", 0, LOG_ERROR, LOG_NNONE);
            return -1;
            break;
    }

    return file->offset;
}

static off_t disk_lseek(file_t* file, off_t offset, int whence) {
    filesystem_t* fs = file->fs;
    disknode_t* node = pmm->alloc(sizeof(disknode_t));
    kmt->spin_lock(&f2d->lock);
    int ret = _disk_lseek(file, offset, whence, node);
    kmt->spin_unlock(&f2d->lock);
    pmm->free(node);
    return ret;
}

static int _disk_mkdir(filesystem_t* fs, const char* path) {
    int len = strlen(path);
    char* buf = pmm->alloc(len + 1);
    char* name = NULL;
    strcpy(buf, path);
    for (int i = len; i >= 0; i--) {
        if (buf[i] == '/') {
            buf[i] = '\0';
            name = &buf[i + 1];
            break;
        }
    }

    // util_log(buf, 0, LOG_SUCCESS, LOG_NNONE);
    // util_log(name, 0, LOG_SUCCESS, LOG_NNONE);

    int parent = _parse_route(fs, f2i->root, buf);

    int ret = -1;

    if (parent != -1) {
        int child = _alloc_sector(fs);
        _mknode(fs, child, name, DISKNODE_DIR);
        _add_child(fs, parent, child);
        ret = 0;
    }

    pmm->free(buf);
    return ret;
}
static int disk_mkdir(filesystem_t* fs, const char* path) {
    kmt->spin_lock(&f2d->lock);
    int ret = _disk_mkdir(fs, path);
    kmt->spin_unlock(&f2d->lock);
    return ret;
}

static int _disk_rmdir(filesystem_t* fs, const char* path) {
    int len = strlen(path);
    char* buf = pmm->alloc(len + 1);
    strcpy(buf, path);
    for (int i = len; i >= 0; i--) {
        if (buf[i] == '/') {
            buf[i] = '\0';
            break;
        }
    }

    // util_log(buf, 0, LOG_SUCCESS, LOG_NNONE);
    // util_log(name, 0, LOG_SUCCESS, LOG_NNONE);

    int parent = _parse_route(fs, f2i->root, buf);
    int child = _parse_route(fs, f2i->root, path);

    int ret = -1;

    if (parent != -1 && child != -1) {
        _del_child(fs, parent, child);
        ret = 0;
    }

    pmm->free(buf);
    return ret;
}
static int disk_rmdir(filesystem_t* fs, const char* path) {
    kmt->spin_lock(&f2d->lock);
    int ret = _disk_rmdir(fs, path);
    kmt->spin_unlock(&f2d->lock);
    return ret;
}

filesystem_t* mkfs(device_t* dev, const char* name) {
    assert(dev != NULL);

    filesystem_t* fs = pmm->alloc(sizeof(filesystem_t));
    fs->name = name;
    fs->dev = dev;
    fs->data = pmm->alloc(sizeof(diskfs_t));

    f2i = pmm->alloc(sizeof(diskinfo_t));

    // util_log(fs->name, (uint32_t)fs, LOG_INFO, LOG_NHEX);
    // util_log(fs->name, (uint32_t)f2d, LOG_INFO, LOG_NHEX);
    // util_log(fs->name, (uint32_t)f2i, LOG_INFO, LOG_NHEX);

    if (strlen(name) > 31) {
        memcpy(f2i->name, name, 31);
        f2i->name[31] = '\0';
    } else {
        strcpy(f2i->name, name);
    }

    f2i->secSz = SECTOR_SIZE;

    f2d->size = ((rd_t*)dev->ptr)->end - ((rd_t*)dev->ptr)->start;
    f2d->dataNr = (f2i->secSz - sizeof(int) * 2 - 32) / sizeof(int*);
    kmt->spin_init(&f2d->lock, name);

    f2i->secNr = f2d->size / f2i->secSz;

    f2i->dataSt = (f2i->secNr / f2i->secSz) + 2;

    char* valid = pmm->alloc(f2i->secNr);
    memset(valid, 0, f2i->secNr);
    memset(valid, 1, f2i->dataSt);
    WRITE(dev, disk[1], valid, f2i->secNr);
    pmm->free(valid);

    f2i->root = _alloc_sector(fs);
    WRITE(dev, disk[0], f2i, f2i->secSz);

    assert(_mknode(fs, f2i->root, "/", DISKNODE_DIR) == 0);

    int tmp = _alloc_sector(fs);
    _mknode(fs, tmp, "fuck.txt", DISKNODE_FILE);
    _add_child(fs, f2i->root, tmp);
    _parse_route(fs, f2i->root, "/fuck.txt");
    _del_child(fs, f2i->root, tmp);
    _free_sector(fs, tmp);

    fs->open = disk_open;
    fs->close = disk_close;
    fs->read = disk_read;
    fs->write = disk_write;
    fs->lseek = disk_lseek;
    fs->mkdir = disk_mkdir;
    fs->rmdir = disk_rmdir;

    return fs;
}