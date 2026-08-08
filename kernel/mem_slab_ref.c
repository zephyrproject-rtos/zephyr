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

#define ATOMIC_INIT(i) { (i) }
#define REFCOUNT_INIT(n) { .refs = ATOMIC_INIT(n), }
#define SLAB_RC_HEADER_SIZE WB_UP(sizeof(struct k_mem_slab_ref_header))

typedef struct {
    atomic_t counter;
} refcount_t;

typedef struct {
    refcount_t refcount;
} kref;

struct k_mem_slab_ref_header {
    struct kref kref;
    struct k_mem_slab_ref *owner;
};

/* Helper Functions */
void kref_init(struct kref *kref);
void kref_get(struct kref *kref);
int kref_put(struct kref *kref, void(*release)(struct kref *ref));
int kref_get_unless_zero(struct kref *kref);
atomic_ptr_val_t kref_read(const struct kref *kref);
int kref_put_mutex(struct kref *kref, void (*release)(struct kref *kref), 
                                                        struct k_mutex *k_mutex);

void __refcount_add(int i, refcount_t *r, int *oldp); void __refcount_inc(refcount_t *r, int *oldp); void atomic_set(atomic_t *v, int i);
bool __refcount_add_not_zero(int i, refcount_t *r, int *oldp);
bool __refcount_inc_not_zero(refcount_t *r, int *oldp);
bool __refcount_sub_and_test(int i, refcount_t *r, int *oldp); 
bool __refcount_dec_and_test(refcount_t *r, int *oldp);
void __refcount_dec(refcount_t *r, int *oldp);

atomic_ptr_val_t refcount_read(const refcount_t *r);
void refcount_inc(refcount_t *r);
bool refcount_inc_not_zero(refcount_t *r);
void refcount_add(int i, refcount_t *r);
bool refcount_add_not_zero(int i, refcount_t *r);
bool refcount_dec_and_test(refcount_t *r);
void refcount_dec(refcount_t *r);
int  refcount_sub_and_test(int i, refcount_t *r);
bool refcount_dec_if_one(refcount_t *r);
bool refcount_dec_not_one(refcount_t *r);
bool refcount_dec_and_mutex_lock(refcount_t *r, struct k_mutex *lock);
bool refcount_dec_and_lock(refcount_t *r, struct k_spinlock *lock);

/**
 * 
 */
int k_mem_slab_ref_init(struct k_mem_slab_ref_header *slab_ref_header,
                       void *buffer, size_t block_size, uint32_t num_blocks) {

    kref_init(&slab_ref_header->kref);
    return k_mem_slab_init(&header->slab, buffer, block_size, num_blocks);
}

/**
 *  @brief
 */
int k_mem_slab_ref_alloc(struct k_mem_slab_ref *slab_ref, void **mem, 
						k_timeout_t timeout) {
    void *blk = NULL;
    int ret  = k_mem_slab_alloc(&slab_ref->slab, &blk, timeout);

    if(ret == 0) {
        struct k_mem_slab_ref_header *header = (struct k_mem_slab_ref_header*)blk;
        kref_init(&header->kref);
        header->owner = slab_ref;
        *mem = (uint8_t*)blk + SLAB_RC_HEADER_SIZE;

    }
    k_free(blk);
    return ret;
}

/**
 *  @brief
 */
int k_mem_slab_ref_read_alloc(struct k_mem_slab_ref_header *slab_ref_header, void **mem, 
						k_timeout_t timeout) {
    if(!slab_ref_header || !slab_ref_header->owner) {
        return -EINVAL;
    }

    void *blk = NULL;
    int   ret = k_mem_slab_alloc(slab_ref_header->owner->slab, &blk, timeout);

    if(ret == 0) {
        kref_get(&slab_ref_header->kref);
        *mem = (uint8_t*)blk + SLAB_RC_HEADER_SIZE;
    }
    k_free(blk);
    return ret;
}

void block_release_callback(struct kref *ref) {
    struct k_mem_slab_ref_header *header = CONTAINER_OF(ref, struct k_mem_slab_ref_header,
                                            kref);
    k_mem_slab_free(&header->owner->slab);
    k_free(ref);
}

/**
 *  @brief
 */
void k_mem_slab_ref_free(struct k_mem_slab_ref *slab_ref, void *mem) {
    if(mem == NULL) {
        return -EINVAL;
    }

    struct k_mem_slab_ref_header* header = (struct k_mem_slab_ref_header *)
                                            ((uint8_t*)mem - SLAB_RC_HEADER_SIZE);

    kref_put(&header->kref, block_release_callback);
    k_free(mem);
}

/**
 *  @brief
 */
static inline uint32_t k_mem_slab_ref_max_used_get(struct k_mem_slab_ref_header 
                                                                *slab_ref_header) {
    return k_mem_slab_max_used_get(&slab_ref_header->owner->slab);
}

/**
 *  @brief
 */
static inline uint32_t k_mem_slab_ref_num_free_get(struct k_mem_slab_ref_header 
                                                                *slab_ref_header) {
    return k_mem_slab_num_free_get(&slab_ref_header->owner->slab);
}

/**
 *  @brief
 */
int k_mem_slab_ref_runtime_stats_get(struct k_mem_slab_ref_header *slab_ref_header, 
						struct sys_memory_stats *stats){
    return k_mem_slab_runtime_stats_get(&slab_ref_header->owner->slab, stats);
}

/**
 *  @brief
 */
int k_mem_slab_ref_runtime_stats_reset_max(struct k_mem_slab_ref_header 
                                                *slab_ref_header) {
    return k_mem_slab_runtime_stats_reset_max(&slab_ref_header->owner->slab);
}

