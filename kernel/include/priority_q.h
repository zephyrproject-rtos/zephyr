/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_KERNEL_INCLUDE_PRIORITY_Q_H_
#define ZEPHYR_KERNEL_INCLUDE_PRIORITY_Q_H_

#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/dlist.h>

extern int32_t z_sched_prio_cmp(struct k_thread *thread_1,
	struct k_thread *thread_2);

bool z_priq_rb_lessthan(struct rbnode *a, struct rbnode *b);

/* Dumb Scheduling */
#if defined(CONFIG_SCHED_DUMB)
#define _priq_run_add		z_priq_dumb_add
#define _priq_run_remove	z_priq_dumb_remove
# if defined(CONFIG_SCHED_CPU_MASK)
#  define _priq_run_best	z_priq_dumb_mask_best
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

#if defined(CONFIG_64BIT)
#define NBITS 64
#else
#define NBITS 32
#endif /* CONFIG_64BIT */

#define _priq_run_add		z_priq_mq_add
#define _priq_run_remove	z_priq_mq_remove
#define _priq_run_best		z_priq_mq_best
static ALWAYS_INLINE void z_priq_mq_add(struct _priq_mq *pq, struct k_thread *thread);
static ALWAYS_INLINE void z_priq_mq_remove(struct _priq_mq *pq, struct k_thread *thread);
#endif

/* Scalable Wait Queue */
#if defined(CONFIG_WAITQ_SCALABLE)
#define _priq_wait_add		z_priq_rb_add
#define _priq_wait_remove	z_priq_rb_remove
#define _priq_wait_best		z_priq_rb_best
/* Dumb Wait Queue */
#elif defined(CONFIG_WAITQ_DUMB)
#define _priq_wait_add		z_priq_dumb_add
#define _priq_wait_remove	z_priq_dumb_remove
#define _priq_wait_best		z_priq_dumb_best
#endif

static ALWAYS_INLINE void z_priq_dumb_remove(sys_dlist_t *pq, struct k_thread *thread)
{
	ARG_UNUSED(pq);

	sys_dlist_remove(&thread->base.qnode_dlist);
}

static ALWAYS_INLINE struct k_thread *z_priq_dumb_best(sys_dlist_t *pq)
{
	struct k_thread *thread = NULL;
	sys_dnode_t *n = sys_dlist_peek_head(pq);

	if (n != NULL) {
		thread = CONTAINER_OF(n, struct k_thread, base.qnode_dlist);
	}
	return thread;
}

static ALWAYS_INLINE void z_priq_rb_add(struct _priq_rb *pq, struct k_thread *thread)
{
	struct k_thread *t;

	thread->base.order_key = pq->next_order_key;
	++pq->next_order_key;

	/* Renumber at wraparound.  This is tiny code, and in practice
	 * will almost never be hit on real systems.  BUT on very
	 * long-running systems where a priq never completely empties
	 * AND that contains very large numbers of threads, it can be
	 * a latency glitch to loop over all the threads like this.
	 */
	if (!pq->next_order_key) {
		RB_FOR_EACH_CONTAINER(&pq->tree, t, base.qnode_rb) {
			t->base.order_key = pq->next_order_key;
			++pq->next_order_key;
		}
	}

	rb_insert(&pq->tree, &thread->base.qnode_rb);
}

static ALWAYS_INLINE void z_priq_rb_remove(struct _priq_rb *pq, struct k_thread *thread)
{
	rb_remove(&pq->tree, &thread->base.qnode_rb);

	if (!pq->tree.root) {
		pq->next_order_key = 0;
	}
}

static ALWAYS_INLINE struct k_thread *z_priq_rb_best(struct _priq_rb *pq)
{
	struct k_thread *thread = NULL;
	struct rbnode *n = rb_get_min(&pq->tree);

	if (n != NULL) {
		thread = CONTAINER_OF(n, struct k_thread, base.qnode_rb);
	}
	return thread;
}

static ALWAYS_INLINE struct k_thread *z_priq_mq_best(struct _priq_mq *pq)
{
	struct k_thread *thread = NULL;

	for (int i = 0; i < PRIQ_BITMAP_SIZE; ++i) {
		if (!pq->bitmask[i]) {
			continue;
		}

#ifdef CONFIG_64BIT
		sys_dlist_t *l = &pq->queues[i * 64 + u64_count_trailing_zeros(pq->bitmask[i])];
#else
		sys_dlist_t *l = &pq->queues[i * 32 + u32_count_trailing_zeros(pq->bitmask[i])];
#endif
		sys_dnode_t *n = sys_dlist_peek_head(l);

		if (n != NULL) {
			thread = CONTAINER_OF(n, struct k_thread, base.qnode_dlist);
			break;
		}
	}

	return thread;
}


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



#ifdef CONFIG_SCHED_CPU_MASK
static ALWAYS_INLINE struct k_thread *z_priq_dumb_mask_best(sys_dlist_t *pq)
{
	/* With masks enabled we need to be prepared to walk the list
	 * looking for one we can run
	 */
	struct k_thread *thread;

	SYS_DLIST_FOR_EACH_CONTAINER(pq, thread, base.qnode_dlist) {
		if ((thread->base.cpu_mask & BIT(_current_cpu->id)) != 0) {
			return thread;
		}
	}
	return NULL;
}
#endif /* CONFIG_SCHED_CPU_MASK */


#if defined(CONFIG_SCHED_DUMB) || defined(CONFIG_WAITQ_DUMB)
static ALWAYS_INLINE void z_priq_dumb_add(sys_dlist_t *pq,
					  struct k_thread *thread)
{
	struct k_thread *t;

	SYS_DLIST_FOR_EACH_CONTAINER(pq, t, base.qnode_dlist) {
		if (z_sched_prio_cmp(thread, t) > 0) {
			sys_dlist_insert(&t->base.qnode_dlist,
					 &thread->base.qnode_dlist);
			return;
		}
	}

	sys_dlist_append(pq, &thread->base.qnode_dlist);
}
#endif /* CONFIG_SCHED_DUMB || CONFIG_WAITQ_DUMB */

#endif /* ZEPHYR_KERNEL_INCLUDE_PRIORITY_Q_H_ */
