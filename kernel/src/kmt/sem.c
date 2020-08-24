#include <common.h>
#include <kmt.h>
#include <sem.h>

void sem_init_self() {
    return;
}

void sem_init(sem_t* sem, const char* name, int value) {
    sem->value = value;
    kmt->spin_init(&sem->lock, name);
    for (int i = 0; i < MAX_TASK; i++) {
        sem->tasks[i] = NULL;
    }
    sem->tail = sem->head = 0;
}

void sem_wait(sem_t* sem) {
    kmt->spin_lock(&sem->lock);
    if (sem->value <= 0) {
        // not support now
        sem->tasks[sem->tail] = task_current[_cpu()];
        sem->tail = (sem->tail + 1) % MAX_TASK;
    }
    while (sem->value <= 0) {
        kmt->spin_unlock(&sem->lock);
        // task_current[_cpu()]->hungry = 1;
        _yield();
        kmt->spin_lock(&sem->lock);
    }
    sem->value--;
    kmt->spin_unlock(&sem->lock);
}

void sem_signal(sem_t* sem) {
    kmt->spin_lock(&sem->lock);
    sem->value++;
    if (sem->tasks[sem->head] != NULL) {
        // go to sem->head
        // BUT NOT PLANNING TO SUPPORT NOW
        // sem->tasks[sem->head]->hungry = 0;
        sem->tasks[sem->head] = NULL;
        sem->head = (sem->head + 1) % MAX_TASK;
    }
    kmt->spin_unlock(&sem->lock);
}