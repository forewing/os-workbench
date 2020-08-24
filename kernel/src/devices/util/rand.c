#include <common.h>
#include <klib.h>

static int rand_init(device_t* dev) {
    return 0;
}

static ssize_t rand_read(device_t* dev, off_t offset, void* buf, size_t count) {
    for (int i = 0; i < count; i++) {
        ((char*)buf)[i] = rand() % 0x100;
    }
    return count;
}

static ssize_t rand_write(device_t* dev,
                          off_t offset,
                          const void* buf,
                          size_t count) {
    return count;
}

devops_t rand_ops = {
    .init = rand_init,
    .read = rand_read,
    .write = rand_write,
};
