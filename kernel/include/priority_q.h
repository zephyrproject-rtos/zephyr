/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_KERNEL_INCLUDE_PRIORITY_Q_H_
#define ZEPHYR_KERNEL_INCLUDE_PRIORITY_Q_H_


/* Dump Scheduling */
#if defined(CONFIG_SCHED_DUMB)
#define _priq_run_add		z_priq_dumb_add
#define _priq_run_remove	z_priq_dumb_remove
# if defined(CONFIG_SCHED_CPU_MASK)
#  define _priq_run_best	_priq_dumb_mask_best
# else
#  define _priq_run_best	z_priq_dumb_best
# endif
/* Scalable Scheduling */
#elif defined(CONFIG_SCHED_SCALABLE)
#define _priq_run_add		z_priq_rb_add
#define _priq_run_remove	z_priq_rb_remove
#define _priq_run_best		z_priq_rb_best
 /* Multi Queue Scheduling */
#elif defined(CONFIG_SCHED_MULTIQ)
#define _priq_run_add		z_priq_mq_add
#define _priq_run_remove	z_priq_mq_remove
#define _priq_run_best		z_priq_mq_best
static ALWAYS_INLINE void z_priq_mq_add(struct _priq_mq *pq, struct k_thread *thread);
static ALWAYS_INLINE void z_priq_mq_remove(struct _priq_mq *pq, struct k_thread *thread);
#endif

/* Scalable Wait Queue */
#if defined(CONFIG_WAITQ_SCALABLE)
#define z_priq_wait_add		z_priq_rb_add
#define _priq_wait_remove	z_priq_rb_remove
#define _priq_wait_best		z_priq_rb_best
/* Dump Wait Queue */
#elif defined(CONFIG_WAITQ_DUMB)
#define z_priq_wait_add		z_priq_dumb_add
#define _priq_wait_remove	z_priq_dumb_remove
#define _priq_wait_best		z_priq_dumb_best
#endif

/* Dumb Scheduling*/
struct k_thread *z_priq_dumb_best(sys_dlist_t *pq);
void z_priq_dumb_remove(sys_dlist_t *pq, struct k_thread *thread);

/* Scalable Scheduling */
void z_priq_rb_add(struct _priq_rb *pq, struct k_thread *thread);
void z_priq_rb_remove(struct _priq_rb *pq, struct k_thread *thread);

/* Multi Queue Scheduling */
struct k_thread *z_priq_mq_best(struct _priq_mq *pq);
struct k_thread *z_priq_rb_best(struct _priq_rb *pq);


bool z_priq_rb_lessthan(struct rbnode *a, struct rbnode *b);


#ifdef CONFIG_SCHED_MULTIQ
# if (K_LOWEST_THREAD_PRIO - K_HIGHEST_THREAD_PRIO) > 31
# error Too many priorities for multiqueue scheduler (max 32)
# endif

#if defined(CONFIG_SCHED_DEADLINE)

static ALWAYS_INLINE int z_deadline_cmp(struct k_thread *thread_1,
	struct k_thread *thread_2)
{
	uint32_t d1 = thread_1->base.prio_deadline;
	uint32_t d2 = thread_2->base.prio_deadline;

	return (int32_t) (d2 - d1);
}

#endif

static ALWAYS_INLINE void z_priq_mq_add(struct _priq_mq *pq,
					struct k_thread *thread)
{
	int priority_bit = thread->base.prio - K_HIGHEST_THREAD_PRIO;

#ifdef CONFIG_SCHED_DEADLINE
	struct k_thread *t;

	SYS_DLIST_FOR_EACH_CONTAINER(&pq->queues[priority_bit], t, base.qnode_dlist) {
		if (z_deadline_cmp(thread, t) > 0) {
			sys_dlist_insert(&t->base.qnode_dlist,
					 &thread->base.qnode_dlist);
			return;
		}
	}
#endif

	sys_dlist_append(&pq->queues[priority_bit], &thread->base.qnode_dlist);
	pq->bitmask |= BIT(priority_bit);
}

static ALWAYS_INLINE void z_priq_mq_remove(struct _priq_mq *pq,
					   struct k_thread *thread)
{
	int priority_bit = thread->base.prio - K_HIGHEST_THREAD_PRIO;

	sys_dlist_remove(&thread->base.qnode_dlist);
	if (sys_dlist_is_empty(&pq->queues[priority_bit])) {
		pq->bitmask &= ~BIT(priority_bit);
	}
}
#endif /* CONFIG_SCHED_MULTIQ */
#endif /* ZEPHYR_KERNEL_INCLUDE_PRIORITY_Q_H_ */
