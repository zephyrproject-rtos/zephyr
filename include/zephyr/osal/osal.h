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
#define OSAL_FOREVER ((uint32_t)0xFFFFFFFFU)
/** @brief Do not wait (return immediately). */
#define OSAL_NO_WAIT ((uint32_t)0U)

/** @brief Return codes. */
enum osal_status {
	OSAL_OK = 0,       /**< Operation succeeded. */
	OSAL_ERR = -1,     /**< Generic failure. */
	OSAL_TIMEOUT = -2, /**< Blocking operation timed out. */
	OSAL_NOMEM = -3,   /**< Out of memory. */
	OSAL_INVAL = -4,   /**< Invalid argument. */
	OSAL_ISR = -5,     /**< Called from ISR where not allowed. */
};

/** @brief Opaque spinlock handle. Adapters treat it as a void pointer. */
typedef void *osal_spinlock_t;
/** @brief Opaque mutex handle. Adapters treat it as a void pointer. */
typedef void *osal_mutex_t;
/** @brief Opaque semaphore handle. Adapters treat it as a void pointer. */
typedef void *osal_sem_t;
/** @brief Opaque message-queue handle. Adapters treat it as a void pointer. */
typedef void *osal_queue_t;
/** @brief Opaque thread handle. Adapters treat it as a void pointer. */
typedef void *osal_thread_t;
/** @brief Opaque software-timer handle. Adapters treat it as a void pointer. */
typedef void *osal_timer_t;
/** @brief Opaque event-group handle. Adapters treat it as a void pointer. */
typedef void *osal_event_t;
/** @brief Opaque memory-pool handle. Adapters treat it as a void pointer. */
typedef void *osal_mempool_t;
/** @brief Opaque intrusive-FIFO handle. Adapters treat it as a void pointer. */
typedef void *osal_fifo_t;

/** @brief Thread entry signature (single void* argument). */
typedef void (*osal_thread_entry_t)(void *arg);

/** @brief Software timer callback signature. */
typedef void (*osal_timer_cb_t)(void *arg);

/**
 * @brief Convert a millisecond timeout to a Zephyr @ref k_timeout_t.
 *
 * Recognizes the @ref OSAL_NO_WAIT and @ref OSAL_FOREVER sentinels. Shared by
 * every primitive that blocks.
 *
 * @param timeout_ms Timeout in milliseconds, or a special sentinel value.
 *
 * @return The equivalent @ref k_timeout_t.
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

/**
 * @brief Create a spinlock.
 *
 * @return Spinlock handle, or NULL on allocation failure.
 */
osal_spinlock_t osal_spinlock_create(void);

/**
 * @brief Acquire a spinlock.
 *
 * @param lock Spinlock handle.
 *
 * @return Interrupt key to pass back to osal_spin_unlock(); 0 if @p lock is
 *         NULL.
 */
uint32_t osal_spin_lock(osal_spinlock_t lock);

/**
 * @brief Release a spinlock.
 *
 * @param lock Spinlock handle.
 * @param key Key returned by the matching osal_spin_lock().
 */
void osal_spin_unlock(osal_spinlock_t lock, uint32_t key);

/**
 * @brief Delete a spinlock and free its handle.
 *
 * @param lock Spinlock handle. NULL is a no-op.
 */
void osal_spinlock_delete(osal_spinlock_t lock);

/*
 * Interrupt lock - disables interrupts on the local CPU and returns a key to
 * restore the prior state. No lock object is needed, so this is usable before
 * the kernel is up and for short local-only regions. It maps onto the common
 * foreign "disable interrupts, get a key" wrappers.
 */

/**
 * @brief Disable interrupts on the local CPU.
 *
 * Usable before the kernel is up and for short local-only regions. Not
 * SMP-safe on its own (it does not guard other CPUs); use osal_spin_lock()
 * for data shared across cores.
 *
 * @return Key to pass back to osal_irq_unlock() to restore the prior state.
 */
uint32_t osal_irq_lock(void);

/**
 * @brief Restore the interrupt state saved by osal_irq_lock().
 *
 * @param key Key returned by the matching osal_irq_lock().
 */
void osal_irq_unlock(uint32_t key);

