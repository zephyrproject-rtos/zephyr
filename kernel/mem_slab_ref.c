/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h> 
#include <zephyr/kernel_structs.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/init.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/iterable_sections.h>
#include <string.h>
/* private kernel APIs */
#include <ksched.h>
#include <wait_q.h>

#define ATOMIC_INIT_n(i) { (i) }
#define REFCOUNT_INIT_n(n) { .refs = ATOMIC_INIT_n(n), }
#define SLAB_RC_HEADER_SIZE WB_UP(sizeof(struct k_mem_slab_rc_header))

typedef struct {
    atomic_t counter;
} refcount_t;

typedef struct {
    refcount_t refcount;
} kref;

struct k_mem_slab_rc_header {
    struct kref kref;
    struct k_mem_slab_rc *owner;
};

/* Helper Functions */
void kref_init_n(struct kref *kref);
void kref_get_n(struct kref *kref);
int kref_put_n(struct kref *kref, void(*release)(struct kref *ref));
int kref_get_unless_zero_n(struct kref *kref);
atomic_ptr_val_t kref_read_n(const struct kref_n *kref);
int kref_put_mutex_n(struct kref *kref, void (*release)(struct kref_n *kref), 
                                                        struct k_mutex *k_mutex);

void __refcount_add_n(int i, refcount_n_t *r, int *oldp);
void __refcount_inc_n(refcount_n_t *r, int *oldp);
void atomic_set_n(atomic_n_t *v, int i);
bool __refcount_add_not_zero_n(int i, refcount_n_t *r, int *oldp);
bool __refcount_inc_not_zero_n(refcount_n_t *r, int *oldp);
bool __refcount_sub_and_test_n(int i, refcount_n_t *r, int *oldp); 
bool __refcount_dec_and_test_n(refcount_n_t *r, int *oldp);
void __refcount_dec_n(refcount_n_t *r, int *oldp);

atomic_ptr_val_t refcount_read_n(const refcount_n_t *r);
void refcount_inc_n(refcount_n_t *r);
bool refcount_inc_not_zero_n(refcount_n_t *r);
void refcount_add_n(int i, refcount_n_t *r);
bool refcount_add_not_zero(int i, refcount_n_t *r);
bool refcount_dec_and_test_n(refcount_n_t *r);
void refcount_dec_n(refcount_n_t *r);
int  refcount_sub_and_test_n(int i, refcount_n_t *r);
bool refcount_dec_if_one_n(refcount_n_t *r);
bool refcount_dec_not_one_n(refcount_n_t *r);
bool refcount_dec_and_mutex_lock_n(refcount_n_t *r, struct k_mutex *lock);
bool refcount_dec_and_lock(refcount_n_t *r, struct k_spinlock *lock);



int k_mem_slab_ref_init(struct k_mem_slab_ref_header *slab_ref_header,
                       void *buffer, size_t block_size, uint32_t num_blocks);

/**
 *  @brief
 */
int k_mem_slab_ref_alloc(struct k_mem_slab_ref *slab_ref, void **mem, 
						k_timeout_t timeout) {
    void *blk = NULL;
    int ret  = k_mem_slab_alloc(&slab_ref->slab, &blk, timeout);

    if(ret == 0) {
        struct k_mem_slab_rc_header *header = (struct k_mem_slab_rc_header*)blk;

        kref_init(&header->kref);
        header->owner = slab_ref;

        *mem = (uint8_t*)blk + SLAB_RC_HEADER_SIZE;

    }
    return ret;
}

/**
 *  @brief
 */
void k_mem_slab_ref_free(struct k_mem_slab_ref *slab_ref, void *mem);

/**
 *  @brief
 */
static inline uint32_t k_mem_slab_ref_num_used_get(struct k_mem_slab_ref *slab_ref);

/**
 *  @brief
 */
static inline uint32_t k_mem_slab_ref_max_used_get(struct k_mem_slab_ref *slab_ref);

/**
 *  @brief
 */
static inline uint32_t k_mem_slab_ref_num_free_get(struct k_mem_slab_ref *slab_ref);

