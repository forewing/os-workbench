#include <common.h>
#include <klib.h>
#include <shell.h>
#include <vfs.h>

static const char* help =
    "Currently defined functions:\n"
    "help, echo, cat, mkdir, rmdir, test\n";

int shell_help(const char* args, int fd) {
    vfs->write(fd, (void*)help, strlen(help));
    return 0;
}