/*
 * Recursive critical section - may be entered re-entrantly by the same CPU.
 * The lock is only released when the outermost exit runs. This bridges
 * foreign APIs whose critical sections nest, since Zephyr's k_spinlock is
 * intentionally non-recursive. On SMP it tracks the owning CPU
 * with an atomic so a recursive enter never re-takes the lock; on non-SMP it
 * is an interrupt lock with a recursion counter.
 */

/**
 * @brief Create a recursive critical section.
 *
 * @return Critical-section handle, or NULL on allocation failure.
 */
osal_spinlock_t osal_crit_create(void);

/**
 * @brief Enter a recursive critical section.
 *
 * May be entered re-entrantly by the same CPU; each enter needs a matching
 * osal_crit_exit().
 *
 * @param lock Critical-section handle. NULL is a no-op.
 */
void osal_crit_enter(osal_spinlock_t lock);

/**
 * @brief Exit a recursive critical section.
 *
 * The section is only released when the outermost exit runs.
 *
 * @param lock Critical-section handle. NULL is a no-op.
 */
void osal_crit_exit(osal_spinlock_t lock);

/**
 * @brief Delete a recursive critical section and free its handle.
 *
 * @param lock Critical-section handle. NULL is a no-op.
 */
void osal_crit_delete(osal_spinlock_t lock);

/*
 * Scheduler lock - prevents the current thread from being preempted by other
 * threads (interrupts are still serviced). Nestable: each lock needs a
 * matching unlock. Maps a foreign "suspend all tasks" primitive.
 */

/**
 * @brief Lock the scheduler to prevent preemption of the current thread.
 *
 * Interrupts are still serviced. Nestable: each lock needs a matching
 * osal_sched_unlock().
 */
void osal_sched_lock(void);

/**
 * @brief Unlock the scheduler.
 */
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

/**
 * @brief Create a mutex.
 *
 * @return Mutex handle, or NULL on allocation failure.
 */
osal_mutex_t osal_mutex_create(void);

/**
 * @brief Create a recursive mutex.
 *
 * Equivalent to osal_mutex_create(): the Zephyr mutex is already recursive.
 *
 * @return Mutex handle, or NULL on allocation failure.
 */
osal_mutex_t osal_mutex_create_recursive(void);

/**
 * @brief Lock a mutex.
 *
 * @param mutex Mutex handle.
 * @param timeout_ms Timeout in milliseconds, @ref OSAL_NO_WAIT or
 *                   @ref OSAL_FOREVER.
 *
 * @retval OSAL_OK Mutex locked.
 * @retval OSAL_TIMEOUT Not locked before the timeout elapsed.
 * @retval OSAL_INVAL @p mutex is NULL.
 * @retval OSAL_ERR Other failure.
 */
int osal_mutex_lock(osal_mutex_t mutex, uint32_t timeout_ms);

/**
 * @brief Unlock a mutex.
 *
 * @param mutex Mutex handle.
 *
 * @retval OSAL_OK Mutex unlocked.
 * @retval OSAL_INVAL @p mutex is NULL.
 * @retval OSAL_ERR Other failure (e.g. not held by the caller).
 */
int osal_mutex_unlock(osal_mutex_t mutex);

/**
 * @brief Delete a mutex and free its handle.
 *
 * @param mutex Mutex handle. NULL is a no-op.
 */
void osal_mutex_delete(osal_mutex_t mutex);

/*
 * -------------------------------------------------------------------------
 * Semaphore (counting; binary is max_count == 1)
 * -------------------------------------------------------------------------
 */

/**
 * @brief Create a counting semaphore.
 *
 * @param max_count Maximum count (use 1 for a binary semaphore).
 * @param init_count Initial count; must not exceed @p max_count.
 *
 * @return Semaphore handle, or NULL on allocation failure or invalid counts.
 */
osal_sem_t osal_sem_create(uint32_t max_count, uint32_t init_count);

/**
 * @brief Take (decrement) a semaphore.
 *
 * @param sem Semaphore handle.
 * @param timeout_ms Timeout in milliseconds, @ref OSAL_NO_WAIT or
 *                   @ref OSAL_FOREVER.
 *
 * @retval OSAL_OK Semaphore taken.
 * @retval OSAL_TIMEOUT Not taken before the timeout elapsed.
 * @retval OSAL_INVAL @p sem is NULL.
 * @retval OSAL_ERR Other failure.
 */
