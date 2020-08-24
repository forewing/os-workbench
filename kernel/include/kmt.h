#ifndef __KMT_H__
#define __KMT_H__

#include <common.h>
#include <vfs.h>

struct task {
    int valid;
    const char* name;
    _Context* context;
    uint8_t stack[STACK_SIZE];
    int cpu;
    int hungry;

    uint32_t pid;
    file_t* files[NOFILE];
};

struct spinlock {
    int cpu;
    int locked;
    const char* name;
};

struct semaphore {
    int value;
    spinlock_t lock;
    task_t* tasks[MAX_TASK];
    int tail;
    int head;
};

extern task_t* task_current[MAX_CPU];

#endif