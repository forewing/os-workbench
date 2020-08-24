#include <common.h>
#include <klib.h>
#include <pmm.h>
#include <shell.h>
#include <util.h>
#include <vfs.h>

#define BUFFER_SIZE 1024

struct commands cmds[] = {
    {"help", shell_help},   {"echo", shell_echo},   {"cat", shell_cat},
    {"mkdir", shell_mkdir}, {"rmdir", shell_rmdir}, {"test", shell_test},
};

#define CMD_LEN (sizeof(cmds) / sizeof(struct commands))

int parse_args(const char* str) {
    int len = strlen(str);
    for (int i = 0; i < len; i++) {
        if (str[i] == ' ') {
            return i;
        }
    }
    return len;
}

void shell_task(void* ttyid) {
    char* buffer = pmm->alloc(BUFFER_SIZE);
    sprintf(buffer, "/dev/tty%d", (int)ttyid);
    int fd = vfs->open(buffer, 0);
    while (true) {
        vfs->write(fd, "$ ", 2);
        memset(buffer, 0, BUFFER_SIZE);
        buffer[vfs->read(fd, buffer, BUFFER_SIZE) - 1] = '\0';
        int ptr = parse_args(buffer);
        for (int i = 0; i < CMD_LEN; i++) {
            if (strlen(cmds[i].name) == ptr &&
                strncmp(cmds[i].name, buffer, ptr) == 0) {
                // util_log(cmds[i].name, i, LOG_INFO, LOG_NHEX);
                // util_log(buffer + ptr + 1, i, LOG_INFO, LOG_NHEX);
                int ret = cmds[i].func(buffer + ptr + 1, fd);
                sprintf(buffer, "%s exit with code %d.\n", cmds[i].name, ret);
                vfs->write(fd, buffer, strlen(buffer));
                break;
            }
        }
    }
}