#ifndef __SHELL_H__
#define __SHELL_H__

#include <common.h>

int shell_echo(const char* args, int fd);
int shell_help(const char* args, int fd);
int shell_cat(const char* args, int fd);
int shell_mkdir(const char* args, int fd);
int shell_rmdir(const char* args, int fd);
int shell_test(const char* args, int fd);

struct commands {
    const char* name;
    int (*func)(const char* args, int fd);
};

int parse_args(const char* str);

void shell_task(void* ttyid);

#endif