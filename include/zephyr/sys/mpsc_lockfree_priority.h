#ifndef ZEPHYR_INCLUDE_SYS_MPSC_LOCKFREE_PRIORITY_H_
#define ZEPHYR_INCLUDE_SYS_MPSC_LOCKFREE_PRIORITY_H_

/**
 * @file
 * @brief Priority-aware lock-free multiple-producer, single-consumer queue.
 */

#include <zephyr/sys/mpsc_lockfree.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Priority MPSC queue
 *
 * Each priority is backed by an independent MPSC queue. Priority 0 is the
 * highest priority. Items with the same priority are consumed in FIFO order.
 */
struct mpsc_priority {
	struct mpsc *queues;
	uint8_t num_priorities;
};

/**
 * @brief Initialize a priority MPSC queue
 *
 * @param q Priority queue to initialize
 * @param queues Array containing one MPSC queue per priority
 * @param num_priorities Number of elements in @p queues
 */
static inline void mpsc_priority_init(struct mpsc_priority *q, struct mpsc *queues,
				      uint8_t num_priorities)
{
	__ASSERT_NO_MSG(q != NULL);
	__ASSERT_NO_MSG(queues != NULL);
	__ASSERT_NO_MSG(num_priorities > 0U);

	q->queues = queues;
	q->num_priorities = num_priorities;

	for (uint8_t priority = 0U; priority < num_priorities; priority++) {
		mpsc_init(&queues[priority]);
	}
}

/**
 * @brief Push a node into a priority MPSC queue
 *
 * @param q Priority queue
 * @param node Node to push
 * @param priority Node priority, where 0 is the highest priority
 */
static ALWAYS_INLINE void mpsc_priority_push(struct mpsc_priority *q, struct mpsc_node *node,
					     uint8_t priority)
{
	__ASSERT_NO_MSG(q != NULL);
	__ASSERT_NO_MSG(node != NULL);
	__ASSERT_NO_MSG(priority < q->num_priorities);

	mpsc_push(&q->queues[priority], node);
}

/**
 * @brief Pop the highest-priority available node
 *
 * This function must only be called from one execution context. If producers
 * are active concurrently, the returned node is the highest-priority node
 * observable while the queues are scanned.
 *
 * @param q Priority queue
 *
 * @retval NULL When no node is available
 * @retval node Highest-priority available node
 */
static inline struct mpsc_node *mpsc_priority_pop(struct mpsc_priority *q)
{
	__ASSERT_NO_MSG(q != NULL);

	for (uint8_t priority = 0U; priority < q->num_priorities; priority++) {
		struct mpsc_node *node = mpsc_pop(&q->queues[priority]);

		if (node != NULL) {
			return node;
		}
	}

	return NULL;
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_MPSC_LOCKFREE_PRIORITY_H_ */
