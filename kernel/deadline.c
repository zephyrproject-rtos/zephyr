/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief EDF (Earliest Deadline First) scheduling support
 *
 * Implements the public k_thread_deadline_set() and
 * k_thread_absolute_deadline_set() APIs.  The run-queue manipulation
 * required to keep deadline ordering is delegated back to the scheduler
 * via z_sched_prio_deadline_set() so that sched.c keeps full ownership
 * of the run-queue data structures.
 */

#include <zephyr/kernel.h>
#include <ksched.h>
#include <run_q.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/logging/log.h>

/**
 * @brief Atomically update a thread's prio_deadline in the run queue.
 *
 * If the thread is currently queued the scheduler re-inserts it so that
 * run-queue ordering (including rbtree invariants) is preserved.  May be
 * called from any context; takes and releases _sched_spinlock internally.
 *
 * @param thread   Thread whose deadline is being updated.
 * @param deadline New absolute deadline value (raw cycle counter).
 */
void z_sched_prio_deadline_set(struct k_thread *thread, int deadline)
{
	K_SPINLOCK(&_sched_spinlock) {
		if (z_is_thread_queued(thread)) {
			dequeue_thread(thread);
			thread->base.prio_deadline = deadline;
			queue_thread(thread);
		} else {
			thread->base.prio_deadline = deadline;
		}
	}
}

void z_impl_k_thread_absolute_deadline_set(k_tid_t tid, int deadline)
{
	struct k_thread *thread = tid;

	/* The prio_deadline field changes the sorting order, so it must
	 * not be altered while the thread is in the run queue.  Because
	 * an rbtree will blow up if sorting invariants are violated mid-
	 * update, we delegate the dequeue/update/requeue sequence to
	 * z_sched_prio_deadline_set() which performs it under the
	 * scheduler spinlock.
	 */
	z_sched_prio_deadline_set(thread, deadline);
}

void z_impl_k_thread_deadline_set(k_tid_t tid, int deadline)
{
	deadline = clamp(deadline, 0, INT_MAX);

	int32_t newdl = k_cycle_get_32() + deadline;

	z_impl_k_thread_absolute_deadline_set(tid, newdl);
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_thread_absolute_deadline_set(k_tid_t tid, int deadline)
{
	struct k_thread *thread = tid;

	K_OOPS(K_SYSCALL_OBJ(thread, K_OBJ_THREAD));

	z_impl_k_thread_absolute_deadline_set((k_tid_t)thread, deadline);
}
#include <zephyr/syscalls/k_thread_absolute_deadline_set_mrsh.c>

static inline void z_vrfy_k_thread_deadline_set(k_tid_t tid, int deadline)
{
	struct k_thread *thread = tid;

	K_OOPS(K_SYSCALL_OBJ(thread, K_OBJ_THREAD));
	K_OOPS(K_SYSCALL_VERIFY_MSG(deadline > 0,
				    "invalid thread deadline %d",
				    (int)deadline));

	z_impl_k_thread_deadline_set((k_tid_t)thread, deadline);
}
#include <zephyr/syscalls/k_thread_deadline_set_mrsh.c>
#endif /* CONFIG_USERSPACE */
