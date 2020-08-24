#include <common.h>
#include <klib.h>
#include <shell.h>
#include <util.h>
#include <vfs.h>

static const char* help = "Use: mkdir/rmdir {PATH}\n";

#define BUFFER_SIZE 1024

int shell_mkdir(const char* args, int fd) {
    int len = strlen(args);
    if (len == 0) {
        vfs->write(fd, (void*)help, strlen(help));
        return -1;
    }

    return vfs->mkdir(args);
}

int shell_rmdir(const char* args, int fd) {
    int len = strlen(args);
    if (len == 0) {
        vfs->write(fd, (void*)help, strlen(help));
        return -1;
    }

    return vfs->rmdir(args);
}