/**
 *  @brief 
 */
static inline uint32_t k_mem_slab_ref_refcount_get(struct k_mem_slab_ref_header 
                                                *slab_ref_header) {
    return (uint32_t)(slab_rc_header->kref.refcount.refs.counter);
}

void kref_init(struct kref *kref) {
    refcount_set(&kref->refcount, 1);
}

void kref_get(struct kref *kref) {
    refcount_inc(&kref->refcount);
}

atomic_ptr_val_t kref_read(const struct kref *kref) {
    return refcount_read(&kref->refcount);
}

int kref_put(struct kref *kref, void(*release)(struct kref *ref)) {

    if(refcount_dec_and_test(&kref->refcount)) {
        release(kref);
        return 1;
    }
    return 0;
}

int kref_put_mutex(struct kref *kref, 
    void (*release)(struct kref *kref), struct k_mutex *k_mutex) {
    
    if(refcount_dec_and_mutex_lock(&kref->refcount, k_mutex)){
        release(kref);
        return 1;
    }
    return 0;
}

int kref_get_unless_zero(struct kref *kref) {
    return refcount_inc_not_zero(&kref->refcount);
}

 void __refcount_add(int i, refcount_t *r, int *oldp) {

    int old = atomic_add(&r->refs.counter, i);

    if(oldp) {
        *oldp = old;
    }

    if((!old)){
        refcount_warn_saturate(r, REFCOUNT_ADD_UAF);
    }else if((old < 0 || old + i < 0)){
        refcount_warn_saturate(r, REFCOUNT_ADD_OVF);
    }

}

 void __refcount_inc(refcount_t *r, int *oldp)
{
	__refcount_add(1, r, oldp);
}

 void atomic_set(atomic_t *v, int i) {
    atomic_set(&v->counter, i);
}

 bool __refcount_add_not_zero(int i, refcount_t *r, int *oldp) {
    atomic_val_t old = (atomic_val_t)refcount_read(r);

    do {
        if(!old){ // checks for 0
            break;
        }
    } while (!atomic_cas(&r->refs.counter, old, old + (atomic_val_t)i)); // from zephyr ofc,

    if(oldp){
        *oldp = (int)old;
    }

    if((old < 0 || (int)old + i < 0)){
        // refcount_warn_saturate(r, REFCOUNT_ADD_NOT_ZERO_OVF);
    }

    return (bool)old;
    // if old is zero, would not have succceed,
    // if anything other than that, it would have suceeded
}

 bool __refcount_inc_not_zero(refcount_t *r, int *oldp) {
    return __refcount_add_not_zero(1, r, oldp);
}

 bool __refcount_sub_and_test(int i, refcount_t *r, int *oldp) {
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
        // refcount_warn_saturate(r, REFCOUNT_SUB_UAF);
    }

    return false;
}

 bool __refcount_dec_and_test(refcount_t *r, int *oldp) {
    return __refcount_sub_and_test(1, r, oldp);
}

 void __refcount_dec(refcount_t *r, int *oldp) {
    int old = atomic_sub(&r->refs.counter, 1);

    if(oldp){
        *oldp = old;
    }

    if((old <= 1)){
        refcount_warn_saturate(r, REFCOUNT_DEC_LEAK);
    }
}

atomic_ptr_val_t refcount_read(const refcount_t *r) {
    // atomic_ptr_get belongs to zephyr
    return atomic_ptr_get((atomic_ptr_t*)&r->refs.counter); 
}

void refcount_inc(refcount_t *r) {
    __refcount_inc(r, NULL);
}

bool refcount_inc_not_zero(refcount_t *r) {
    return __refcount_inc_not_zero(r, NULL);
}

void refcount_add(int i, refcount_t *r) {
    __refcount_add(i, r, NULL);
}

bool refcount_add_not_zero(int i, refcount_t *r) {
    return __refcount_add_not_zero(i, r, NULL);
}

bool refcount_dec_and_test(refcount_t *r) {
    return __refcount_dec_and_test(r, NULL);
}

void refcount_dec(refcount_t *r) {
    __refcount_dec(r, NULL);
}

bool refcount_dec_if_one(refcount_t *r) {
    int val = 1;
    return atomic_cas(&r->refs.counter, val, 0);
}

bool refcount_dec_not_one(refcount_t *r) {
    unsigned int new, val = atomic_get(&r->refs.counter); // @atomic_get from zephyr

    do {
        if(unlikely(val == REFCOUNT_SATURATED)) return true;
        if(val == 1) return false;

        new = val - 1;
        if (new > val) { // to detect UAF (use-after-free)
            LOG_WRN_ONCE("refcount_t: underflow use-after-free\n");
        }
    }while(!atomic_cas(&r->refs.counter, val, new)); // #from Zephyr

    return true;
}

bool refcount_dec_and_mutex_lock(refcount_t *r, struct k_mutex *lock) {

    if(refcount_dec_not_one(r)) return false;

    k_mutex_lock(lock, K_NO_WAIT);

    if(!refcount_dec_and_test(r)){
        k_mutex_unlock(lock);
        return false;
    }

    return false;
}

bool refcount_dec_and_lock(refcount_t *r, struct k_spinlock *lock) {
   if(refcount_dec_not_one(r)){
    return false;
   }

   // in zephyr irqsave is builtin with spinlock
   // refcount_dec_and_lock_irqsave() -> separate function is not needed
   k_spin_lock(lock);

   if(!refcount_dec_and_test(r)) {
    k_spin_release(lock);
    return false;
   }

   return true;
}