int osal_sem_take(osal_sem_t sem, uint32_t timeout_ms);

/**
 * @brief Give (increment) a semaphore.
 *
 * @param sem Semaphore handle.
 *
 * @retval OSAL_OK Semaphore given.
 * @retval OSAL_INVAL @p sem is NULL.
 */
int osal_sem_give(osal_sem_t sem);

/**
 * @brief Get the current count of a semaphore.
 *
 * @param sem Semaphore handle.
 *
 * @return Current count, or 0 if @p sem is NULL.
 */
uint32_t osal_sem_count(osal_sem_t sem);

/**
 * @brief Reset a semaphore's count to zero.
 *
 * @param sem Semaphore handle. NULL is a no-op.
 */
void osal_sem_reset(osal_sem_t sem);

/**
 * @brief Delete a semaphore and free its handle.
 *
 * @param sem Semaphore handle. NULL is a no-op.
 */
void osal_sem_delete(osal_sem_t sem);

/*
 * -------------------------------------------------------------------------
 * Message queue (fixed-size items, copy semantics)
 * -------------------------------------------------------------------------
 */

/**
 * @brief Create a message queue.
 *
 * @param queue_len Maximum number of items the queue can hold.
 * @param item_size Size of each item in bytes.
 *
 * @return Queue handle, or NULL on allocation failure or zero arguments.
 */
osal_queue_t osal_queue_create(uint32_t queue_len, uint32_t item_size);

/**
 * @brief Send an item to the back of a message queue (copy semantics).
 *
 * @param queue Queue handle.
 * @param item Pointer to the item to copy in.
 * @param timeout_ms Timeout in milliseconds, @ref OSAL_NO_WAIT or
 *                   @ref OSAL_FOREVER.
 *
 * @retval OSAL_OK Item enqueued.
 * @retval OSAL_TIMEOUT Queue full and not enqueued before the timeout.
 * @retval OSAL_INVAL @p queue or @p item is NULL.
 * @retval OSAL_ERR Other failure.
 */
int osal_queue_send(osal_queue_t queue, const void *item, uint32_t timeout_ms);

/**
 * @brief Send an item to the front of a message queue.
 *
 * Never blocks; @p timeout_ms is accepted only for API symmetry.
 *
 * @param queue Queue handle.
 * @param item Pointer to the item to copy in.
 * @param timeout_ms Ignored.
 *
 * @retval OSAL_OK Item enqueued at the front.
 * @retval OSAL_TIMEOUT Queue full.
 * @retval OSAL_INVAL @p queue or @p item is NULL.
 */
int osal_queue_send_to_front(osal_queue_t queue, const void *item, uint32_t timeout_ms);

/**
 * @brief Send an item to a message queue from ISR context.
 *
 * Non-blocking; safe to call from an ISR.
 *
 * @param queue Queue handle.
 * @param item Pointer to the item to copy in.
 *
 * @retval OSAL_OK Item enqueued.
 * @retval OSAL_TIMEOUT Queue full.
 * @retval OSAL_INVAL @p queue or @p item is NULL.
 */
int osal_queue_send_from_isr(osal_queue_t queue, const void *item);

/**
 * @brief Receive (and remove) an item from a message queue.
 *
 * @param queue Queue handle.
 * @param item Buffer of at least item_size bytes to copy the item into.
 * @param timeout_ms Timeout in milliseconds, @ref OSAL_NO_WAIT or
 *                   @ref OSAL_FOREVER.
 *
 * @retval OSAL_OK Item received.
 * @retval OSAL_TIMEOUT Queue empty and nothing arrived before the timeout.
 * @retval OSAL_INVAL @p queue or @p item is NULL.
 * @retval OSAL_ERR Other failure.
 */
int osal_queue_recv(osal_queue_t queue, void *item, uint32_t timeout_ms);

/**
 * @brief Peek at the head item without removing it.
 *
 * Non-blocking; @p timeout_ms is accepted only for API symmetry.
 *
 * @param queue Queue handle.
 * @param item Buffer of at least item_size bytes to copy the item into.
 * @param timeout_ms Ignored.
 *
 * @retval OSAL_OK Item copied out.
 * @retval OSAL_TIMEOUT Queue empty.
 * @retval OSAL_INVAL @p queue or @p item is NULL.
 */
