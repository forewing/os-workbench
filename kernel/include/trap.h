#ifndef __TRAP_H__
#define __TRAP_H__

#include <common.h>

#define INT_SEQ_MIN 0
#define INT_SEQ_MAX 4

#define INT_NR_MAX 4

struct trap_handler{
    int seq;
    int event;
    handler_t handler;
    int valid;
};

typedef struct trap_handler trap_handler_t;

#endif