/*
 * Copyright (c) 2026 Dhruv Menon <dhruvmenon1104@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_KERNEL_TIMEOUT_SKIPLIST_H_
#define ZEPHYR_KERNEL_TIMEOUT_SKIPLIST_H_

/**
 * @file
 * @brief Skip-list timeout backend (implementation).
 *
 * Pending timeouts are kept in a Pugh skip list keyed on absolute expiry
 * tick (struct _timeout's abs_ticks). Expected insertion and arbitrary
 * removal are O(log n); the earliest timeout is always the level-0 successor
 * of the sentinel. A timeout that is not queued has height == 0.
 *
 * Same-tick firing order is FIFO: a new node is inserted after every node
 * that already has the same abs_ticks.
 *
 * Node height is drawn from a geometric(1/2) distribution using a private
 * xorshift32, so the announce/add/abort paths never call the entropy driver.
 * There is no capacity limit (unlike the min-heap backend).
 */

#define SKIPLIST_LEVELS CONFIG_TIMEOUT_SKIPLIST_MAX_LEVEL

/* Sentinel: never expires, participates in every level, never fired. */
static struct _timeout sl_head = {
	.height = SKIPLIST_LEVELS,
	.forward = { NULL },
};

/* Highest height of any currently queued node; 0 when the list is empty. */
static uint8_t sl_top;

/*
 * Scratch predecessor vector shared by insert and remove. Both run with
 * timeout_lock held and neither keeps it across an unlock, so one array is
 * enough. Any future caller must hold that lock too.
 */
static struct _timeout *sl_update[SKIPLIST_LEVELS];

/* Marsaglia xorshift32 state; period 2^32-1, must stay non-zero. */
static uint32_t sl_rng = 2463534242U;

static uint8_t skiplist_random_height(void)
{
	uint32_t r = sl_rng;

	r ^= r << 13;
	r ^= r >> 17;
	r ^= r << 5;
	sl_rng = r;

	/* p = 1/2: height is 1 + the run of low 1-bits, capped at SKIPLIST_LEVELS. */
	return 1 + MIN(u32_count_trailing_zeros(~r), SKIPLIST_LEVELS - 1);
}

static inline struct _timeout *skiplist_first(void)
{
	return sl_head.forward[0];
}

static void skiplist_drop_top(void)
{
	while (sl_top > 0 && sl_head.forward[sl_top - 1] == NULL) {
		sl_top--;
	}
}

static void skiplist_update_init(struct _timeout **update)
{
	uint8_t i;

	for (i = 0; i < SKIPLIST_LEVELS; i++) {
		update[i] = &sl_head;
	}
}

/* Splice @to out at every level it occupies. */
static void skiplist_unlink(struct _timeout *to, struct _timeout **update)
{
	uint8_t i;
	uint8_t height = to->height;

	for (i = 0; i < height; i++) {
		__ASSERT_NO_MSG(update[i]->forward[i] == to);
		update[i]->forward[i] = to->forward[i];
	}

	to->height = 0;
	skiplist_drop_top();
}

/* Equal keys are skipped so a later insert lands after them (same-tick FIFO). */
static void skiplist_predecessors_after(int64_t abs_ticks, struct _timeout **update)
{
	struct _timeout *x = &sl_head;
	int i;

	for (i = (int)sl_top - 1; i >= 0; i--) {
		while (x->forward[i] != NULL && x->forward[i]->abs_ticks <= abs_ticks) {
			x = x->forward[i];
		}
		update[i] = x;
	}
}

/* Walk equal-key nodes so identity, not just expiry, selects the splice points. */
static void skiplist_predecessors_of(struct _timeout *to, struct _timeout **update)
{
	struct _timeout *x = &sl_head;
	int i;

	for (i = (int)sl_top - 1; i >= 0; i--) {
		while (x->forward[i] != NULL && x->forward[i]->abs_ticks < to->abs_ticks) {
			x = x->forward[i];
		}
		update[i] = x;
	}

	for (i = 0; i < to->height; i++) {
		while (update[i]->forward[i] != NULL &&
		       update[i]->forward[i] != to &&
		       update[i]->forward[i]->abs_ticks == to->abs_ticks) {
			update[i] = update[i]->forward[i];
		}
	}
}

static inline bool z_timeout_q_insert(struct _timeout *to, k_ticks_t dticks)
{
	uint8_t i;
	uint8_t height;

	to->abs_ticks = (int64_t)curr_tick + dticks;
	height = skiplist_random_height();
	to->height = height;

	skiplist_update_init(sl_update);
	skiplist_predecessors_after(to->abs_ticks, sl_update);

	if (height > sl_top) {
		sl_top = height;
	}

	for (i = 0; i < height; i++) {
		to->forward[i] = sl_update[i]->forward[i];
		sl_update[i]->forward[i] = to;
	}

	return skiplist_first() == to;
}

static inline bool z_timeout_q_remove(struct _timeout *to)
{
	bool was_first = (skiplist_first() == to);

	__ASSERT_NO_MSG(to->height > 0);

	skiplist_update_init(sl_update);
	skiplist_predecessors_of(to, sl_update);

	skiplist_unlink(to, sl_update);

	return was_first;
}

static inline k_ticks_t z_timeout_q_remainder(const struct _timeout *to)
{
	return (k_ticks_t)(to->abs_ticks - (int64_t)curr_tick);
}

static inline k_ticks_t z_timeout_q_next_expiry(void)
{
	struct _timeout *t = skiplist_first();
	int64_t gap;

	if (t == NULL) {
		return K_TICKS_FOREVER;
	}

	gap = t->abs_ticks - (int64_t)curr_tick;
	return (gap < 0) ? 0 : (k_ticks_t)gap;
}

static inline int32_t z_timeout_q_next_gap(void)
{
	struct _timeout *t = skiplist_first();
	int64_t gap;

	if (t == NULL) {
		return INT32_MAX;
	}

	/* Overdue must be 0: announce casts dt to uint32_t. */
	gap = t->abs_ticks - (int64_t)curr_tick;
	if (gap <= 0) {
		return 0;
	}

	return (int32_t)MIN(gap, (int64_t)INT32_MAX);
}

static inline void z_timeout_q_advance(int32_t dt)
{
	ARG_UNUSED(dt);
}

/* Earliest node, so it is the head's successor at every level it occupies. */
static inline struct _timeout *z_timeout_q_pop_due(void)
{
	struct _timeout *t = skiplist_first();
	uint8_t i;

	if ((t == NULL) || (t->abs_ticks > (int64_t)curr_tick)) {
		return NULL;
	}

	for (i = 0; i < t->height; i++) {
		__ASSERT_NO_MSG(sl_head.forward[i] == t);
		sl_head.forward[i] = t->forward[i];
	}

	t->height = 0;
	skiplist_drop_top();

	return t;
}

#endif /* ZEPHYR_KERNEL_TIMEOUT_SKIPLIST_H_ */
