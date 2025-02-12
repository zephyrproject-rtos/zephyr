/*
 * Copyright (c) 2010-2011 Dmitry Vyukov
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SYS_MPSC_LOCKFREE_H_
#define ZEPHYR_SYS_MPSC_LOCKFREE_H_

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Multiple Producer Single Consumer (MPSC) Lockfree Queue API
 * @defgroup mpsc_lockfree MPSC Lockfree Queue API
 * @ingroup datastructure_apis
 * @{
 */

/**
 * @file mpsc_lockfree.h
 *
 * @brief A wait-free intrusive multi producer single consumer (MPSC) queue using
 * a singly linked list. Ordering is First-In-First-Out.
 *
 * Based on the well known and widely used wait-free MPSC queue described by
 * Dmitry Vyukov with some slight changes to account for needs of an
 * RTOS on a variety of archs. Both consumer and producer are wait free. No CAS
 * loop or lock is needed.
 *
 * An MPSC queue is safe to produce or consume in an ISR with O(1) push/pop.
 *
 * @warning MPSC is *not* safe to consume in multiple execution contexts.
 */

/*
 * On single core systems atomics are unnecessary
 * and cause a lot of unnecessary cache invalidation
 *
 * Using volatile to at least ensure memory is read/written
 * by the compiler generated op codes is enough.
 *
 * On SMP atomics *must* be used to ensure the pointers
 * are updated in the correct order.
 */
#if defined(CONFIG_SMP)

typedef atomic_ptr_t mpsc_ptr_t;

#define mpsc_ptr_get(ptr)          atomic_ptr_get(&(ptr))
#define mpsc_ptr_set(ptr, val)     atomic_ptr_set(&(ptr), val)
#define mpsc_ptr_set_get(ptr, val) atomic_ptr_set(&(ptr), val)

#else /* defined(CONFIG_SMP) */

typedef struct mpsc_node *mpsc_ptr_t;

#define mpsc_ptr_get(ptr)      ptr
#define mpsc_ptr_set(ptr, val) ptr = val
#define mpsc_ptr_set_get(ptr, val)                                                                 \
	({                                                                                         \
		mpsc_ptr_t tmp = ptr;                                                              \
		ptr = val;                                                                         \
		tmp;                                                                               \
	})

#endif /* defined(CONFIG_SMP) */

/**
 * @brief Queue member
 */
struct mpsc_node {
	mpsc_ptr_t next;
};

/**
 * @brief MPSC Queue
 */
struct mpsc {
	mpsc_ptr_t head;
	struct mpsc_node *tail;
	struct mpsc_node stub;
};

/**
 * @brief Static initializer for a mpsc queue
 *
 * Since the queue is
 *
 * @param symbol name of the queue
 */
#define MPSC_INIT(symbol)                                                                          \
	{                                                                                          \
		.head = (struct mpsc_node *)&symbol.stub,                                          \
		.tail = (struct mpsc_node *)&symbol.stub,                                          \
		.stub = {                                                                          \
			.next = NULL,                                                              \
		},                                                                                 \
	}

/**
 * @brief Initialize queue
 *
 * @param q Queue to initialize or reset
 */
static inline void mpsc_init(struct mpsc *q)
{
	mpsc_ptr_set(q->head, &q->stub);
	q->tail = &q->stub;
	mpsc_ptr_set(q->stub.next, NULL);
}

/**
 * @brief Push a node
 *
 * @param q Queue to push the node to
 * @param n Node to push into the queue
 */
static ALWAYS_INLINE void mpsc_push(struct mpsc *q, struct mpsc_node *n)
{
	struct mpsc_node *prev;
	int key;

	mpsc_ptr_set(n->next, NULL);

	key = arch_irq_lock();
	prev = (struct mpsc_node *)mpsc_ptr_set_get(q->head, n);
	mpsc_ptr_set(prev->next, n);
	arch_irq_unlock(key);
}

/**
 * @brief Pop a node off of the list
 *
 * @retval NULL When no node is available
 * @retval node When node is available
 */
static inline struct mpsc_node *mpsc_pop(struct mpsc *q)
{
	struct mpsc_node *head;
	struct mpsc_node *tail = q->tail;
	struct mpsc_node *next = (struct mpsc_node *)mpsc_ptr_get(tail->next);

	/* Skip over the stub/sentinel */
	if (tail == &q->stub) {
		if (next == NULL) {
			return NULL;
		}

		q->tail = next;
		tail = next;
		next = (struct mpsc_node *)mpsc_ptr_get(next->next);
	}

	/* If next is non-NULL then a valid node is found, return it */
	if (next != NULL) {
		q->tail = next;
		return tail;
	}

	head = (struct mpsc_node *)mpsc_ptr_get(q->head);

	/* If next is NULL, and the tail != HEAD then the queue has pending
	 * updates that can't yet be accessed.
	 */
	if (tail != head) {
		return NULL;
	}

	mpsc_push(q, &q->stub);

	next = (struct mpsc_node *)mpsc_ptr_get(tail->next);

	if (next != NULL) {
		q->tail = next;
		return tail;
	}

	return NULL;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SYS_MPSC_LOCKFREE_H_ */
