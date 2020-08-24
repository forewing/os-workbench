#include <common.h>
#include <klib.h>
#include <shell.h>
#include <util.h>
#include <vfs.h>

static const char* help = "Use: cat {PATH}\n";

#define BUFFER_SIZE 1024

int shell_cat(const char* args, int fd) {
    int len = strlen(args);
    if (len == 0) {
        vfs->write(fd, (void*)help, strlen(help));
        return -1;
    }

    int fd_file = vfs->open(args, MODE_W);
    if (fd_file == -1) {
        return -1;
    }

    char* buf = pmm->alloc(BUFFER_SIZE);
    // vfs->read(fd_file, (void*)buf, BUFFER_SIZE);

    vfs->write(fd, (void*)buf, vfs->read(fd_file, (void*)buf, BUFFER_SIZE));
    vfs->close(fd_file);
    pmm->free(buf);
    return 0;
}