#include <common.h>
#include <klib.h>
#include <kmt.h>
#include <shell.h>
#include <vfs.h>

void vfs_test(void* arg);

int shell_test(const char* args, int fd) {
    kmt->create(pmm->alloc(sizeof(task_t)), "vfs_test", vfs_test, NULL);
    return 0;
}