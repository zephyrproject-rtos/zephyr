/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_KERNEL_INCLUDE_SCHEDULER_H_
#define ZEPHYR_KERNEL_INCLUDE_SCHEDULER_H_

#include <wait_q.h>

/**
 * @brief Initialize the scheduler subsystem
 *
 * This is called from the architecture-specific initialization code, and is not
 * intended to be called by any other code. It is not a public API and should not
 * be called by any code outside of the kernel's own initialization routines.
 */
void z_sched_init(void);

/**
 * @brief Update the scheduler cache and reschedule, releasing the scheduler
 * spinlock.  Callers must hold _sched_spinlock before calling; the lock is
 * released (via reschedule) before this returns.
 *
 * @param key Spinlock key obtained from k_spin_lock(&_sched_spinlock).
 */
void z_sched_lock_reschedule(k_spinlock_key_t key);

/*
 * APIs for working with the Zephyr kernel scheduler. Intended for use in
 * management of IPC objects, either in the core kernel or other IPC
 * implemented by OS compatibility layers, providing basic wait/wake operations
 * with spinlocks used for synchronization.
 *
 * These APIs are public and will be treated as contract, even if the
 * underlying scheduler implementation changes.
 */

/**
 * Callback function called during queue walk by @ref z_sched_waitq_walk
 * for each thread in the wait queue (if not ended earlier)
 *
 * @param thread Thread info data structure
 * @param data   `data` parameter of z_sched_waitq_walk()
 * @return non-zero to end wait queue walk immediately, 0 to continue
 */
typedef int (*_waitq_walk_cb_t)(struct k_thread *thread, void *data);

/**
 * Callback function called after queue walk by @ref z_sched_waitq_walk
 *
 * @param status Wait queue walk status
 * (same value that will be returned to caller of z_sched_waitq_walk())
 * @param data   `data` parameter of z_sched_waitq_walk()
 */
typedef void (*_waitq_post_walk_cb_t)(int status, void *data);

/**
 * @brief Walks the wait queue invoking a callback on each waiting thread
 *
 * This function walks the wait queue invoking the `walk_func` callback function
 * on each waiting thread (except if stopped earlier by a non-zero return value),
 * followed by a single call of `post_func`, all while holding `_sched_spinlock`.
 * This can be useful for routines that need to operate on multiple waiting threads.
 *
 * CAUTION! As a wait queue is of indeterminate length, the scheduler will be
 * locked for an indeterminate amount of time. This may impact system performance.
 * As such, care must be taken when using this function and in the callbacks.
 *
 * @warning @p walk_func may safely remove the thread received as argument from
 * the wait queue only when `CONFIG_WAITQ_SCALABLE=n`.
 *
 * @param wait_q    Identifies the wait queue to walk
 * @param walk_func Callback to invoke for each waiting thread
 * @param post_func Callback to invoke after queue walk (optional)
 * @param data      Custom data passed to the callbacks
 *
 * @return non-zero if walk is terminated by the callback; otherwise 0
 */
int z_sched_waitq_walk(_wait_q_t *wait_q, _waitq_walk_cb_t walk_func,
		       _waitq_post_walk_cb_t post_func, void *data);

/**
 * Wake up a thread pending on the provided wait queue
 *
 * Given a wait_q, wake up the highest priority thread on the queue. If the
 * queue was empty just return false.
 *
 * Otherwise, do the following, in order,  holding _sched_spinlock the entire
 * time so that the thread state is guaranteed not to change:
 * - Set the thread's swap return values to swap_retval and swap_data
 * - un-pend and ready the thread, but do not invoke the scheduler.
 *
 * Repeated calls to this function until it returns false is a suitable
 * way to wake all threads on the queue.
 *
 * It is up to the caller to implement locking such that the return value of
 * this function (whether a thread was woken up or not) does not immediately
 * become stale. Calls to wait and wake on the same wait_q object must have
 * synchronization. Calling this without holding any spinlock is a sign that
 * this API is not being used properly.
 *
 * @param wait_q Wait queue to wake up the highest prio thread
 * @param swap_retval Swap return value for woken thread
 * @param swap_data Data return value to supplement swap_retval. May be NULL.
 * @retval true If a thread was woken up
 * @retval false If the wait_q was empty
 */
bool z_sched_wake(_wait_q_t *wait_q, int swap_retval, void *swap_data);

/**
 * Wakes the specified thread.
 *
 * Given a specific thread, wake it up. This routine assumes that the given
 * thread is not on the timeout queue.
 *
 * @warning Caller must hold _sched_spinlock when calling this function!
 *
 * @param thread Given thread to wake up.
 *
 */
void z_sched_wake_thread_locked(struct k_thread *thread);

/**
 * Wake up all threads pending on the provided wait queue
 *
 * Convenience function to invoke z_sched_wake() on all threads in the queue
 * until there are no more to wake up.
 *
 * @param wait_q Wait queue to wake up the highest prio thread
 * @param swap_retval Swap return value for woken thread
 * @param swap_data Data return value to supplement swap_retval. May be NULL.
 * @retval true If any threads were woken up
 * @retval false If the wait_q was empty
 */
static inline bool z_sched_wake_all(_wait_q_t *wait_q, int swap_retval,
				    void *swap_data)
{
	bool woken = false;

	while (z_sched_wake(wait_q, swap_retval, swap_data)) {
		woken = true;
	}

	/* True if we woke at least one thread up */
	return woken;
}

/**
 * Atomically put the current thread to sleep on a wait queue, with timeout
 *
 * The thread will be added to the provided waitqueue. The lock, which should
 * be held by the caller with the provided key, will be released once this is
 * completely done and we have swapped out.
 *
 * The return value and data pointer is set by whoever woke us up via
 * z_sched_wake.
 *
 * @param lock Address of spinlock to release when we swap out
 * @param key Key to the provided spinlock when it was locked
 * @param wait_q Wait queue to go to sleep on
 * @param timeout Waiting period to be woken up, or K_FOREVER to wait
 *                indefinitely.
 * @param data Storage location for data pointer set when thread was woken up.
 *             May be NULL if not used.
 * @retval Return value set by whatever woke us up, or -EAGAIN if the timeout
 *         expired without being woken up.
 */
int z_sched_wait(struct k_spinlock *lock, k_spinlock_key_t key,
		 _wait_q_t *wait_q, k_timeout_t timeout, void **data);

#endif /* ZEPHYR_KERNEL_INCLUDE_SCHEDULER_H_ */
