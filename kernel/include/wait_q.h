/* wait queue for multiple threads on kernel objects */

/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_KERNEL_INCLUDE_WAIT_Q_H_
#define ZEPHYR_KERNEL_INCLUDE_WAIT_Q_H_

#include <kernel_structs.h>
#include <misc/dlist.h>
#include <misc/rb.h>
#include <ksched.h>
#include <sched_priq.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_SYS_CLOCK_EXISTS
#include <timeout_q.h>
#else
static ALWAYS_INLINE void _init_thread_timeout(struct _thread_base *thread_base)
{
	ARG_UNUSED(thread_base);
}

static ALWAYS_INLINE void
_add_thread_timeout(struct k_thread *thread, _wait_q_t *wait_q, s32_t timeout)
{
	ARG_UNUSED(thread);
	ARG_UNUSED(wait_q);
	ARG_UNUSED(timeout);
}

static ALWAYS_INLINE int _abort_thread_timeout(struct k_thread *thread)
{
	ARG_UNUSED(thread);

	return 0;
}
#define _get_next_timeout_expiry() (K_FOREVER)
#endif

#ifdef CONFIG_WAITQ_SCALABLE

#define _WAIT_Q_FOR_EACH(wq, thread_ptr) \
	RB_FOR_EACH_CONTAINER(&(wq)->waitq.tree, thread_ptr, base.qnode_rb)

static inline void _waitq_init(_wait_q_t *w)
{
	w->waitq = (struct _priq_rb) {
		.tree = {
			.lessthan_fn = _priq_rb_lessthan
		}
	};
}

static inline struct k_thread *_waitq_head(_wait_q_t *w)
{
	return (void *)rb_get_min(&w->waitq.tree);
}

#else /* !CONFIG_WAITQ_SCALABLE: */

#define _WAIT_Q_FOR_EACH(wq, thread_ptr) \
	SYS_DLIST_FOR_EACH_CONTAINER(&((wq)->waitq), thread_ptr, \
				     base.qnode_dlist)

static inline void _waitq_init(_wait_q_t *w)
{
	sys_dlist_init(&w->waitq);
}

static inline struct k_thread *_waitq_head(_wait_q_t *w)
{
	return (void *)sys_dlist_peek_head(&w->waitq);
}

#endif /* !CONFIG_WAITQ_SCALABLE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_KERNEL_INCLUDE_WAIT_Q_H_ */
