/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_KERNEL_INCLUDE_PRIORITY_Q_H_
#define ZEPHYR_KERNEL_INCLUDE_PRIORITY_Q_H_

#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/dlist.h>

/* Dumb Scheduling */
#if defined(CONFIG_SCHED_SIMPLE)
#define _priq_run_init		z_priq_simple_init
#define _priq_run_add		z_priq_simple_add
#define _priq_run_remove	z_priq_simple_remove
#define _priq_run_yield         z_priq_simple_yield
# if defined(CONFIG_SCHED_CPU_MASK)
#  define _priq_run_best	z_priq_simple_mask_best
# else
#  define _priq_run_best	z_priq_simple_best
# endif /* CONFIG_SCHED_CPU_MASK */
/* Scalable Scheduling */
#elif defined(CONFIG_SCHED_SCALABLE)
#define _priq_run_init		z_priq_rb_init
#define _priq_run_add		z_priq_rb_add
#define _priq_run_remove	z_priq_rb_remove
#define _priq_run_yield         z_priq_rb_yield
#define _priq_run_best		z_priq_rb_best
 /* Multi Queue Scheduling */
#elif defined(CONFIG_SCHED_MULTIQ)
#define _priq_run_init		z_priq_mq_init
#define _priq_run_add		z_priq_mq_add
#define _priq_run_remove	z_priq_mq_remove
#define _priq_run_yield         z_priq_mq_yield
#define _priq_run_best		z_priq_mq_best
#endif

/* Scalable Wait Queue */
#if defined(CONFIG_WAITQ_SCALABLE)
#define _priq_wait_add		z_priq_rb_add
#define _priq_wait_remove	z_priq_rb_remove
#define _priq_wait_best		z_priq_rb_best
/* Dumb Wait Queue */
#elif defined(CONFIG_WAITQ_SIMPLE)
#define _priq_wait_add		z_priq_simple_add
#define _priq_wait_remove	z_priq_simple_remove
#define _priq_wait_best		z_priq_simple_best
#endif

#if defined(CONFIG_64BIT)
#define NBITS          64
#define TRAILING_ZEROS u64_count_trailing_zeros
#else
#define NBITS          32
#define TRAILING_ZEROS u32_count_trailing_zeros
#endif /* CONFIG_64BIT */

static ALWAYS_INLINE void z_priq_simple_init(sys_dlist_t *pq)
{
	sys_dlist_init(pq);
}

/*
 * Return value same as e.g. memcmp
 * > 0 -> thread 1 priority  > thread 2 priority
 * = 0 -> thread 1 priority == thread 2 priority
 * < 0 -> thread 1 priority  < thread 2 priority
 * Do not rely on the actual value returned aside from the above.
 * (Again, like memcmp.)
 */
static ALWAYS_INLINE int32_t z_sched_prio_cmp(struct k_thread *thread_1, struct k_thread *thread_2)
{
	/* `prio` is <32b, so the below cannot overflow. */
	int32_t b1 = thread_1->base.prio;
	int32_t b2 = thread_2->base.prio;

	if (b1 != b2) {
		return b2 - b1;
	}

#ifdef CONFIG_SCHED_DEADLINE
	/* If we assume all deadlines live within the same "half" of
	 * the 32 bit modulus space (this is a documented API rule),
	 * then the latest deadline in the queue minus the earliest is
	 * guaranteed to be (2's complement) non-negative.  We can
	 * leverage that to compare the values without having to check
	 * the current time.
	 */
	uint32_t d1 = thread_1->base.prio_deadline;
	uint32_t d2 = thread_2->base.prio_deadline;

	if (d1 != d2) {
		/* Sooner deadline means higher effective priority.
		 * Doing the calculation with unsigned types and casting
		 * to signed isn't perfect, but at least reduces this
		 * from UB on overflow to impdef.
		 */
		return (int32_t)(d2 - d1);
	}
#endif /* CONFIG_SCHED_DEADLINE */
	return 0;
}

static ALWAYS_INLINE void z_priq_simple_add(sys_dlist_t *pq, struct k_thread *thread)
{
	struct k_thread *t;

	SYS_DLIST_FOR_EACH_CONTAINER(pq, t, base.qnode_dlist) {
		if (z_sched_prio_cmp(thread, t) > 0) {
			sys_dlist_insert(&t->base.qnode_dlist, &thread->base.qnode_dlist);
			return;
		}
	}

	sys_dlist_append(pq, &thread->base.qnode_dlist);
}

static ALWAYS_INLINE void z_priq_simple_remove(sys_dlist_t *pq, struct k_thread *thread)
{
	ARG_UNUSED(pq);

	sys_dlist_remove(&thread->base.qnode_dlist);
}

