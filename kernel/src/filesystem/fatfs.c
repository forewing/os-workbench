/*

// DEPRECATED


#include <common.h>
#include <devices.h>
#include <fatfs.h>
#include <util.h>
#include <vfs.h>

static filesystem_t fatfs;

static spinlock_t fatfs_lock;

static fatdisk_t* disk;
static fatinode_t* root;

static char empty_sector[SECTOR_SIZE];
static char valid_buf[sizeof(fatvalid_t)];

#define WRITE(__WRITE_OFFSET, __WRITE_BUFFER, __WRITE_SIZE)                 \
    fatfs.dev->ops->write(fatfs.dev, (off_t)__WRITE_OFFSET, __WRITE_BUFFER, \
                          __WRITE_SIZE)

#define READ(__READ_OFFSET, __READ_BUFFER, __READ_SIZE)                  \
    fatfs.dev->ops->read(fatfs.dev, (off_t)__READ_OFFSET, __READ_BUFFER, \
                         __READ_SIZE)

static fatinode_t* find_inode(fatinode_t* parent, const char* path);
static fatinode_t* _find_inode(fatinode_t* parent,
                               const char* path,
                               char* buf,
                               fatinode_t** dir) {
    READ(parent->name, buf, INODE_NAME_SIZE);

    // if (strncmp(path, parent->name, INODE_NAME_SIZE) == 0) {
    //     return parent;
    // }

    if (strlen(path) == 0) {
        return parent;
    }

    fatiinfo_t info;
    READ(parent->info, &info, sizeof(fatiinfo_t));

    if (info.flag != INODE_TYPE_DIR) {
        return NULL;
    }

    // /fuck/fuck2 5

    for (int i = 0; i < info.secs; i++) {
        READ(info.ptr, dir, sizeof(fatinode_t*) * DIR_CHILD_NR);
        for (int j = 0; j < DIR_CHILD_NR; j++) {
            READ(dir[j]->name, buf, INODE_NAME_SIZE);
            int len1 = strlen(buf);
            int len2 = parse_path(path);
            if (len1 - 1 == len2 && strncmp(buf, path + 1, len2) == 0) {
                return find_inode(dir[j], path + len1);
            }
        }
    }

    return NULL;
}

static fatinode_t* find_inode(fatinode_t* parent, const char* path) {
    char* buf = pmm->alloc(INODE_NAME_SIZE);
    fatinode_t** dir = pmm->alloc(sizeof(fatinode_t*) * DIR_CHILD_NR);
    fatinode_t* ret = _find_inode(parent, path, buf, dir);
    pmm->free(buf);
    pmm->free(dir);
    return ret;
}

static void* alloc_sec() {
    READ(disk->valid, valid_buf, sizeof(fatvalid_t));

    int ptr = -1;
    for (int i = 0; i < SECTOR_NR; i++) {
        if (valid_buf[i] == 0) {
            char write = 1;
            WRITE(&disk->valid[i], &write, 1);
            ptr = i;
            break;
        }
    }

    if (ptr == -1) {
        return NULL;
    }

    WRITE(disk->secs[ptr], empty_sector, SECTOR_SIZE);

    return (void*)(disk->secs[ptr]);
}

static fatinode_t* new_inode(int flag, const char* name) {
    fatinode_t* ret = (fatinode_t*)alloc_sec();
    if (ret == NULL) {
        return NULL;
    }

    fatiinfo_t info;
    info.flag = flag;
    info.size = 0;
    info.secs = 0;
    info.ptr = NULL;

    WRITE(ret->info, &info, sizeof(info));
    int len = strlen(name) + 1;
    assert(len <= INODE_NAME_SIZE);

    WRITE(ret->name, name, 0);

    return ret;
}

static int fatfs_open(file_t* file) {
    return -1;
}

static int fatfs_close(file_t* file) {
    return -1;
}

static ssize_t fatfs_read(file_t* file, char* buf, size_t size) {
    return -1;
}

static ssize_t fatfs_write(file_t* file, const char* buf, size_t size) {
    return -1;
}

static off_t fatfs_lseek(file_t* file, off_t offset, int whence) {
    return -1;
}

static int fatfs_mkdir(filesystem_t* fs, const char* path) {
    find_inode(root, "/fuck");
    return -1;
}

static int fatfs_rmdir(filesystem_t* fs, const char* path) {
    return -1;
}

void fatfs_init() {
    fatfs.dev = dev_lookup("ramdisk1");
    fatfs.name = "fatfs";
    fatfs.open = fatfs_open;
    fatfs.close = fatfs_close;
    fatfs.read = fatfs_read;
    fatfs.write = fatfs_write;
    fatfs.lseek = fatfs_lseek;
    fatfs.mkdir = fatfs_mkdir;
    fatfs.rmdir = fatfs_rmdir;

    assert(fatfs.dev != NULL);

    kmt->spin_init(&fatfs_lock, "fatfs");

    disk = (fatdisk_t*)0;

    memset(valid_buf, 0, sizeof(fatvalid_t));
    memset(empty_sector, 0, SECTOR_SIZE);

    WRITE(disk->valid, valid_buf, sizeof(fatvalid_t));

    root = new_inode(INODE_TYPE_DIR, "");

    char* buf = pmm->alloc(100);
    READ(root->name, buf, 100);
    assert(strcmp(buf, "") == 0);
    pmm->free(buf);

    fatiinfo_t info;
    READ(root->info, &info, sizeof(fatiinfo_t));
    assert(info.flag == INODE_TYPE_DIR);
    assert(info.size == 0);
    assert(info.secs == 0);
    assert(info.ptr == NULL);

    vfs->mount("/mnt", &fatfs);
}
*/