/**
 *  @brief
 */
int k_mem_slab_ref_runtime_stats_get(struct k_mem_slab_ref *slab_ref, 
						struct sys_memory_stats *stats);

/**
 *  @brief
 */
int k_mem_slab_ref_runtime_stats_reset_max(struct k_mem_slab_ref *slab_ref);

/**
 *  @brief 
 */
static inline uint32_t k_mem_slab_ref_refcount_get(struct 
										k_mem_slab_rc_header *slab_rc_header) {
    return (uint32_t)(slab_rc_header->kref.refcount.refs.counter);
}

void kref_init_n(struct kref_n *kref) {
    refcount_set_n(&kref->refcount, 1);
}

void kref_get_n(struct kref_n *kref) {
    refcount_inc_n(&kref->refcount);
}

atomic_ptr_val_t kref_read_n(const struct kref_n *kref) {
    return refcount_read_n(&kref->refcount);
}

int kref_put_n(struct kref_n *kref, void(*release)(struct kref_n *ref)) {

    if(refcount_dec_and_test_n(&kref->refcount)) {
        release(kref);
        return 1;
    }
    return 0;
}

int kref_put_mutex_n(struct kref_n *kref, 
    void (*release)(struct kref_n *kref), struct k_mutex *k_mutex) {
    
    if(refcount_dec_and_mutex_lock_n(&kref->refcount, k_mutex)){
        release(kref);
        return 1;
    }
    return 0;
}

int kref_get_unless_zero_n(struct kref_n *kref) {
    return refcount_inc_not_zero_n(&kref->refcount);
}

 void __refcount_add_n(int i, refcount_n_t *r, int *oldp) {

    int old = atomic_add(&r->refs.counter, i);

    if(oldp) {
        *oldp = old;
    }

    if((!old)){
        refcount_warn_saturate_n(r, REFCOUNT_ADD_UAF);
    }else if((old < 0 || old + i < 0)){
        refcount_warn_saturate_n(r, REFCOUNT_ADD_OVF);
    }

}

// https://github.com/torvalds/linux/blob/master/include/linux/refcount.h#L364
 void __refcount_inc_n(refcount_n_t *r, int *oldp)
{
	__refcount_add_n(1, r, oldp);
}

// https://github.com/torvalds/linux/blob/master/tools/arch/x86/include/asm/atomic.h#L39
 void atomic_set_n(atomic_n_t *v, int i) {
    atomic_set(&v->counter, i);
}

// https://github.com/torvalds/linux/blob/master/include/linux/refcount.h#L174
 bool __refcount_add_not_zero_n(int i, refcount_n_t *r, int *oldp) {
    atomic_val_t old = (atomic_val_t)refcount_read_n(r);

    do {
        if(!old){ // checks for 0
            break;
        }
    } while (!atomic_cas(&r->refs.counter, old, old + (atomic_val_t)i)); // from zephyr ofc,

    if(oldp){
        *oldp = (int)old;
    }

    if((old < 0 || (int)old + i < 0)){
        refcount_warn_saturate_n(r, REFCOUNT_ADD_NOT_ZERO_OVF);
    }

    return (bool)old;
    // if old is zero, would not have succceed,
    // if anything other than that, it would have suceeded
}

// https://github.com/torvalds/linux/blob/master/include/linux/refcount.h#L315
 bool __refcount_inc_not_zero_n(refcount_n_t *r, int *oldp) {
    return __refcount_add_not_zero_n(1, r, oldp);
}

// https://github.com/torvalds/linux/blob/master/include/linux/refcount.h#L387
 bool __refcount_sub_and_test_n(int i, refcount_n_t *r, int *oldp) {
    int old = atomic_sub(&r->refs.counter, i); // from zephyr

    if (oldp) {
        *oldp = old;
    }

    if (old > 0 && old == i) {
        barrier_dmem_fence_full(); // #from_zephyr
        /*
        * acts as the barrier -> refer notion for more details
        */ 
        return true;
    }

    if((old <= 0 || old - i < 0)) {
        refcount_warn_saturate_n(r, REFCOUNT_SUB_UAF);
    }

    return false;
}

