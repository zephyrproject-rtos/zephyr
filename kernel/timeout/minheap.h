/*
 * Copyright (c) 2026 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_KERNEL_TIMEOUT_MINHEAP_H_
#define ZEPHYR_KERNEL_TIMEOUT_MINHEAP_H_

/**
 * @file
 * @brief Binary min-heap timeout backend (implementation).
 *
 * Pending timeouts are kept in a binary min-heap keyed on absolute expiry
 * tick (struct _timeout's abs_ticks). Insertion and arbitrary removal are
 * O(log n); the earliest timeout is always at the root. A timeout that is
 * not queued has heap_handle.idx == 0.
 *
 * Included only by kernel/timeout.c, after its shared state (curr_tick), so
 * the heap instance and the z_timeout_q_*() operations below are private to
 * that translation unit. The per-node helpers live in
 * kernel/include/timeout_q.h.
 */

#include <zephyr/sys/min_heap_ref.h>

static int timeout_cmp(const struct min_heap_handle *a, const struct min_heap_handle *b)
{
	const struct _timeout *ta = CONTAINER_OF(a, struct _timeout, heap_handle);
	const struct _timeout *tb = CONTAINER_OF(b, struct _timeout, heap_handle);

	if (ta->abs_ticks < tb->abs_ticks) {
		return -1;
	}

	return (ta->abs_ticks > tb->abs_ticks) ? 1 : 0;
}

MIN_HEAP_REF_DEFINE_STATIC(timeout_heap_inst, CONFIG_TIMEOUT_HEAP_MAX_ENTRIES, timeout_cmp);

static inline struct _timeout *z_timeout_q_root(void)
{
	const struct min_heap_handle *h = min_heap_ref_peek(&timeout_heap_inst);

	return (h == NULL) ? NULL : CONTAINER_OF(h, struct _timeout, heap_handle);
}

/* Insert @to so that it expires @dticks ticks after curr_tick. Returns true
 * if @to is now the earliest pending timeout (heap root).
 */
static inline bool z_timeout_q_insert(struct _timeout *to, k_ticks_t dticks)
{
	to->abs_ticks = (int64_t)curr_tick + dticks;

	if (unlikely(min_heap_ref_push(&timeout_heap_inst, &to->heap_handle) != 0)) {
		/*
		 * Heap overflow is always a configuration error: there is no
		 * graceful way to drop a kernel timeout. Increase
		 * CONFIG_TIMEOUT_HEAP_MAX_ENTRIES.
		 */
		k_panic();
	}

	return to->heap_handle.idx == 1U;
}

/* Remove an arbitrary timeout from the heap. Returns true if it was the
 * earliest pending timeout. Used only by the abort path, never from announce.
 */
static inline bool z_timeout_q_remove(struct _timeout *to)
{
	bool was_first = (to->heap_handle.idx == 1U);

	(void)min_heap_ref_remove(&timeout_heap_inst, &to->heap_handle);

	return was_first;
}

/* Ticks from curr_tick until @to expires. Caller holds timeout_lock and has
 * verified that @to is active.
 */
static inline k_ticks_t z_timeout_q_remainder(const struct _timeout *to)
{
	return (k_ticks_t)(to->abs_ticks - (int64_t)curr_tick);
}

/* Ticks from curr_tick to the earliest pending timeout, or K_TICKS_FOREVER if
 * the heap is empty. The announce-range cap is applied centrally by
 * next_timeout() in timeout.c.
 */
static inline k_ticks_t z_timeout_q_next_expiry(void)
{
	struct _timeout *t = z_timeout_q_root();

	return (t == NULL) ? K_TICKS_FOREVER
			   : (k_ticks_t)(t->abs_ticks - (int64_t)curr_tick);
}

/* --- announce-loop helpers (see sys_clock_announce_locked() in timeout.c) --- */

/* Ticks from curr_tick until the earliest expiry. A value larger than any
 * announce window when the heap is empty.
 */
static inline int32_t z_timeout_q_next_gap(void)
{
	struct _timeout *t = z_timeout_q_root();

	if (t == NULL) {
		return INT32_MAX;
	}

	return (int32_t)MIN(t->abs_ticks - (int64_t)curr_tick, (int64_t)INT32_MAX);
}

/* Advance backend state by @dt ticks. The heap stores absolute expiry ticks,
 * so the caller's curr_tick += dt is all the advancing required here.
 */
static inline void z_timeout_q_advance(int32_t dt)
{
	ARG_UNUSED(dt);
}

/* Pop and return one timeout that is due at the current curr_tick, or NULL if
 * none is due right now. Tied timeouts share the same abs_ticks and are
 * drained by successive calls.
 */
static inline struct _timeout *z_timeout_q_pop_due(void)
{
	struct _timeout *t = z_timeout_q_root();

	if ((t == NULL) || (t->abs_ticks > (int64_t)curr_tick)) {
		return NULL;
	}

	return CONTAINER_OF(min_heap_ref_pop(&timeout_heap_inst), struct _timeout, heap_handle);
}

#endif /* ZEPHYR_KERNEL_TIMEOUT_MINHEAP_H_ */
