#ifndef __SEM_H__
#define __SEM_H__

#include <common.h>

void sem_init_self();

void sem_init(sem_t* sem, const char* name, int value);
void sem_wait(sem_t* sem);
void sem_signal(sem_t* sem);

#endif