// https://github.com/torvalds/linux/blob/master/include/linux/refcount.h#L430
 bool __refcount_dec_and_test_n(refcount_n_t *r, int *oldp) {
    return __refcount_sub_and_test_n(1, r, oldp);
}

// https://github.com/torvalds/linux/blob/master/include/linux/refcount.h#L453
 void __refcount_dec_n(refcount_n_t *r, int *oldp) {
    int old = atomic_sub(&r->refs.counter, 1);

    if(oldp){
        *oldp = old;
    }

    if((old <= 1)){
        refcount_warn_saturate_n(r, REFCOUNT_DEC_LEAK);
    }
}

atomic_ptr_val_t refcount_read_n(const refcount_n_t *r) {
    // atomic_ptr_get belongs to zephyr
    return atomic_ptr_get((atomic_ptr_t*)&r->refs.counter); 
}

// https://github.com/torvalds/linux/blob/master/include/linux/refcount.h#L381
void refcount_inc_n(refcount_n_t *r) {
    __refcount_inc_n(r, NULL);
}

// https://github.com/torvalds/linux/blob/master/include/linux/refcount.h#L333
bool refcount_inc_not_zero_n(refcount_n_t *r) {
    return __refcount_inc_not_zero_n(r, NULL);
}

// https://github.com/torvalds/linux/blob/master/include/linux/refcount.h#L310
void refcount_add_n(int i, refcount_n_t *r) {
    __refcount_add_n(i, r, NULL);
}

// https://github.com/torvalds/linux/blob/master/include/linux/refcount.h#L210
bool refcount_add_not_zero(int i, refcount_n_t *r) {
    return __refcount_add_not_zero_n(i, r, NULL);
}

// https://github.com/torvalds/linux/blob/master/include/linux/refcount.h#L448
bool refcount_dec_and_test_n(refcount_n_t *r) {
    return __refcount_dec_and_test_n(r, NULL);
}

// https://github.com/torvalds/linux/blob/master/include/linux/refcount.h#L474
void refcount_dec_n(refcount_n_t *r) {
    __refcount_dec_n(r, NULL);
}

// https://github.com/torvalds/linux/blob/master/lib/refcount.c#L55
bool refcount_dec_if_one_n(refcount_n_t *r) {
    int val = 1;
    return atomic_cas(&r->refs.counter, val, 0);
}

// https://github.com/torvalds/linux/blob/master/lib/refcount.c#L74
bool refcount_dec_not_one_n(refcount_n_t *r) {
    unsigned int new, val = atomic_get(&r->refs.counter); // @atomic_get from zephyr

    do {
        if(unlikely(val == REFCOUNT_SATURATED)) return true;
        if(val == 1) return false;

        new = val - 1;
        if (new > val) { // to detect UAF (use-after-free)
            LOG_WRN_ONCE("refcount_n_t: underflow use-after-free\n");
        }
    }while(!atomic_cas(&r->refs.counter, val, new)); // #from Zephyr

    return true;
}

// https://github.com/torvalds/linux/blob/master/lib/refcount.c#L113
bool refcount_dec_and_mutex_lock_n(refcount_n_t *r, struct k_mutex *lock) {

    if(refcount_dec_not_one_n(r)) return false;

    k_mutex_lock(lock, K_NO_WAIT);

    if(!refcount_dec_and_test_n(r)){
        k_mutex_unlock(lock);
        return false;
    }

    return false;
}

// https://github.com/torvalds/linux/blob/master/lib/refcount.c#L144
bool refcount_dec_and_lock(refcount_n_t *r, struct k_spinlock *lock) {
   if(refcount_dec_not_one_n(r)){
    return false;
   }

   // in zephyr irqsave is builtin with spinlock
   // refcount_dec_and_lock_irqsave() -> separate function is not needed
   k_spin_lock(lock);

   if(!refcount_dec_and_test_n(r)) {
    k_spin_release(lock);
    return false;
   }

   return true;
}
