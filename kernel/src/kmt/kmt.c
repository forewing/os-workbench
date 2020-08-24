#include <common.h>
#include <klib.h>
#include <kmt.h>
#include <sem.h>
#include <spin.h>
#include <trap.h>
#include <util.h>

static spinlock_t tasks_spinlock;
task_t* task_current[MAX_CPU];
task_t* tasks[MAX_CPU][MAX_TASK];
static int cpu_task_nr[MAX_CPU];

static _Context* kmt_contex_save(_Event e, _Context* c);
static _Context* kmt_contex_switch(_Event e, _Context* c);

static void kmt_init() {
    spin_init_self();
    sem_init_self();

    for (int i = 0; i < MAX_CPU; i++) {
        task_current[i] = NULL;
        for (int j = 0; j < MAX_TASK; j++) {
            tasks[i][j] = NULL;
        }
        cpu_task_nr[i] = 0;
    }

    os->on_irq(INT_SEQ_MIN, _EVENT_NULL, kmt_contex_save);
    os->on_irq(INT_SEQ_MAX - 1, _EVENT_NULL, kmt_contex_switch);

    kmt->spin_init(&tasks_spinlock, "kmt_task");
}

static _Context* kmt_contex_save(_Event e, _Context* c) {
    kmt->spin_lock(&tasks_spinlock);

    if (task_current[_cpu()] != NULL) {
        *task_current[_cpu()]->context = *c;
    }

    kmt->spin_unlock(&tasks_spinlock);
    return NULL;
}

static _Context* kmt_contex_switch(_Event e, _Context* c) {
    // util_log("Switch", (uint32_t)c, LOG_WARNING, LOG_NHEX);

    kmt->spin_lock(&tasks_spinlock);

    _Context* ret = c;

    int task_valid_nr = 0;
    task_t* task_valid[MAX_TASK];
    for (int i = 0; i < MAX_TASK; i++) {
        if (tasks[_cpu()][i] != NULL) {
            task_valid[task_valid_nr] = tasks[_cpu()][i];
            task_valid_nr++;
        }
    }

    if (task_valid_nr != 0) {
        task_current[_cpu()] = task_valid[rand() % task_valid_nr];
        ret = task_current[_cpu()]->context;
    }

    kmt->spin_unlock(&tasks_spinlock);

    return ret;
}

static int create(task_t* task,
                  const char* name,
                  void (*entry)(void* arg),
                  void* arg) {
    task->name = name;
    _Area stack =
        (_Area){&task->stack, (void*)((uint32_t)(&task->stack) + STACK_SIZE)};
    task->context = _kcontext(stack, entry, arg);
    task->hungry = 0;

    for (int i = 0; i < NOFILE; i++) {
        task->files[i] = NULL;
    }

    kmt->spin_lock(&tasks_spinlock);

    int min = 0x3f3f3f3f, pos = -1;
    for (int i = 0; i < _ncpu(); i++) {
        if (cpu_task_nr[i] <= min) {
            min = cpu_task_nr[i];
            pos = i;
        }
    }
    cpu_task_nr[pos] += 1;

    int cpu_sel = pos;
    int ptr;
    for (ptr = 0; ptr < MAX_TASK; ptr++) {
        if (tasks[cpu_sel][ptr] == NULL) {
            tasks[cpu_sel][ptr] = task;
            task->cpu = cpu_sel;
            break;
        }
    }

    // util_log("init task start", (uint32_t)stack.start, LOG_INFO, LOG_NHEX);
    // util_log("init task end", (uint32_t)stack.end, LOG_INFO, LOG_NHEX);

    if (ptr == MAX_TASK) {
        // no free slot
        assert(0);
    }

    task->pid = cpu_sel * MAX_TASK + ptr;

    kmt->spin_unlock(&tasks_spinlock);

    return 0;
}

static void teardown(task_t* task) {
    kmt->spin_lock(&tasks_spinlock);
    int cpu = task->cpu;
    for (int i = 0; i < MAX_TASK; i++) {
        if (tasks[cpu][i] == task) {
            tasks[cpu][i] = NULL;
        }
    }
    kmt->spin_unlock(&tasks_spinlock);
}

MODULE_DEF(kmt){
    .init = kmt_init,
    .create = create,
    .teardown = teardown,
    .spin_init = spin_init,
    .spin_lock = spin_lock,
    .spin_unlock = spin_unlock,
    .sem_init = sem_init,
    .sem_wait = sem_wait,
    .sem_signal = sem_signal,
};