static ALWAYS_INLINE void z_priq_simple_yield(sys_dlist_t *pq)
{
#ifndef CONFIG_SMP
	sys_dnode_t *n;

	n = sys_dlist_peek_next_no_check(pq, &_current->base.qnode_dlist);

	sys_dlist_dequeue(&_current->base.qnode_dlist);

	struct k_thread *t;

	/*
	 * As it is possible that the current thread was not at the head of
	 * the run queue, start searching from the present position for where
	 * to re-insert it.
	 */

	while (n != NULL) {
		t = CONTAINER_OF(n, struct k_thread, base.qnode_dlist);
		if (z_sched_prio_cmp(_current, t) > 0) {
			sys_dlist_insert(&t->base.qnode_dlist,
					 &_current->base.qnode_dlist);
			return;
		}
		n = sys_dlist_peek_next_no_check(pq, n);
	}

	sys_dlist_append(pq, &_current->base.qnode_dlist);
#endif
}

static ALWAYS_INLINE struct k_thread *z_priq_simple_best(sys_dlist_t *pq)
{
	struct k_thread *thread = NULL;
	sys_dnode_t *n = sys_dlist_peek_head(pq);

	if (n != NULL) {
		thread = CONTAINER_OF(n, struct k_thread, base.qnode_dlist);
	}
	return thread;
}

#ifdef CONFIG_SCHED_CPU_MASK
static ALWAYS_INLINE struct k_thread *z_priq_simple_mask_best(sys_dlist_t *pq)
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

#if defined(CONFIG_SCHED_SCALABLE) || defined(CONFIG_WAITQ_SCALABLE)
static ALWAYS_INLINE void z_priq_rb_init(struct _priq_rb *pq)
{
	bool z_priq_rb_lessthan(struct rbnode *a, struct rbnode *b);

	*pq = (struct _priq_rb) {
		.tree = {
			.lessthan_fn = z_priq_rb_lessthan,
		}
	};
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

static ALWAYS_INLINE void z_priq_rb_yield(struct _priq_rb *pq)
{
#ifndef CONFIG_SMP
	z_priq_rb_remove(pq, _current);
	z_priq_rb_add(pq, _current);
#endif
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
#endif

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

static ALWAYS_INLINE unsigned int z_priq_mq_best_queue_index(struct _priq_mq *pq)
{
	unsigned int i = 0;

	do {
		if (likely(pq->bitmask[i])) {
			return i * NBITS + TRAILING_ZEROS(pq->bitmask[i]);
		}
		i++;
	} while (i < PRIQ_BITMAP_SIZE);

	return K_NUM_THREAD_PRIO - 1;
}

static ALWAYS_INLINE void z_priq_mq_init(struct _priq_mq *q)
{
	for (int i = 0; i < ARRAY_SIZE(q->queues); i++) {
		sys_dlist_init(&q->queues[i]);
	}

#ifndef CONFIG_SMP
	q->cached_queue_index = K_NUM_THREAD_PRIO - 1;
#endif
}

static ALWAYS_INLINE void z_priq_mq_add(struct _priq_mq *pq,
					struct k_thread *thread)
{
	struct prio_info pos = get_prio_info(thread->base.prio);

	sys_dlist_append(&pq->queues[pos.offset_prio], &thread->base.qnode_dlist);
	pq->bitmask[pos.idx] |= BIT(pos.bit);

#ifndef CONFIG_SMP
	if (pos.offset_prio < pq->cached_queue_index) {
		pq->cached_queue_index = pos.offset_prio;
	}
#endif
}

static ALWAYS_INLINE void z_priq_mq_remove(struct _priq_mq *pq,
					   struct k_thread *thread)
{
	struct prio_info pos = get_prio_info(thread->base.prio);

	sys_dlist_dequeue(&thread->base.qnode_dlist);
	if (unlikely(sys_dlist_is_empty(&pq->queues[pos.offset_prio]))) {
		pq->bitmask[pos.idx] &= ~BIT(pos.bit);
#ifndef CONFIG_SMP
		pq->cached_queue_index = z_priq_mq_best_queue_index(pq);
#endif
	}
}

static ALWAYS_INLINE void z_priq_mq_yield(struct _priq_mq *pq)
{
#ifndef CONFIG_SMP
	struct prio_info pos = get_prio_info(_current->base.prio);

	sys_dlist_dequeue(&_current->base.qnode_dlist);
	sys_dlist_append(&pq->queues[pos.offset_prio],
			 &_current->base.qnode_dlist);
#endif
}

static ALWAYS_INLINE struct k_thread *z_priq_mq_best(struct _priq_mq *pq)
{
#ifdef CONFIG_SMP
	unsigned int index = z_priq_mq_best_queue_index(pq);
#else
	unsigned int index = pq->cached_queue_index;
#endif

	sys_dnode_t *n = sys_dlist_peek_head(&pq->queues[index]);

	if (likely(n != NULL)) {
		return CONTAINER_OF(n, struct k_thread, base.qnode_dlist);
	}

	return NULL;
}

#endif /* ZEPHYR_KERNEL_INCLUDE_PRIORITY_Q_H_ */
