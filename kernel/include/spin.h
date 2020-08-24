#ifndef __SPIN_H__
#define __SPIN_H__

#include <common.h>

void spin_init_self();

void spin_init(spinlock_t* lk, const char* name);
void spin_lock(spinlock_t* lk);
void spin_unlock(spinlock_t* lk);

#endif