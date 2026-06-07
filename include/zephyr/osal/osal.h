/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Zephyr OS abstraction layer.
 *
 * External modules and binary blobs are commonly built against a foreign
 * RTOS API or OS abstraction shim. Each one re-implements the same handful
 * of Zephyr kernel translations (allocate a control block, init it, convert
 * a millisecond timeout to a k_timeout_t, ...). This layer centralizes those
 * translations so an adapter shrinks to a thin name/return-code mapping.
 *
 * Timeouts are expressed in milliseconds for portability across foreign
 * APIs. Two special values are recognized:
 *  - OSAL_NO_WAIT  : return immediately (K_NO_WAIT)
 *  - OSAL_FOREVER  : block indefinitely (K_FOREVER)
 */

#ifndef ZEPHYR_INCLUDE_OSAL_OSAL_H_
#define ZEPHYR_INCLUDE_OSAL_OSAL_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief OS Abstraction Layer (OSAL) API
 * @defgroup osal OS Abstraction Layer API
 * @since 4.5.0
 * @version 0.1.0
 * @ingroup os_services
 * @{
 */

/** @brief Wait forever (block indefinitely). */
#define OSAL_FOREVER  ((uint32_t)0xFFFFFFFFU)
/** @brief Do not wait (return immediately). */
#define OSAL_NO_WAIT  ((uint32_t)0U)

/** @brief Return codes. */
enum osal_status {
	OSAL_OK = 0,        /**< Operation succeeded. */
	OSAL_ERR = -1,      /**< Generic failure. */
	OSAL_TIMEOUT = -2,  /**< Blocking operation timed out. */
	OSAL_NOMEM = -3,    /**< Out of memory. */
	OSAL_INVAL = -4,    /**< Invalid argument. */
	OSAL_ISR = -5,      /**< Called from ISR where not allowed. */
};

/** @brief Opaque handle types. Adapters treat these as void pointers. */
typedef void *osal_spinlock_t;
typedef void *osal_mutex_t;
typedef void *osal_sem_t;
typedef void *osal_queue_t;
typedef void *osal_thread_t;
typedef void *osal_timer_t;
typedef void *osal_event_t;
typedef void *osal_mempool_t;
typedef void *osal_fifo_t;

/** @brief Thread entry signature (single void* argument). */
typedef void (*osal_thread_entry_t)(void *arg);

/** @brief Software timer callback signature. */
typedef void (*osal_timer_cb_t)(void *arg);

/*
 * Timeout helpers - convert a millisecond value (with the OSAL_* sentinels)
 * into a Zephyr k_timeout_t. Shared by every primitive that blocks.
 */
static inline k_timeout_t osal_ms_to_timeout(uint32_t timeout_ms)
{
	if (timeout_ms == OSAL_NO_WAIT) {
		return K_NO_WAIT;
	}
	if (timeout_ms == OSAL_FOREVER) {
		return K_FOREVER;
	}
	return K_MSEC(timeout_ms);
}

/*
 * -------------------------------------------------------------------------
 * Spinlock / critical section
 * -------------------------------------------------------------------------
 * Backed by k_spinlock, so it is SMP-safe (it guards other CPUs, not just
 * the local interrupt state). The lock is caller-owned, matching foreign
 * APIs that pass an explicit lock object. The lock returned by
 * osal_spin_lock() must be released with the key it returns.
 */
osal_spinlock_t osal_spinlock_create(void);
uint32_t osal_spin_lock(osal_spinlock_t lock);
void osal_spin_unlock(osal_spinlock_t lock, uint32_t key);
void osal_spinlock_delete(osal_spinlock_t lock);

/*
 * Interrupt lock - disables interrupts on the local CPU and returns a key to
 * restore the prior state. No lock object is needed, so this is usable before
 * the kernel is up and for short local-only regions. It maps onto the common
 * foreign "disable interrupts, get a key" wrappers.
 *
 * WARNING: this is NOT SMP-safe on its own (it does not guard other CPUs). Use
 * osal_spin_lock() for data shared across cores.
 */
uint32_t osal_irq_lock(void);
void osal_irq_unlock(uint32_t key);

/*
 * Recursive critical section - may be entered re-entrantly by the same CPU.
 * The lock is only released when the outermost exit runs. This bridges
 * foreign APIs whose critical sections nest, since Zephyr's k_spinlock is
 * intentionally non-recursive. On SMP it tracks the owning CPU
 * with an atomic so a recursive enter never re-takes the lock; on non-SMP it
 * is an interrupt lock with a recursion counter.
 */
osal_spinlock_t osal_crit_create(void);
void osal_crit_enter(osal_spinlock_t lock);
void osal_crit_exit(osal_spinlock_t lock);
void osal_crit_delete(osal_spinlock_t lock);

/*
 * Scheduler lock - prevents the current thread from being preempted by other
 * threads (interrupts are still serviced). Nestable: each lock needs a
 * matching unlock. Maps a foreign "suspend all tasks" primitive.
 */
void osal_sched_lock(void);
void osal_sched_unlock(void);

/*
 * -------------------------------------------------------------------------
 * Mutex
 * -------------------------------------------------------------------------
 * Zephyr k_mutex is recursive and priority-inheriting by default, so the
 * recursive and non-recursive create paths share an implementation. The
 * recursive variants are provided so foreign APIs that distinguish them
 * map cleanly.
 */
osal_mutex_t osal_mutex_create(void);
osal_mutex_t osal_mutex_create_recursive(void);
int osal_mutex_lock(osal_mutex_t mutex, uint32_t timeout_ms);
int osal_mutex_unlock(osal_mutex_t mutex);
void osal_mutex_delete(osal_mutex_t mutex);

/*
 * -------------------------------------------------------------------------
 * Semaphore (counting; binary is max_count == 1)
 * -------------------------------------------------------------------------
 */
osal_sem_t osal_sem_create(uint32_t max_count, uint32_t init_count);
int osal_sem_take(osal_sem_t sem, uint32_t timeout_ms);
int osal_sem_give(osal_sem_t sem);
uint32_t osal_sem_count(osal_sem_t sem);
void osal_sem_reset(osal_sem_t sem);
void osal_sem_delete(osal_sem_t sem);

/*
 * -------------------------------------------------------------------------
 * Message queue (fixed-size items, copy semantics)
 * -------------------------------------------------------------------------
 */
osal_queue_t osal_queue_create(uint32_t queue_len, uint32_t item_size);
int osal_queue_send(osal_queue_t queue, const void *item, uint32_t timeout_ms);
int osal_queue_send_to_front(osal_queue_t queue, const void *item, uint32_t timeout_ms);
int osal_queue_send_from_isr(osal_queue_t queue, const void *item);
int osal_queue_recv(osal_queue_t queue, void *item, uint32_t timeout_ms);
int osal_queue_peek(osal_queue_t queue, void *item, uint32_t timeout_ms);
uint32_t osal_queue_count(osal_queue_t queue);
void osal_queue_delete(osal_queue_t queue);

/*
 * -------------------------------------------------------------------------
 * Intrusive FIFO (pointer-passing linked queue, backed by k_queue)
 * -------------------------------------------------------------------------
 * Unlike the copy-semantics message queue, this enqueues caller-owned object
 * pointers without copying. The first word of each object is reserved by the
 * queue for its link, so callers must leave sizeof(void *) free at the start
 * of the queued object. Supports prepend and arbitrary removal, which the
 * message queue cannot. Useful for event queues that pass embedded list nodes.
 */
osal_fifo_t osal_fifo_create(void);
void osal_fifo_put(osal_fifo_t fifo, void *item);
void osal_fifo_put_front(osal_fifo_t fifo, void *item);
void *osal_fifo_get(osal_fifo_t fifo, uint32_t timeout_ms);
bool osal_fifo_remove(osal_fifo_t fifo, void *item);
void osal_fifo_delete(osal_fifo_t fifo);

/*
 * -------------------------------------------------------------------------
 * Event group (bit-level wait/notify, backed by k_event)
 * -------------------------------------------------------------------------
 * wait_bits returns the matching bits, or 0 on timeout. With wait_all set,
 * all requested bits must be present; otherwise any one suffices. With clear
 * set, the matched bits are cleared on return.
 *
 * Note: the clear is performed after the wait returns and is not atomic with
 * respect to a concurrent setter. This is safe for the common single-consumer
 * pattern; with multiple consumers of overlapping bits, clear externally under
 * your own lock.
 */
osal_event_t osal_event_create(void);
uint32_t osal_event_set(osal_event_t event, uint32_t bits);
uint32_t osal_event_clear(osal_event_t event, uint32_t bits);
uint32_t osal_event_wait(osal_event_t event, uint32_t bits, bool wait_all,
			      bool clear, uint32_t timeout_ms);
void osal_event_delete(osal_event_t event);

/*
 * -------------------------------------------------------------------------
 * Thread / task
 * -------------------------------------------------------------------------
 * The stack is allocated from the heap. Priority is given in the
 * caller's own scale and clamped into the Zephyr cooperative/preemptive
 * range by the implementation.
 *
 * osal_thread_delete() accepts only a handle returned by
 * osal_thread_create(). Passing a handle obtained from
 * osal_thread_current() (which may be a thread OSAL did not create) is not
 * supported. A thread should prefer to return from its entry function rather
 * than self-delete, since a self-delete cannot reclaim its own control block.
 */
osal_thread_t osal_thread_create(osal_thread_entry_t entry, const char *name,
					   void *arg, uint32_t stack_size, int priority);
void osal_thread_delete(osal_thread_t thread);
osal_thread_t osal_thread_current(void);
void osal_thread_yield(void);
void osal_thread_suspend(osal_thread_t thread);
void osal_thread_resume(osal_thread_t thread);
int osal_thread_priority_get(osal_thread_t thread);
void osal_thread_priority_set(osal_thread_t thread, int priority);

/*
 * -------------------------------------------------------------------------
 * Heap
 * -------------------------------------------------------------------------
 */
void *osal_malloc(size_t size);
void *osal_calloc(size_t n, size_t size);
void osal_free(void *ptr);

/*
 * -------------------------------------------------------------------------
 * Memory pool (fixed-size blocks, backed by k_mem_slab)
 * -------------------------------------------------------------------------
 * A faster, fragmentation-free alternative to the heap for repeated
 * allocation of same-size objects. alloc returns NULL on timeout/exhaustion.
 */
osal_mempool_t osal_mempool_create(uint32_t block_count, uint32_t block_size);
void *osal_mempool_alloc(osal_mempool_t pool, uint32_t timeout_ms);
void osal_mempool_free(osal_mempool_t pool, void *block);
uint32_t osal_mempool_used(osal_mempool_t pool);
void osal_mempool_delete(osal_mempool_t pool);

/*
 * -------------------------------------------------------------------------
 * Time / delay
 * -------------------------------------------------------------------------
 */
void osal_delay_ms(uint32_t ms);
uint64_t osal_uptime_ms(void);
void osal_busy_wait_us(uint32_t us);
uint64_t osal_tick_count(void);
uint32_t osal_tick_freq(void);

/*
 * -------------------------------------------------------------------------
 * Context
 * -------------------------------------------------------------------------
 */
bool osal_in_isr(void);

/*
 * -------------------------------------------------------------------------
 * Software timer (one-shot or periodic)
 * -------------------------------------------------------------------------
 */
osal_timer_t osal_timer_create(osal_timer_cb_t cb, void *arg, bool periodic);
int osal_timer_start(osal_timer_t timer, uint32_t period_ms);
int osal_timer_stop(osal_timer_t timer);
void osal_timer_delete(osal_timer_t timer);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_OSAL_OSAL_H_ */
