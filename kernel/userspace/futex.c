/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/init.h>
#include <ksched.h>
#include <scheduler.h>

/*
 * Kernel futex implementation
 *
 * We use a single queue for all futex waiters. This is lightweight and
 * efficient for a small number of threads but does not scale well to larger
 * numbers waiting on different mutexes, because each wake needs to walk the
 * full queue while holding _sched_spinlock.
 *
 * Potential alternatives for scaling:
 * - Use dedicated queues per futex, which requires a dynamic data structure to
 *   map futex addresses to wait queues.
 * - Create a new data structure to sort by both futex address and priority to
 *   efficiently find the highest priority thread waiting on a given futex.
 * - Keep a small rbtree of active futexes keyed by address, but put the rbnode
 *   in the waiting thread's own TCB instead of a slab pool — the lead waiter
 *   for each futex contributes the node, handed to the next waiter when it
 *   leaves. Per-futex queues, no heap, no fixed table, at the cost of a little
 *   bookkeeping on wait/wake.
 *   https://github.com/zephyrproject-rtos/zephyr/pull/110059#issuecomment-4625886727
 *
 * Further improvements:
 * - Add a futex wait call with priority inheritance, where the futex value is
 *   expected to be the TID of the holder, so that a higher priority thread
 *   waiting for a futex can boost the holders priority.
 */
static _wait_q_t wait_q = Z_WAIT_Q_INIT(&wait_q);
static struct k_spinlock lock;

struct futex_walk_data {
	struct k_thread *head;
	struct k_futex *futex;
	int woken;
	bool wake_all;
};

static void futex_post_walk_op(int status, void *data)
{
	/*
	 * Note: z_sched_wake_thread_locked() is safe
	 * to call here because this walk_op callback
	 * is invoked with _sched_spinlock held.
	 */
	ARG_UNUSED(status);
	struct futex_walk_data *walk_data = data;
	struct k_thread *thread, *next;

	thread = walk_data->head;

	while (thread != NULL) {
		next = thread->next_wake_link;

		(void)z_try_abort_thread_timeout(thread);
		arch_thread_return_value_set(thread, 0);
		z_sched_wake_thread_locked(thread);
		walk_data->woken++;

		thread = next;
	}
}

static int futex_walk_op(struct k_thread *thread, void *data)
{
	struct futex_walk_data *walk_data = data;

	/* Ignore threads waiting on different futexes */
	if (thread->base.futex_pointer == walk_data->futex) {
		/*
		 * If we want to wake all, we add the found thread to the queue.
		 * Otherwise, we keep walk_data->head pointing only to the highest
		 * priority waiter.
		 */
		if (walk_data->wake_all) {
			thread->next_wake_link = walk_data->head;
			walk_data->head = thread;
		} else {
			if (walk_data->head == NULL ||
			    z_is_prio_higher(thread->base.prio, walk_data->head->base.prio)) {
				walk_data->head = thread;
				walk_data->head->next_wake_link = NULL;
			}
		}
	}

	return 0;
}

int z_impl_k_futex_wake(struct k_futex *futex, bool wake_all)
{
	k_spinlock_key_t key;
	struct futex_walk_data walk_data = {
		.head = NULL, .futex = futex, .wake_all = wake_all, .woken = 0};

	key = k_spin_lock(&lock);

	/*
	 * Walk through wait queue to find threads waiting on this futex. Unlike
	 * the events walker, we always do the wake in futex_post_walk_op. In case
	 * wake_all is false, we want to run through the queue to find the thread
	 * with the highest priority waiting on this futex. We cannot use
	 * z_unpend_first_thread because the overall highest priority thread could
	 * be waiting on a different futex.
	 */
	z_sched_waitq_walk(&wait_q, futex_walk_op, futex_post_walk_op, &walk_data);

	if (walk_data.woken == 0) {
		k_spin_unlock(&lock, key);
	} else {
		z_reschedule(&lock, key);
	}

	return walk_data.woken;
}

static inline int z_vrfy_k_futex_wake(struct k_futex *futex, bool wake_all)
{
	if (K_SYSCALL_MEMORY_WRITE(futex, sizeof(struct k_futex)) != 0) {
		return -EACCES;
	}

	return z_impl_k_futex_wake(futex, wake_all);
}
#include <zephyr/syscalls/k_futex_wake_mrsh.c>

int z_impl_k_futex_wait(struct k_futex *futex, int expected, k_timeout_t timeout)
{
	int ret;
	k_spinlock_key_t key;

	key = k_spin_lock(&lock);

	if (atomic_get(&futex->val) != (atomic_val_t)expected) {
		k_spin_unlock(&lock, key);
		return -EAGAIN;
	}

	/*
	 * Store futex pointer we are waiting on, so that it can be used in
	 * futex_walk_op filtering
	 */
	_current->base.futex_pointer = futex;
	ret = z_pend_curr(&lock, key, &wait_q, timeout);
	if (ret == -EAGAIN) {
		ret = -ETIMEDOUT;
	}

	return ret;
}

static inline int z_vrfy_k_futex_wait(struct k_futex *futex, int expected, k_timeout_t timeout)
{
	if (K_SYSCALL_MEMORY_WRITE(futex, sizeof(struct k_futex)) != 0) {
		return -EACCES;
	}

	return z_impl_k_futex_wait(futex, expected, timeout);
}
#include <zephyr/syscalls/k_futex_wait_mrsh.c>
