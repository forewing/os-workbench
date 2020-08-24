#ifndef __PMM_H__
#define __PMM_H__

#include <common.h>

enum PMM_STATUS { PMM_FREE, PMM_USED };

typedef struct pmm_t pmm_t;
typedef struct pmm_t {
    pmm_t* next;
    pmm_t* prev;
    size_t size;
    enum PMM_STATUS status;
} pmm_t;

extern uint32_t free_mem;
extern uint32_t total_mem;

#endif