int osal_queue_peek(osal_queue_t queue, void *item, uint32_t timeout_ms);

/**
 * @brief Get the number of items currently in a message queue.
 *
 * @param queue Queue handle.
 *
 * @return Number of queued items, or 0 if @p queue is NULL.
 */
uint32_t osal_queue_count(osal_queue_t queue);

/**
 * @brief Delete a message queue and free its handle.
 *
 * @param queue Queue handle. NULL is a no-op.
 */
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

/**
 * @brief Create an intrusive FIFO.
 *
 * @return FIFO handle, or NULL on allocation failure.
 */
osal_fifo_t osal_fifo_create(void);

/**
 * @brief Append an item to the back of a FIFO.
 *
 * The first word of @p item is used as the link node.
 *
 * @param fifo FIFO handle. NULL is a no-op.
 * @param item Caller-owned object pointer. NULL is a no-op.
 */
void osal_fifo_put(osal_fifo_t fifo, void *item);

/**
 * @brief Prepend an item to the front of a FIFO.
 *
 * @param fifo FIFO handle. NULL is a no-op.
 * @param item Caller-owned object pointer. NULL is a no-op.
 */
void osal_fifo_put_front(osal_fifo_t fifo, void *item);

/**
 * @brief Get (and remove) the head item of a FIFO.
 *
 * @param fifo FIFO handle.
 * @param timeout_ms Timeout in milliseconds, @ref OSAL_NO_WAIT or
 *                   @ref OSAL_FOREVER.
 *
 * @return The dequeued item pointer, or NULL on timeout or if @p fifo is NULL.
 */
void *osal_fifo_get(osal_fifo_t fifo, uint32_t timeout_ms);

/**
 * @brief Remove a specific item from a FIFO.
 *
 * @param fifo FIFO handle.
 * @param item The item pointer to remove.
 *
 * @return true if the item was found and removed, false otherwise.
 */
bool osal_fifo_remove(osal_fifo_t fifo, void *item);

/**
 * @brief Delete a FIFO and free its handle.
 *
 * @param fifo FIFO handle. NULL is a no-op.
 */
void osal_fifo_delete(osal_fifo_t fifo);

/*
 * -------------------------------------------------------------------------
 * Event group (bit-level wait/notify, backed by k_event)
 * -------------------------------------------------------------------------
 * wait_bits returns the matching bits, or 0 on timeout. With wait_all set,
 * all requested bits must be present; otherwise any one suffices. With clear
 * set, the matched bits are cleared atomically on return, so a concurrent
 * set of a matched bit is not lost.
 */

/**
 * @brief Create an event group.
 *
 * @return Event-group handle, or NULL on allocation failure.
 */
osal_event_t osal_event_create(void);

/**
 * @brief Set (post) bits in an event group.
 *
 * @param event Event-group handle.
 * @param bits Bits to set.
 *
 * @return The event's bits before the set, or 0 if @p event is NULL.
 */
uint32_t osal_event_set(osal_event_t event, uint32_t bits);

/**
 * @brief Clear bits in an event group.
 *
 * @param event Event-group handle.
 * @param bits Bits to clear.
 *
 * @return The event's bits before the clear, or 0 if @p event is NULL.
 */
uint32_t osal_event_clear(osal_event_t event, uint32_t bits);

/**
 * @brief Wait for bits in an event group.
 *
 * @param event Event-group handle.
 * @param bits Bits to wait for.
 * @param wait_all If true, all @p bits must be set; otherwise any one suffices.
 * @param clear If true, atomically clear the matched bits on return.
 * @param timeout_ms Timeout in milliseconds, @ref OSAL_NO_WAIT or
 *                   @ref OSAL_FOREVER.
 *
 * @return The matched bits, or 0 on timeout or if @p event is NULL.
 */
uint32_t osal_event_wait(osal_event_t event, uint32_t bits, bool wait_all, bool clear,
			 uint32_t timeout_ms);

/**
 * @brief Delete an event group and free its handle.
 *
 * @param event Event-group handle. NULL is a no-op.
 */
