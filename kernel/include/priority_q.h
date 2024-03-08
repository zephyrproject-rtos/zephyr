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
# endif /* CONFIG_SCHED_CPU_MASK */
/* Scalable Scheduling */
#elif defined(CONFIG_SCHED_SCALABLE)
#define _priq_run_add		z_priq_rb_add
#define _priq_run_remove	z_priq_rb_remove
#define _priq_run_best		z_priq_rb_best
 /* Multi Queue Scheduling */
#elif defined(CONFIG_SCHED_MULTIQ)

# if defined(CONFIG_64BIT)
#  define NBITS 64
# else
#  define NBITS 32
# endif

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

struct prio_info {
	uint8_t offset_prio;
	uint8_t idx;
	uint8_t bit;
};

static ALWAYS_INLINE struct prio_info get_prio_info(int8_t old_prio)
{
	struct prio_info ret;

	ret.offset_prio = old_prio - K_HIGHEST_THREAD_PRIO;
	ret.idx = ret.offset_prio / NBITS;
	ret.bit = ret.offset_prio % NBITS;

	return ret;
}

static ALWAYS_INLINE void z_priq_mq_add(struct _priq_mq *pq,
					struct k_thread *thread)
{
	struct prio_info pos = get_prio_info(thread->base.prio);

	sys_dlist_append(&pq->queues[pos.offset_prio], &thread->base.qnode_dlist);
	pq->bitmask[pos.idx] |= BIT(pos.bit);
}

static ALWAYS_INLINE void z_priq_mq_remove(struct _priq_mq *pq,
					   struct k_thread *thread)
{
	struct prio_info pos = get_prio_info(thread->base.prio);

	sys_dlist_remove(&thread->base.qnode_dlist);
	if (sys_dlist_is_empty(&pq->queues[pos.offset_prio])) {
		pq->bitmask[pos.idx] &= ~BIT(pos.bit);
	}
}
#endif /* CONFIG_SCHED_MULTIQ */
#endif /* ZEPHYR_KERNEL_INCLUDE_PRIORITY_Q_H_ */
