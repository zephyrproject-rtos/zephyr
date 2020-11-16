/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_SCHEDULER_H
#define ZEPHYR_INCLUDE_SYS_SCHEDULER_H

#include <kernel.h>
#include <sys_clock.h>
#include <wait_q.h>

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
 * Callback function prototype for k_sched_wake_one/wake_all
 *
 * If a thread is woken up, the callback function is called with the thread
 * as an argument. The sched_spinlock is held the entire time, ensuring that
 * thread state doesn't suddenly change (from some nested IRQ or activity on
 * another CPU) when the callback is modifying the thread's state.
 *
 * It is only inside these callbacks that it is safe to inspect or modify
 * the thread that was woken up.
 */
typedef void (*k_sched_wake_cb_t)(struct k_thread *thread, void *obj,
				  void *context);

/**
 * Wake up a thread pending on the provided wait queue, with callback
 *
 * Given a wait_q, wake up the highest priority thread on the queue. If the
 * queue was empty just return false.
 *
 * Otherwise, do the following, in order,  holding sched_spinlock the entire
 * time so that the thread state is guaranteed not to change:
 * - if the callback function is provided, run the callback function with
 *   the provided object and context pointers
 * - Set the thread's swap return values to swap_retval and swap_data
 * - un-pend and ready the thread, but do not invoke the scheduler.
 *
 * It is up to the caller to implement locking such that the return value of
 * this function (whether a thread was woken up or not) does not immediately
 * become stale.
 *
 * Repeated calls to this function until it returns false is a suitable
 * way to wake all threads on the queue.
 *
 * It is up to the caller to implement locking such that the return value of
 * this function does not immediately become stale. Calls to wait and wake on
 * the same wait_q object must have synchronization. Calling this without
 * holding any spinlock is a sign that this API is not being used properly.
 *
 * @param wait_q Wait queue to wake up the highest prio thread
 * @param swap_retval Swap return value for woken thread
 * @param swap_data Data return value to supplement swap_retval. May be NULL.
 * @param cb Callback function, or NULL in which case no callback is invoked.
 * @param obj Kernel object associated with this wake operation, passed to cb.
 *        Only used by the callback. May be NULL.
 * @param context Additional context pointer associated with this wake
 *        operation. Only used by the callback. May be NULL.
 * @retval true If a thread waa woken up
 * @retval false If the wait_q was empty
 */
bool k_sched_callback_wake(_wait_q_t *wait_q, int swap_retval, void *swap_data,
			   k_sched_wake_cb_t cb, void *obj, void *context);

/**
 * Wake up a thread pending on the provided wait queue
 *
 * Convenient function to k_sched_callback_wake(), for the very common case
 * where no callback is required.
 *
 * @param wait_q Wait queue to wake up the highest prio thread
 * @param swap_retval Swap return value for woken thread
 * @param swap_data Data return value to supplement swap_retval. May be NULL.
 * @retval true If a thread waa woken up
 * @retval false If the wait_q was empty
 */
static inline bool k_sched_wake(_wait_q_t *wait_q, int swap_retval,
				void *swap_data)
{
	return k_sched_callback_wake(wait_q, swap_retval, swap_data,
				     NULL, NULL, NULL);
}

/**
 * Wake up all threads pending on the provided wait queue
 *
 * Convenience function to invoke k_sched_wake() on all threads in the queue
 * until there are no more to wake up.
 *
 * @param wait_q Wait queue to wake up the highest prio thread
 * @param swap_retval Swap return value for woken thread
 * @param swap_data Data return value to supplement swap_retval. May be NULL.
 * @retval true If any threads were woken up
 * @retval false If the wait_q was empty
 */
static inline bool k_sched_wake_all(_wait_q_t *wait_q, int swap_retval,
				    void *swap_data)
{
	bool woken = false;

	while (k_sched_wake(wait_q, swap_retval, swap_data)) {
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
 * k_sched_wake.
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
int k_sched_wait(struct k_spinlock *lock, k_spinlock_key_t key,
		 _wait_q_t *wait_q, k_timeout_t timeout, void **data);

/**
 * Atomically invoke a scheduling decision
 *
 * Waking up a thread with APIs like k_sched_wake() un-pends and readies
 * the woken up thread, but does not implicitly invoke the scheduler to
 * immediately schedule them. This allows other tasks to be performed in
 * between.
 *
 * When called, the kernel may context switch to some other thread,
 * releasing the lock once we have switched out. The lock will never
 * be held when this returns.
 *
 * If the caller is a co-operative thread, or the next thread to run on this
 * CPU is otherwise still the current thread, this just unlocks the spinlock
 * and returns.
 *
 * Most users will use the same lock here that synchronizes k_sched_wait()
 * and k_sched_wake() calls.
 *
 * It is safe (albeit very slightly wasteful) to call this even if no thread
 * was woken up.
 *
 * @param lock Address of spinlock to release when we swap out
 * @param key Key to the provided spinlock when it was locked
 */
void k_sched_invoke(struct k_spinlock *lock, k_spinlock_key_t key);

#endif /* ZEPHYR_INCLUDE_SCHEDULER_H */
