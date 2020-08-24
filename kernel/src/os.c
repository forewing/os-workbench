#include <common.h>
#include <devices.h>
#include <klib.h>
#include <kmt.h>
#include <shell.h>
#include <spin.h>
#include <trap.h>
#include <util.h>

static void trap_init();

// void create_test(void* arg) {
//     static int cnt[MAX_CPU];
//     while (1) {
//         if (cnt[_cpu()] % 10000 == 0) {
//             cnt[_cpu()] = 1;
//             util_log("hello", (uint32_t)arg, LOG_INFO, LOG_NHEX);
//         } else {
//             cnt[_cpu()] += 1;
//         }
//     }
// }

// void tty_write_c(device_t* tty, char* text) {
//     tty->ops->write(tty, 0, text, strlen(text));
// }
// void echo_task(void* name) {
//     device_t* tty = dev_lookup(name);
//     while (1) {
//         char line[128], text[128];
//         sprintf(text, "(%s) $ ", name);
//         tty_write_c(tty, text);
//         int nread = tty->ops->read(tty, 0, line, sizeof(line));
//         line[nread - 1] = '\0';
//         sprintf(text, "Echo: %s.\n", line);
//         tty_write_c(tty, text);
//     }
// }

void vfs_test(void* arg);
static void os_init() {
    assert(_ncpu() <= MAX_CPU);
    trap_init();
    kmt->init();
    util_init();
    pmm->init();
    dev->init();
    vfs->init();

    kmt->create(pmm->alloc(sizeof(task_t)), "vfs_test", shell_task, (void*)1);
    // kmt->create(pmm->alloc(sizeof(task_t)), "vfs_test", vfs_test, NULL);
    // kmt->create(pmm->alloc(sizeof(task_t)), "vfs_test", vfs_test, NULL);
    // kmt->create(pmm->alloc(sizeof(task_t)), "vfs_test", vfs_test, NULL);
    // kmt->create(pmm->alloc(sizeof(task_t)), "vfs_test", vfs_test, NULL);
    // for (int i = 0; i < 13; i++){
    //     kmt->create(pmm->alloc(sizeof(task_t)), "fuck", create_test,
    //     (void*)i);
    // }
    // kmt->create(pmm->alloc(sizeof(task_t)), "fuck", echo_task, "tty1");
    // kmt->create(pmm->alloc(sizeof(task_t)), "fuck", create_test, (void*)1);
    // kmt->create(pmm->alloc(sizeof(task_t)), "fuck", create_test, (void*)2);
    // kmt->create(pmm->alloc(sizeof(task_t)), "fuck", create_test, (void*)3);
    // kmt->create(pmm->alloc(sizeof(task_t)), "fuck", echo_task, "tty2");
    // kmt->create(pmm->alloc(sizeof(task_t)), "fuck", echo_task, "tty3");
    // kmt->create(pmm->alloc(sizeof(task_t)), "fuck", echo_task, "tty4");
}

static void hello() {
    // util_log("Hello from cpu.", 0, LOG_INFO, LOG_NNONE);
}

// #define test_ptr_nr 1024
// static void* test_ptrs[MAX_CPU][test_ptr_nr];
// static void alloc_test() {
//     util_log("kalloc test started.", 0, LOG_WARNING, LOG_NNONE);

//     for (int i = 0; i < test_ptr_nr; i++) {
//         test_ptrs[_cpu()][i] = pmm->alloc(1 << 12);
//     }
//     for (int i = 0; i < test_ptr_nr; i++) {
//         pmm->free(test_ptrs[_cpu()][i]);
//     }

//     for (int i = 0; i < test_ptr_nr; i++) {
//         test_ptrs[_cpu()][i] = pmm->alloc(1 << 2);
//     }
//     for (int i = 0; i < test_ptr_nr; i++) {
//         pmm->free(test_ptrs[_cpu()][test_ptr_nr - i - 1]);
//     }

//     for (int i = 0; i < test_ptr_nr; i++) {
//         test_ptrs[_cpu()][i] = pmm->alloc(1 << 14);
//     }
//     for (int i = 0; i < test_ptr_nr; i++) {
//         pmm->free(test_ptrs[_cpu()][i]);
//     }

//     util_log("kalloc test done.", 0, LOG_SUCCESS, LOG_NNONE);
// }

static void os_run() {
    _intr_write(1);

    hello();
    // alloc_test();

    while (1) {
        // util_log("Yield", 0, LOG_INFO, LOG_NNONE);
        _yield();
        // asm volatile("hlt" : : "a"(-1));
    }
}

static trap_handler_t trap_handlers[INT_SEQ_MAX][INT_NR_MAX];
static _Context* trap_handler_invalid(_Event e, _Context* c) {
    // should not reach here
    assert(0);
}
static void trap_init() {
    for (int i = INT_SEQ_MIN; i < INT_SEQ_MAX; i++) {
        for (int j = 0; j < INT_NR_MAX; j++) {
            trap_handlers[i][j].handler = trap_handler_invalid;
            trap_handlers[i][j].valid = 0;
        }
    }
}

static _Context* os_trap(_Event ev, _Context* context) {
    // util_log("TRAP", trap_nr[_cpu()]++, LOG_WARNING, LOG_NHEX);
    _Context* ret = NULL;
    for (int i = INT_SEQ_MIN; i < INT_SEQ_MAX; i++) {
        for (int j = 0; j < INT_NR_MAX; j++) {
            if (trap_handlers[i][j].valid) {
                if (trap_handlers[i][j].event == ev.event ||
                    trap_handlers[i][j].event == _EVENT_NULL) {
                    _Context* next = trap_handlers[i][j].handler(ev, context);
                    // util_log("Trap succ", (uint32_t)next, LOG_SUCCESS,
                    //          LOG_NHEX);
                    if (next != NULL) {
                        ret = next;
                    }
                }
            } else {
                break;
            }
        }
    }
    if (ret == NULL) {
        ret = context;
    }
    return ret;
}

static void os_on_irq(int seq, int event, handler_t handler) {
    int ptr;
    for (ptr = INT_SEQ_MIN; ptr < INT_NR_MAX; ptr++) {
        if (trap_handlers[seq][ptr].valid == 0) {
            break;
        }
    }
    if (ptr == INT_NR_MAX) {
        // No free slot, should enlarge INT_NR_MAX
        assert(0);
        return;
    }
    trap_handlers[seq][ptr].valid = 1;
    trap_handlers[seq][ptr].seq = seq;
    trap_handlers[seq][ptr].event = event;
    trap_handlers[seq][ptr].handler = handler;

    // util_log("Trap init", trap_handlers[seq][ptr].valid, LOG_SUCCESS,
    // LOG_NHEX); util_log("Trap init", seq, LOG_SUCCESS, LOG_NHEX);
    // util_log("Trap init", ptr, LOG_SUCCESS, LOG_NHEX);
}

MODULE_DEF(os){
    .init = os_init,
    .run = os_run,
    .trap = os_trap,
    .on_irq = os_on_irq,
};
