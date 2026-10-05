/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_KERNEL_TIMEOUT_LIST_H_
#define ZEPHYR_KERNEL_TIMEOUT_LIST_H_

/**
 * @file
 * @brief Sorted delta-list timeout backend (implementation).
 *
 * This is the original (and default) timeout queue: a doubly linked list
 * sorted by expiry, where each node stores its expiry as a delta (dticks)
 * relative to its predecessor. Insertion is O(n), head removal is O(1).
 *
 * Included only by kernel/timeout.c, after its shared state, so the queue
 * instance and the z_timeout_q_*() operations below are private to that
 * translation unit. The per-node helpers live in kernel/include/timeout_q.h.
 */

#include <zephyr/sys/dlist.h>

static sys_dlist_t timeout_list = SYS_DLIST_STATIC_INIT(&timeout_list);

static inline struct _timeout *z_timeout_q_first(void)
{
	sys_dnode_t *t = sys_dlist_peek_head(&timeout_list);

	return (t == NULL) ? NULL : CONTAINER_OF(t, struct _timeout, node);
}

static inline struct _timeout *z_timeout_q_next(struct _timeout *t)
{
	sys_dnode_t *n = sys_dlist_peek_next(&timeout_list, &t->node);

	return (n == NULL) ? NULL : CONTAINER_OF(n, struct _timeout, node);
}

/* Insert @to so that it expires @dticks ticks after curr_tick. Returns true
 * if @to is now the earliest pending timeout.
 */
static inline bool z_timeout_q_insert(struct _timeout *to, k_ticks_t dticks)
{
	struct _timeout *t;

	to->dticks = dticks;
	for (t = z_timeout_q_first(); t != NULL; t = z_timeout_q_next(t)) {
		if (t->dticks > to->dticks) {
			t->dticks -= to->dticks;
			sys_dlist_insert(&t->node, &to->node);
			return z_timeout_q_first() == to;
		}
		to->dticks -= t->dticks;
	}

	sys_dlist_append(&timeout_list, &to->node);
	return z_timeout_q_first() == to;
}

/* Remove an arbitrary timeout from the queue, absorbing its delta into the
 * successor so the remaining deltas stay valid. Returns true if @to was the
 * earliest pending timeout. Used only by the abort path, never from announce.
 */
static inline bool z_timeout_q_remove(struct _timeout *to)
{
	bool was_first = (z_timeout_q_first() == to);
	struct _timeout *n = z_timeout_q_next(to);

	if (n != NULL) {
		n->dticks += to->dticks;
	}
	sys_dlist_remove(&to->node);

	return was_first;
}

/* Ticks from curr_tick until @to expires. Caller holds timeout_lock and has
 * verified that @to is active.
 */
static inline k_ticks_t z_timeout_q_remainder(const struct _timeout *to)
{
	k_ticks_t ticks = 0;

	for (struct _timeout *t = z_timeout_q_first(); t != NULL;
	     t = z_timeout_q_next(t)) {
		ticks += t->dticks;
		if (to == t) {
			break;
		}
	}

	return ticks;
}

/* Ticks from (curr_tick + ticks_elapsed) until the next expiry, clamped to
 * int32 / SYS_CLOCK_MAX_WAIT. Empty queue yields SYS_CLOCK_MAX_WAIT.
 */
/* Ticks from curr_tick to the earliest pending timeout, or K_TICKS_FOREVER if
 * the queue is empty. The announce-range cap is applied centrally by
 * next_timeout() in timeout.c.
 */
static inline k_ticks_t z_timeout_q_next_expiry(void)
{
	struct _timeout *to = z_timeout_q_first();

	return (to == NULL) ? K_TICKS_FOREVER : to->dticks;
}

/* --- announce-loop helpers (see sys_clock_announce_locked() in timeout.c) --- */

/* Ticks from curr_tick until the next event the announce loop must act on
 * (here: the earliest expiry). A value larger than any announce window when
 * the queue is empty.
 */
static inline int32_t z_timeout_q_next_gap(void)
{
	struct _timeout *t = z_timeout_q_first();

	return (t == NULL) ? INT32_MAX : (int32_t)MIN((int64_t)t->dticks, INT32_MAX);
}

/* Advance backend state by @dt ticks. For the delta list this shifts the head
 * closer to expiry; a head whose gap is fully consumed reads dticks == 0 and
 * is then drained by z_timeout_q_pop_due(). The caller advances curr_tick.
 */
static inline void z_timeout_q_advance(int32_t dt)
{
	struct _timeout *t = z_timeout_q_first();

	if (t != NULL) {
		t->dticks -= dt;
	}
}

/* Pop and return one timeout that is due at the current curr_tick, or NULL if
 * none is due right now. Strict head-only removal with no delta absorption:
 * z_timeout_q_advance() has already brought the head's dticks to 0, and
 * curr_tick was advanced to match, so the successor's stored delta stays
 * valid without propagation.
 */
static inline struct _timeout *z_timeout_q_pop_due(void)
{
	struct _timeout *t = z_timeout_q_first();

	if ((t == NULL) || (t->dticks != 0)) {
		return NULL;
	}

	sys_dlist_remove(&t->node);

	return t;
}

#endif /* ZEPHYR_KERNEL_TIMEOUT_LIST_H_ */