void osal_event_delete(osal_event_t event);

/*
 * -------------------------------------------------------------------------
 * Thread / task
 * -------------------------------------------------------------------------
 * The stack is allocated per the configured allocation policy (from the heap
 * in dynamic mode, from a fixed pool in static mode). Priority is given in the
 * caller's own scale and clamped into the Zephyr preemptive range by the
 * implementation.
 *
 * osal_thread_delete() accepts only a handle returned by
 * osal_thread_create(). Passing a handle obtained from
 * osal_thread_current() (which may be a thread OSAL did not create) is not
 * supported.
 *
 * Neither returning from the entry function nor self-deleting reclaims the
 * control block and stack: another thread must call osal_thread_delete() on
 * the handle to free them. osal_thread_delete() force-aborts the target, so
 * it must not be called on a thread that may be holding an osal mutex, crit
 * or spinlock, as the abort will not release it; ensure the thread has
 * quiesced first.
 */

/**
 * @brief Create and start a thread.
 *
 * @param entry Thread entry function.
 * @param name Thread name, or NULL. Applied only if CONFIG_THREAD_NAME is set.
 * @param arg Argument passed to @p entry.
 * @param stack_size Stack size in bytes.
 * @param priority Priority in the caller's scale (higher means higher
 *                 priority), clamped into the Zephyr preemptive range.
 *
 * @return Thread handle, or NULL on failure.
 */
osal_thread_t osal_thread_create(osal_thread_entry_t entry, const char *name, void *arg,
				 uint32_t stack_size, int priority);

/**
 * @brief Abort a thread and free its control block and stack.
 *
 * @param thread Handle returned by osal_thread_create(). NULL is a no-op.
 */
void osal_thread_delete(osal_thread_t thread);

/**
 * @brief Get a handle to the current thread.
 *
 * @return Handle to the calling thread.
 */
osal_thread_t osal_thread_current(void);

/**
 * @brief Yield the current thread.
 */
void osal_thread_yield(void);

/**
 * @brief Suspend a thread.
 *
 * @param thread Thread handle. NULL is a no-op.
 */
void osal_thread_suspend(osal_thread_t thread);

/**
 * @brief Resume a suspended thread.
 *
 * @param thread Thread handle. NULL is a no-op.
 */
void osal_thread_resume(osal_thread_t thread);

/**
 * @brief Get a thread's priority in the caller's scale.
 *
 * @param thread Thread handle.
 *
 * @return Priority in the caller's scale, or 0 if @p thread is NULL.
 */
int osal_thread_priority_get(osal_thread_t thread);

/**
 * @brief Set a thread's priority.
 *
 * @param thread Thread handle. NULL is a no-op.
 * @param priority Priority in the caller's scale (higher means higher
 *                 priority).
 */
void osal_thread_priority_set(osal_thread_t thread, int priority);

/*
 * -------------------------------------------------------------------------
 * Heap
 * -------------------------------------------------------------------------
 */

/**
 * @brief Allocate memory from the system heap.
 *
 * @param size Number of bytes to allocate.
 *
 * @return Pointer to the allocated memory, or NULL on failure.
 */
void *osal_malloc(size_t size);

/**
 * @brief Allocate and zero an array from the system heap.
 *
 * @param n Number of elements.
 * @param size Size of each element in bytes.
 *
 * @return Pointer to the zeroed memory, or NULL on failure.
 */
void *osal_calloc(size_t n, size_t size);

/**
 * @brief Free memory allocated by osal_malloc() or osal_calloc().
 *
 * @param ptr Pointer to free. NULL is a no-op.
 */
void osal_free(void *ptr);

/*
 * -------------------------------------------------------------------------
 * Memory pool (fixed-size blocks, backed by k_mem_slab)
 * -------------------------------------------------------------------------
 * A faster, fragmentation-free alternative to the heap for repeated
 * allocation of same-size objects. alloc returns NULL on timeout/exhaustion.
 */

/**
 * @brief Create a fixed-block memory pool.
 *
 * @param block_count Number of blocks in the pool.
 * @param block_size Size of each block in bytes (rounded up for alignment).
 *
 * @return Pool handle, or NULL on allocation failure or zero arguments.
 */
