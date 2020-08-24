#include <common.h>
#include <klib.h>

static int zero_init(device_t* dev) {
    return 0;
}

static ssize_t zero_read(device_t* dev, off_t offset, void* buf, size_t count) {
    for (int i = 0; i < count; i++) {
        ((char*)buf)[i] = 0;
    }
    return count;
}

static ssize_t zero_write(device_t* dev,
                          off_t offset,
                          const void* buf,
                          size_t count) {
    return count;
}

devops_t zero_ops = {
    .init = zero_init,
    .read = zero_read,
    .write = zero_write,
};
