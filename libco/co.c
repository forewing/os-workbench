#include <ucontext.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include "co.h"

// #define log()

#define CO_MAX 16

enum co_State
{
    CO_INVALID,
    CO_INITED,
    CO_RUNNING,
    CO_PAUSED
};

struct co
{
    char *name;
    func_t func;
    void *arg;
    enum co_State state;
    ucontext_t ctx;
    uint8_t stack[4 * 1024];
};
struct co coroutines[CO_MAX];
ucontext_t main_ctx;
// struct co main_thd;
int co_active;

void co_init()
{
    srand(time(0));

    for (int i = 0; i < CO_MAX; i++)
    {
        coroutines[i].state = CO_INVALID;
    }
    co_active = -1;
}

static void co_exec()
{
    if (co_active != -1)
    {
        struct co *p = &coroutines[co_active];
        //printf("[log] coroutine %d started.\n", co_active);
        p->func(p->arg);
        //printf("[log] coroutine %d killed.\n", co_active);
        p->state = CO_INVALID;
        co_active = -1;
    }
}

struct co *co_start(const char *name, func_t func, void *arg)
{
    int p;
    for (p = 0; p < CO_MAX; p++)
    {
        if (coroutines[p].state == CO_INVALID)
        {
            //printf("[log]\tnew coroutine start from slot %d\n", p);
            break;
        }
    }
    if (p == CO_MAX)
    {
        //printf("[error]\tno free coroutine slots.\n");
        assert(0);
    }

    co_active = p;

    struct co *co_new = &coroutines[p];

    co_new->name = (char *)name;
    co_new->func = func;
    co_new->arg = arg;
    co_new->state = CO_INITED;
    getcontext(&co_new->ctx);
    co_new->ctx.uc_stack.ss_sp = co_new->stack;
    co_new->ctx.uc_stack.ss_size = sizeof(co_new->stack);
    co_new->ctx.uc_stack.ss_flags = 0;
    co_new->ctx.uc_link = &main_ctx;

    makecontext(&co_new->ctx, (void (*)())(co_exec), 0);
    swapcontext(&(main_ctx), &co_new->ctx);

    // func(arg);
    return co_new;
}

void co_yield()
{
    if (co_active != -1)
    {
        //printf("[log]\treceieve yield.\n");
        struct co *p = &coroutines[co_active];
        p->state = CO_PAUSED;
        co_active = -1;
        swapcontext(&(p->ctx), &main_ctx);
    }
    else
    {
        //printf("[error]\tyield failed.\n");
    }
}

static int co_pool[CO_MAX];
static int co_scheduler()
{
    int co_nr = 0;
    for (int i = 0; i < CO_MAX; i++)
    {
        if (coroutines[i].state == CO_PAUSED)
        {
            co_pool[co_nr] = i;
            co_nr += 1;
        }
    }
    if (co_nr == 0)
    {
        return -1;
    }
    int co_nxt = co_pool[rand() % co_nr];
    //printf("[log]\tswitch to %d.\n", co_nxt);
    return co_nxt;
}

void co_wait(struct co *thd)
{
    //printf("[log]\twaiting.\n");
    while (thd->state != CO_INVALID)
    {
        int co_nxt = co_scheduler();
        if (co_nxt == -1)
        {
            //printf("[error]\tno valid coroutine to wait.");
            assert(0);
        }
        co_active = co_nxt;
        swapcontext(&main_ctx, &coroutines[co_nxt].ctx);
    }
    //printf("[log]\tstop waiting.\n");
}
