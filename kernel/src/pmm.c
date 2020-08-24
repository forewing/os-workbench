#include <common.h>
#include <dlist.h>
#include <klib.h>
#include <kmt.h>
#include <pmm.h>
#include <util.h>

static spinlock_t pmm_spinlock;
static uintptr_t pm_start, pm_end;
static pmm_t pmm_dlist_e;
#define pmm_dlist (&pmm_dlist_e)

uint32_t free_mem;
uint32_t total_mem;

static void pmm_init() {
    kmt->spin_init(&pmm_spinlock, "pmm");

    pm_start = (uintptr_t)_heap.start;
    pm_end = (uintptr_t)_heap.end;

    __DLIST_INIT(pmm_dlist, next, prev);
    pmm_dlist->size = 0;
    pmm_dlist->status = PMM_USED;

    pmm_t* pmm_e = (pmm_t*)pm_start;
    __DLIST_INSERT(pmm_dlist, next, prev, pmm_e);
    pmm_e->size = pm_end - (uintptr_t)(pmm_e + sizeof(pmm_t));
    pmm_e->status = PMM_FREE;

    free_mem = pmm_e->size;
    total_mem = pmm_e->size;
}

static void* _kalloc(size_t size) {
    pmm_t* ptr;
    __DLIST_FOREACH(pmm_dlist, next, ptr) {
        if (ptr->status == PMM_FREE && ptr->size >= size + sizeof(pmm_t)) {
            break;
        }
    }
    if (ptr == pmm_dlist) {
        util_log("kalloc: no more free space.", (uint32_t)size, LOG_WARNING,
                 LOG_NHEX);
        assert(0);
        return NULL;
    }

    ptr->status = PMM_USED;

    // ====================================
    // ptr |         ptr->size
    // ====================================
    //                |
    //                V
    // ====================================
    // ptr |    size   | new | new->size
    // ====================================
    if (ptr->size > size + sizeof(pmm_t)) {
        pmm_t* pmm_new = (pmm_t*)((uintptr_t)ptr + sizeof(pmm_t) + size);
        pmm_new->size = ptr->size - size - sizeof(pmm_t);
        pmm_new->status = PMM_FREE;
        ptr->size = size;
        __DLIST_INSERT(ptr, next, prev, pmm_new);
    }

    void* ret = (void*)((uintptr_t)ptr + sizeof(pmm_t));
    memset(ret, 0, size);

    free_mem -= size + sizeof(pmm_t);
    return ret;
}

static void* kalloc(size_t size) {
    kmt->spin_lock(&pmm_spinlock);
    void* ret = _kalloc(size);
    kmt->spin_unlock(&pmm_spinlock);
    return ret;
}

static int pmm_cmp_equal(pmm_t* a, pmm_t* b) {
    return (a == b) ? 1 : 0;
}

static void _kfree(void* ptr) {
    pmm_t* itr;
    pmm_t* target = (pmm_t*)((uintptr_t)ptr - sizeof(pmm_t));
    __DLIST_FIND_NODE(pmm_dlist, next, itr, target, pmm_cmp_equal);

    if (itr == NULL) {
        util_log("kfree: memory to free is invalid.", (uint32_t)ptr, LOG_ERROR,
                 LOG_NHEX);
        return;
    }

    free_mem += itr->size + sizeof(pmm_t);

    itr->status = PMM_FREE;
    // Case 1 : right part is free
    // ====================================
    // itr |  itr->size | nxt | nxt->size
    // ====================================
    //                |
    //                V
    // ====================================
    // itr |         itr->size
    // ====================================
    if (itr->next->status == PMM_FREE) {
        // util_log("kreee: case 1", 0, LOG_INFO, LOG_NNONE);
        itr->size += (sizeof(pmm_t) + itr->next->size);
        pmm_t* tbd = itr->next;  // Macro arg should be unit
        __DLIST_DELETE(pmm_dlist, tbd, next, prev);
    }

    // Case 2 : left part is free
    // ====================================
    // pre |  pre->size | itr | itr->size
    // ====================================
    //                |
    //                V
    // ====================================
    // pre |         pre->size
    // ====================================
    if (itr->prev->status == PMM_FREE) {
        // util_log("kreee: case 2", 0, LOG_INFO, LOG_NNONE);
        itr->prev->size += (sizeof(pmm_t) + itr->size);
        __DLIST_DELETE(pmm_dlist, itr, next, prev);
    }
}

static void kfree(void* ptr) {
    kmt->spin_lock(&pmm_spinlock);
    _kfree(ptr);
    kmt->spin_unlock(&pmm_spinlock);
}

MODULE_DEF(pmm){
    .init = pmm_init,
    .alloc = kalloc,
    .free = kfree,
};