osal_mempool_t osal_mempool_create(uint32_t block_count, uint32_t block_size);

/**
 * @brief Allocate a block from a memory pool.
 *
 * @param pool Pool handle.
 * @param timeout_ms Timeout in milliseconds, @ref OSAL_NO_WAIT or
 *                   @ref OSAL_FOREVER.
 *
 * @return Pointer to the block, or NULL on timeout/exhaustion or if @p pool
 *         is NULL.
 */
void *osal_mempool_alloc(osal_mempool_t pool, uint32_t timeout_ms);

/**
 * @brief Return a block to its memory pool.
 *
 * @param pool Pool handle. NULL is a no-op.
 * @param block Block previously returned by osal_mempool_alloc(). NULL is a
 *              no-op.
 */
void osal_mempool_free(osal_mempool_t pool, void *block);

/**
 * @brief Get the number of blocks currently allocated from a pool.
 *
 * @param pool Pool handle.
 *
 * @return Number of blocks in use, or 0 if @p pool is NULL.
 */
uint32_t osal_mempool_used(osal_mempool_t pool);

/**
 * @brief Delete a memory pool and free its handle and backing buffer.
 *
 * @param pool Pool handle. NULL is a no-op.
 */
void osal_mempool_delete(osal_mempool_t pool);

/*
 * -------------------------------------------------------------------------
 * Time / delay
 * -------------------------------------------------------------------------
 * osal_busy_wait_us() spins without yielding and does not advance the system
 * tick, so it must not be relied on to move uptime forward (notably on
 * simulated targets).
 */

/**
 * @brief Sleep the current thread for a number of milliseconds.
 *
 * @param ms Milliseconds to sleep.
 */
void osal_delay_ms(uint32_t ms);

/**
 * @brief Get the system uptime in milliseconds.
 *
 * @return Milliseconds since boot.
 */
uint64_t osal_uptime_ms(void);

/**
 * @brief Busy-wait (spin) for a number of microseconds.
 *
 * Does not yield and does not advance the system tick.
 *
 * @param us Microseconds to spin.
 */
void osal_busy_wait_us(uint32_t us);

/**
 * @brief Get the system uptime in ticks.
 *
 * @return Ticks since boot.
 */
uint64_t osal_tick_count(void);

/**
 * @brief Get the system tick frequency.
 *
 * @return Ticks per second.
 */
uint32_t osal_tick_freq(void);

/*
 * -------------------------------------------------------------------------
 * Context
 * -------------------------------------------------------------------------
 */

/**
 * @brief Test whether the caller is in interrupt context.
 *
 * @return true if called from an ISR, false otherwise.
 */
bool osal_in_isr(void);

/*
 * -------------------------------------------------------------------------
 * Software timer (one-shot or periodic)
 * -------------------------------------------------------------------------
 */

/**
 * @brief Create a software timer.
 *
 * @param cb Callback invoked on expiry.
 * @param arg Argument passed to @p cb.
 * @param periodic If true, the timer repeats; otherwise it is one-shot.
 *
 * @return Timer handle, or NULL on allocation failure.
 */
osal_timer_t osal_timer_create(osal_timer_cb_t cb, void *arg, bool periodic);

/**
 * @brief Start a software timer.
 *
 * For a one-shot timer @p period_ms is the delay until expiry; for a periodic
 * timer it is both the initial delay and the repeat interval.
 *
 * @param timer Timer handle.
 * @param period_ms Period in milliseconds; must be non-zero.
 *
 * @retval OSAL_OK Timer started.
 * @retval OSAL_INVAL @p timer is NULL or @p period_ms is 0.
 */
int osal_timer_start(osal_timer_t timer, uint32_t period_ms);

/**
 * @brief Stop a software timer.
 *
 * @param timer Timer handle.
 *
 * @retval OSAL_OK Timer stopped.
 * @retval OSAL_INVAL @p timer is NULL.
 */
int osal_timer_stop(osal_timer_t timer);

/**
 * @brief Stop and delete a software timer and free its handle.
 *
 * @param timer Timer handle. NULL is a no-op.
 */
void osal_timer_delete(osal_timer_t timer);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_OSAL_OSAL_H_ */
