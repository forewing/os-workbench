#include <common.h>
#include <klib.h>
#include <kmt.h>
#include <spin.h>

typedef struct {
    int cli_depth;
    int intr_backup;
} cpu_lockinfo_t;

static cpu_lockinfo_t cpu_lockinfo[MAX_CPU];

static void cli_push() {
    uintptr_t intr_backup = _intr_read();
    _intr_write(0);
    if (cpu_lockinfo[_cpu()].cli_depth == 0) {
        cpu_lockinfo[_cpu()].intr_backup = intr_backup;
    }

    cpu_lockinfo[_cpu()].cli_depth += 1;
}

static void cli_pop() {
    uintptr_t intr_backup = _intr_read();
    if (intr_backup == 1) {
        assert(0);
    }

    cpu_lockinfo[_cpu()].cli_depth -= 1;
    if (cpu_lockinfo[_cpu()].cli_depth < 0) {
        assert(0);
    }

    if (cpu_lockinfo[_cpu()].cli_depth == 0 &&
        cpu_lockinfo[_cpu()].intr_backup == 1) {
        _intr_write(1);
    }
}

static int holding(spinlock_t* lk) {
    int r;
    cli_push();
    r = lk->locked && lk->cpu == _cpu();
    cli_pop();
    return r;
}

void spin_init_self() {
    for (int i = 0; i < _ncpu(); i++) {
        cpu_lockinfo[i].cli_depth = 0;
        cpu_lockinfo[i].intr_backup = 1;
    }
}

void spin_init(spinlock_t* lk, const char* name) {
    lk->locked = 0;
    lk->cpu = -1;
    lk->name = name;
    return;
}

void spin_lock(spinlock_t* lk) {
    cli_push();

    if (holding(lk)) {
        assert(0);
    }

    while (_atomic_xchg(&lk->locked, 1) != 0)
        ;
    lk->cpu = _cpu();
    return;
}

void spin_unlock(spinlock_t* lk) {
    if (!holding(lk)) {
        assert(0);
    }

    lk->cpu = -1;
    _atomic_xchg(&lk->locked, 0);
    cli_pop();
    return;
}