/* wait queue for multiple threads on kernel objects */

/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_KERNEL_INCLUDE_WAIT_Q_H_
#define ZEPHYR_KERNEL_INCLUDE_WAIT_Q_H_

#include <zephyr/kernel_structs.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/sys/rb.h>
#include <timeout_q.h>
#include <priority_q.h>
#include <kspinlock.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_WAITQ_SCALABLE

#define _WAIT_Q_FOR_EACH(wq, thread_ptr) \
	RB_FOR_EACH_CONTAINER(&(wq)->waitq.tree, thread_ptr, base.qnode_rb)

static inline void z_waitq_init(_wait_q_t *w)
{
	w->waitq = (struct _priq_rb) {
		.tree = {
			.lessthan_fn = z_priq_rb_lessthan
		}
	};
}

static inline struct k_thread *z_waitq_head_locked(_wait_q_t *w)
{
	return (struct k_thread *)rb_get_min(&w->waitq.tree);
}

#else /* !CONFIG_WAITQ_SCALABLE: */

#define _WAIT_Q_FOR_EACH(wq, thread_ptr) \
	SYS_DLIST_FOR_EACH_CONTAINER(&((wq)->waitq), thread_ptr, \
				     base.qnode_dlist)

static inline void z_waitq_init(_wait_q_t *w)
{
	sys_dlist_init(&w->waitq);
}

static inline struct k_thread *z_waitq_head_locked(_wait_q_t *w)
{
	return (struct k_thread *)sys_dlist_peek_head(&w->waitq);
}

#endif /* !CONFIG_WAITQ_SCALABLE */

static inline struct k_thread *z_waitq_head(_wait_q_t *w)
{
	struct k_thread *thread = NULL;

	/*
	 * On a single core system, all callers of this function are already
	 * protected by a spinlock held by the caller. On those systems, there
	 * is no need to lock the scheduler's spinlock. However, on SMP systems
	 * the caller controlled spinlock is insufficient as the thread timeout
	 * handler may be running on another CPU. Use LOCK_SCHED_SPINLOCK
	 * to protect the scope as necessary.
	 */

	LOCK_SCHED_SPINLOCK {
		thread = z_waitq_head_locked(w);
	}

	return thread;
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_KERNEL_INCLUDE_WAIT_Q_H_ */
