#include <common.h>
#include <klib.h>
#include <shell.h>
#include <vfs.h>

static const char* help = "Use: echo {PATH} {CONTENT}\n";

int shell_echo(const char* args, int fd) {
    int len = strlen(args);
    int next = parse_args(args);
    if (len == next) {
        vfs->write(fd, (void*)help, strlen(help));
        return -1;
    }
    char* path = pmm->alloc(next + 1);
    strncpy(path, args, next);
    path[next] = '\0';

    int fd_file = vfs->open(path, MODE_W);
    if (fd_file == -1) {
        pmm->free(path);
        return -1;
    }

    vfs->write(fd_file, (void*)(args + next + 1), len - next - 1);
    vfs->close(fd_file);
    pmm->free(path);
    return 0;
}