/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/spinlock.h>
#include <ksched.h>
#include <timeout_q.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <zephyr/llext/symbol.h>

#include <timeslicing.h>

static uint64_t curr_tick;

static struct timeout_q timeout_list;

/*
 * The timeout code shall take no locks other than its own (timeout_lock), nor
 * shall it call any other subsystem while holding this lock.
 */
static struct k_spinlock timeout_lock;

/* Ticks left to process in the currently-executing sys_clock_announce() */
static int announce_remaining;

#if defined(CONFIG_TIMEOUT_MANAGEMENT_DELTAQ)
static inline int timeout_deltaq_init(void)
{
	sys_dlist_init(&timeout_list.deltaq);

	return 0;
}

/**
 * Get the first timeout in the delta-q timeout management structure
 */
static struct _timeout *timeout_deltaq_first(void)
{
	sys_dnode_t *t = sys_dlist_peek_head(&timeout_list.deltaq);

	return (t == NULL) ? NULL : CONTAINER_OF(t, struct _timeout, node);
}

/**
 * Given a pointer to a timeout structure, get the next timeout
 */
static struct _timeout *timeout_deltaq_next(struct _timeout *t)
{
	sys_dnode_t *n = sys_dlist_peek_next(&timeout_list.deltaq, &t->node);

	return (n == NULL) ? NULL : CONTAINER_OF(n, struct _timeout, node);
}

/**
 * Remove a timeout from the delta-q timeout management structure
 */
static void timeout_deltaq_remove_timeout(struct _timeout *t)
{
	if (timeout_deltaq_next(t) != NULL) {
		timeout_deltaq_next(t)->dticks += t->dticks;
	}

	sys_dlist_remove(&t->node);
}

/**
 * Get the remaining number of ticks until the specified timeout expires
 */
static k_ticks_t timeout_deltaq_remainder(const struct _timeout *to)
{
	k_ticks_t ticks = 0;

	for (struct _timeout *t = timeout_deltaq_first();
	     t != NULL;
	     t = timeout_deltaq_next(t)) {
		ticks += t->dticks;
		if (to == t) {
			break;
		}
	}

	return ticks;
}

/**
 * Add a timeout to the delta-q timeout management structure
 */
static void timeout_deltaq_add_timeout(struct _timeout *to)
{
	struct _timeout *t;

	for (t = timeout_deltaq_first();
	     t != NULL;
	     t = timeout_deltaq_next(t)) {
		if (t->dticks > to->dticks) {
			t->dticks -= to->dticks;
			sys_dlist_insert(&t->node, &to->node);
			break;
		}
		to->dticks -= t->dticks;
		}

	if (t == NULL) {
		sys_dlist_append(&timeout_list.deltaq, &to->node);
	}
}

/**
 * Loop to announce ticks and process expired timeouts
 */
static inline k_spinlock_key_t timeout_deltaq_announce(k_spinlock_key_t key)
{
	struct _timeout *t;

	for (t = timeout_deltaq_first();
	     (t != NULL) && (t->dticks <= announce_remaining);
	     t = timeout_deltaq_first()) {
		int dt = t->dticks;

		curr_tick += dt;
		t->dticks = 0;
		timeout_deltaq_remove_timeout(t);
		t->dticks = TIMEOUT_DTICKS_ANNOUNCING;

		k_spin_unlock(&timeout_lock, key);
		t->fn(t);
		key = k_spin_lock(&timeout_lock);
		announce_remaining -= dt;
	}

	if (t != NULL) {
		t->dticks -= announce_remaining;
	}

	curr_tick += announce_remaining;
	announce_remaining = 0;

	return key;
}

static int32_t timeout_deltaq_next_timeout(int32_t ticks_elapsed)
{
	struct _timeout *to = timeout_deltaq_first();
	int32_t ret;

	if ((to == NULL) ||
	    ((int64_t)(to->dticks - ticks_elapsed) > (int64_t)INT_MAX)) {
		ret = SYS_CLOCK_MAX_WAIT;
	} else {
		ret = max(0, to->dticks - ticks_elapsed);
	}

	return ret;
}
#elif defined(CONFIG_TIMEOUT_MANAGEMENT_WHEEL)
static int timeout_wheel_init(void)
{
	unsigned int i;

	/* Initialize all the lists in the timeout wheel */

	for (i = 0; i < NUM_SOON_LISTS; i++) {
		sys_dlist_init(&timeout_list.soon[i]);
	}

	for (i = 0; i < NUM_LATER_LISTS; i++) {
		sys_dlist_init(&timeout_list.later[i]);
	}

	sys_dlist_init(&timeout_list.distant);
	sys_dlist_init(&timeout_list.soon_defer);

	timeout_list.soon_bits = 0;
	timeout_list.soon_index = NUM_SOON_LISTS - 1;
	timeout_list.later_index = 0;
	timeout_list.announcing = 0;
	timeout_list.sifted = 1;

	return 0;
}

static inline void timeout_wheel_clear_flags(struct _timeout *to)
{
	to->flags &= ~(_TIMEOUT_FLAG_TIMER_WHEEL_SOON |
		       _TIMEOUT_FLAG_TIMER_WHEEL_LATER);
}

/**
 * Return the last item in the specified list, or NULL if the list is empty.
 */
static struct _timeout *timeout_wheel_last_in_list(sys_dlist_t *list)
{
	sys_dnode_t *t = sys_dlist_peek_tail(list);

	return (t == NULL) ? NULL : CONTAINER_OF(t, struct _timeout, node);
}

/**
 * Return the first timeout expiring in the specified list, or NULL
 */
static struct _timeout *timeout_wheel_first_in_list(sys_dlist_t *list)
{
	sys_dnode_t *t = sys_dlist_peek_head(list);

	return (t == NULL) ? NULL : CONTAINER_OF(t, struct _timeout, node);
}

/**
 * Get the first timeout in the wheel timeout management structure from its
 * 'soon' lists. NULL means that the first timeout is the sift operation which
 * always implicitly exists within the final 'soon' list.
 */
static struct _timeout *timeout_wheel_first(void)
{
	uint32_t bitmap;
	unsigned int index;
	sys_dnode_t *t;

	bitmap = timeout_list.soon_bits | BIT(NUM_SOON_LISTS - 1);
	if (timeout_list.sifted == 0) {
		bitmap &= ~(BIT(timeout_list.soon_index) - 1);
	}
	index = u32_count_trailing_zeros(bitmap) + timeout_list.sifted;

	t = sys_dlist_peek_head(&timeout_list.soon[index - timeout_list.sifted]);

	return (t == NULL) ? NULL : CONTAINER_OF(t, struct _timeout, node);
}

/**
 * Return the timeout following @a t expiring "distantly".
 */
static struct _timeout *timeout_wheel_next_distant(struct _timeout *t)
{
	sys_dnode_t *n = sys_dlist_peek_next(&timeout_list.distant, &t->node);

	return (n == NULL) ? NULL : CONTAINER_OF(n, struct _timeout, node);
}

/**
 * Prepend a timeout to the appropriate "soon" list. This is only called during
 * the sifting process after the timeout had been removed from a 'later' list.
 * Recall that while in the 'later' list, the timeout's 'dticks' field was a
 * composite value. We must update the 'dticks' field as it is added to the
 * appropriate 'soon' list. Prepending preserves the time ordering of the
 * timeouts.
 */
static void timeout_wheel_prepend_soon(struct _timeout *to)
{
	unsigned int index = to->dticks & (NUM_SOON_LISTS - 1);

	timeout_list.soon_bits |= BIT(index);
	sys_dlist_prepend(&timeout_list.soon[index], &to->node);
	to->flags |= _TIMEOUT_FLAG_TIMER_WHEEL_SOON;
	to->dticks = index;
}

/**
 * Append a timeout to the appropriate "soon" list.
 *
 * As part of the appending process, this routine modifies the timeout's
 * 'dticks' field to be the index of the "soon" list to which it is appended.
 */
static void timeout_wheel_append_soon(struct _timeout *to)
{
	unsigned int index;

	index = (to->dticks + timeout_list.soon_index) & (NUM_SOON_LISTS - 1);
	to->dticks = index;

	timeout_list.soon_bits |= BIT(index);
	sys_dlist_append(&timeout_list.soon[index], &to->node);
	to->flags |= _TIMEOUT_FLAG_TIMER_WHEEL_SOON;
}

/**
 * Append a timeout to the soon deferred list.
 *
 * This is a special case for timeouts that are exactly NUM_SOON_LISTS into the
 * future when the system is currently announcing timeouts. Such timeouts would
 * be indistinguishable from timeouts on the current rotation of the wheel if
 * they were appended to the appropriate 'soon' list, so we append them to a
 * separate 'soon_defer' list. Timeouts on the 'soon_defer' list are processed
 * at the end of the announcing process and are appended to the appropriate
 * 'soon' list at that time.
 */
static void timeout_wheel_append_soon_defer(struct _timeout *to)
{
	unsigned int index;

	index = (to->dticks + timeout_list.soon_index) & (NUM_SOON_LISTS - 1);
	to->dticks = index + NUM_SOON_LISTS;

	sys_dlist_append(&timeout_list.soon_defer, &to->node);
	to->flags |= _TIMEOUT_FLAG_TIMER_WHEEL_SOON;
}

/**
 * Append a timeout to the appropriate "later" list.
 *
 * As part of the appending process, this routine modifies the timeout's
 * 'dticks' field to both identify which "soon" list into which the timeout may
 * eventually be sifted as well as to which "later" list it is will be appended.
 */
static void timeout_wheel_append_later(struct _timeout *to)
{
	uint32_t position;
	uint32_t soon;
	uint32_t later;

	position = to->dticks + timeout_list.soon_index - NUM_SOON_LISTS;

	soon = position & (NUM_SOON_LISTS - 1);
	later = ((position / NUM_SOON_LISTS) + timeout_list.later_index -
		 timeout_list.sifted) & (NUM_LATER_LISTS - 1);

	to->dticks = (later * NUM_SOON_LISTS) + soon;
	to->flags |= _TIMEOUT_FLAG_TIMER_WHEEL_LATER;

	sys_dlist_append(&timeout_list.later[later], &to->node);
}

/**
 * Insert a timeout into the "distant" list.
 *
 * The "distant" list is ordered by timeout expiration and takes the form of
 * a delta list. The 'dticks' field of each timeout in the "distant" list is
 * the number of ticks until the timeout expires relative to the previous
 * timeout in the list.
 *
 * For optimal performance, the 'distant' list should be short at all times
 * so that the linear insertion time does not become too significant.
 */
static void timeout_wheel_insert_distant(struct _timeout *to, uint32_t cutoff)
{
	struct _timeout *t;

	to->dticks -= cutoff;

	for (t = timeout_wheel_first_in_list(&timeout_list.distant);
	     t != NULL; t = timeout_wheel_next_distant(t)) {
		if (t->dticks > to->dticks) {
			t->dticks -= to->dticks;
			sys_dlist_insert(&t->node, &to->node);
			break;
		}
		to->dticks -= t->dticks;
	}

	if (t == NULL) {
		sys_dlist_append(&timeout_list.distant, &to->node);
	}
}

/**
 * Add a timeout to the timeout wheel. The timeout will never be placed into
 * the 'soon_index' list.
 */
static void timeout_wheel_add_timeout(struct _timeout *to)
{
	__ASSERT_NO_MSG(to->dticks != 0);

	/*
	 * Generally, timeouts that are within NUM_SOON_LISTS ticks into the
	 * future are automatically placed into the appropriate 'soon' list.
	 * However, there is a special case for timeouts that are exactly
	 * NUM_SOON_LISTS into the future. If the system is currently announcing
	 * timeouts, then those timeouts are placed into the 'soon_defer' list
	 * so they do not get mixed up with the currently expiring timeouts.
	 */

	if (to->dticks <= (NUM_SOON_LISTS - timeout_list.announcing)) {
		timeout_wheel_append_soon(to);
		return;
	}

	if (to->dticks == NUM_SOON_LISTS) {
		timeout_wheel_append_soon_defer(to);
		return;
	}

	/*
	 * Timeouts that are beyond NUM_SOON_LISTS ticks into the future are
	 * placed into either a 'later' list or the 'distant' list. The cutoff
	 * between 'later' and 'distant' fluctuates based on 'soon_index'.
	 */

	uint32_t cutoff;

	cutoff = ((NUM_LATER_LISTS + timeout_list.sifted + 1) *
		  NUM_SOON_LISTS) - timeout_list.soon_index - 1;
	if (to->dticks <= cutoff) {
		timeout_wheel_append_later(to);
		return;
	}

	timeout_wheel_insert_distant(to, cutoff);
}

/**
 * Remove the specified timeout.
 */
static void timeout_wheel_remove_timeout(struct _timeout *to)
{
	sys_dlist_remove(&to->node);

	/*
	 * If the timeout is marked as being in a 'soon' list, check if
	 * that list is now empty. If it is, clear the corresponding bit
	 * in 'soon_bits'.
	 */

	if (to->flags & _TIMEOUT_FLAG_TIMER_WHEEL_SOON) {
		unsigned int index = to->dticks;

		if (sys_dlist_is_empty(&timeout_list.soon[index])) {
			timeout_list.soon_bits &= ~BIT(index);
		}
	}

	timeout_wheel_clear_flags(to);
}

/**
 * Calculate the number of ticks until the specified timeout expires.
 */
static k_ticks_t timeout_wheel_remainder(const struct _timeout *to)
{
	if (to->flags & _TIMEOUT_FLAG_TIMER_WHEEL_SOON) {
		uint32_t now = timeout_list.soon_index;
		uint32_t idx = to->dticks;

		if (idx == now) {
			return (1U - timeout_list.announcing) * NUM_SOON_LISTS;
		} else if (idx >= NUM_SOON_LISTS) {
			/* Special case if querying item in 'soon_defer' list */
			return NUM_SOON_LISTS;
		}

		return (idx - now) & (NUM_SOON_LISTS - 1U);
	}

	if (to->flags & _TIMEOUT_FLAG_TIMER_WHEEL_LATER) {
		uint32_t later_slot = to->dticks / NUM_SOON_LISTS;
		uint32_t soon_index = to->dticks & (NUM_SOON_LISTS - 1U);

		/* later slots ahead until the bucket is sifted */
		uint32_t slots_ahead = ((later_slot - timeout_list.later_index) &
					(NUM_LATER_LISTS - 1U)) + timeout_list.sifted;

		return (k_ticks_t)((NUM_SOON_LISTS - timeout_list.soon_index) +
				   (slots_ahead * NUM_SOON_LISTS) + soon_index);
	}

	struct _timeout *t;
	k_ticks_t remaining;

	remaining  = ((NUM_LATER_LISTS + timeout_list.sifted + 1) *
		      NUM_SOON_LISTS) - timeout_list.soon_index - 1;

	for (t = timeout_wheel_first_in_list(&timeout_list.distant);
	     t != NULL; t = timeout_wheel_next_distant(t)) {
		remaining += t->dticks;
		if (t == to) {
			break;
		}
	}

	return remaining;
}

/**
 * Sift from a 'later' list down into the appropriate 'soon' list. Timeouts
 * are prepended to the 'soon' list to preserve their time ordering.
 */
static void timeout_wheel_sift_down_to_soon(void)
{
	sys_dlist_t *list;
	struct _timeout *t;

	list = &timeout_list.later[timeout_list.later_index];
	while ((t = timeout_wheel_last_in_list(list)) != NULL) {
		sys_dlist_remove(&t->node);
		timeout_wheel_clear_flags(t);
		timeout_wheel_prepend_soon(t);
	}
}

/**
 * Sift from the 'distant' list down into the appropriate 'later' list. This
 * is a two step process--preparation and moving. During the preparation phase,
 * timeouts in the distant list are both identified and have their meta-data
 * massaged in anticipation of the move. Massaging the meta-data involves
 * updating both the 'dticks' and 'flags' fields. During the moving phase, the
 * identified timeouts are moved en masse into the appropriate 'later' list.
 */
static void timeout_wheel_sift_down_to_later(void)
{
	struct _timeout *t;
	int64_t sift_ticks = NUM_SOON_LISTS;
	struct _timeout *head = timeout_wheel_first_in_list(&timeout_list.distant);
	struct _timeout *tail = NULL;
	int new_dticks = timeout_list.later_index * NUM_SOON_LISTS;

	for (t = head; t != NULL; t = timeout_wheel_next_distant(t)) {
		if (t->dticks >= sift_ticks) {
			t->dticks -= sift_ticks;
			break;
		}

		sift_ticks -= t->dticks;
		new_dticks += t->dticks;

		t->flags |= _TIMEOUT_FLAG_TIMER_WHEEL_LATER;
		t->dticks = new_dticks;
		tail = t;
	}

	if (tail != NULL) {
		sys_dlist_range_prepend(&timeout_list.later[timeout_list.later_index],
					&head->node, &tail->node);
	}

}

/**
 * Sifting is performed every NUM_SOON_LISTS ticks during
 * timeout_wheel_announce_now(). First, it sifts from the current 'later' list
 * into the appropriate 'soon' lists. This frees up the current 'later' list to
 * become the destination list for the subsequent sifting from 'distant' into
 * this 'later' list.
 */
static void timeout_wheel_sift_down(void)
{
	timeout_wheel_sift_down_to_soon();

	timeout_wheel_sift_down_to_later();

	/* Advance to the next 'later' list */
	timeout_list.later_index++;
	timeout_list.later_index &= (NUM_LATER_LISTS - 1);
	timeout_list.sifted = 1;
}

/**
 * Announce all the timeouts expiring "now".
 *
 * curr_tick has been advanced. announce_remaining is non-zero.
 */
static k_spinlock_key_t timeout_wheel_announce_now(k_spinlock_key_t key)
{
	struct _timeout *t;    /* timeout in soon list */
	uint32_t index = timeout_list.soon_index;
	_timeout_func_t handler;

	timeout_list.sifted = 0;
	timeout_list.announcing = 1;
	for (t = timeout_wheel_first_in_list(&timeout_list.soon[index]);
	     t != NULL;
	     t = timeout_wheel_first_in_list(&timeout_list.soon[index])) {
		sys_dlist_remove(&t->node);
		t->dticks = TIMEOUT_DTICKS_ANNOUNCING;
		handler = t->fn;
		timeout_wheel_clear_flags(t);
		k_spin_unlock(&timeout_lock, key);
		handler(t);
		key = k_spin_lock(&timeout_lock);
	}
	timeout_list.announcing = 0;

	/*
	 * Now that all timeouts expiring 'now' have been processed, the
	 * 'soon_index' slot is ready to have the 'soon deferred' timeouts that
	 * are exactly NUM_SOON_LISTS into the future moved into it.
	 */

	for (t = timeout_wheel_first_in_list(&timeout_list.soon_defer);
	     t != NULL;
	     t = timeout_wheel_first_in_list(&timeout_list.soon_defer)) {
		sys_dlist_remove(&t->node);
		/* Correct the dticks to be the appropriate 'soon' list index */
		t->dticks &= (NUM_SOON_LISTS - 1);
		sys_dlist_append(&timeout_list.soon[t->dticks], &t->node);
	}

	if (index == (NUM_SOON_LISTS - 1)) {
		timeout_wheel_sift_down();
	}

	return key;
}

/**
 * Announce the passage of time by the specified number of ticks specified
 * by the global variable 'announce_remaining'. Invoke the handler of each
 * timeout as it expires.
 */
static k_spinlock_key_t timeout_wheel_announce(k_spinlock_key_t key)
{
	uint32_t mask;
	int      next;    /* Index of next 'soon' list to expire */
	int      num_ticks_to_advance;
	int      correction;  /* NUM_SOON_LISTS or 0 */
	uint32_t bits;

	while (announce_remaining > 0) {

		mask = 0xFFFFFFFF << ((timeout_list.soon_index + 1) & (NUM_SOON_LISTS - 1));
		bits = timeout_list.soon_bits | BIT(NUM_SOON_LISTS - 1);
		next = u32_count_trailing_zeros(mask & bits);
		correction = ((timeout_list.soon_index + 1) / NUM_SOON_LISTS) * NUM_SOON_LISTS;
		num_ticks_to_advance = next + correction - timeout_list.soon_index;

		if (num_ticks_to_advance > announce_remaining) {
			curr_tick += announce_remaining;
			timeout_list.sifted = 0;
			timeout_list.soon_index += announce_remaining;
			timeout_list.soon_index &= (NUM_SOON_LISTS - 1);

			announce_remaining = 0;
			break;
		}

		/*
		 * 1. Advance to the next tick to process.
		 * 2. Clear its bit in the 'soon_bits'.
		 */

		timeout_list.soon_index += num_ticks_to_advance;
		timeout_list.soon_index &= (NUM_SOON_LISTS - 1);
		timeout_list.soon_bits &= ~BIT(timeout_list.soon_index);

		curr_tick += num_ticks_to_advance;
		key = timeout_wheel_announce_now(key);
		announce_remaining -= num_ticks_to_advance;
	}

	return key;
}

/**
 * Determine the number of ticks until the next timeout in the 'soon' list.
 * This yields a value between 1 and NUM_SOON_LISTS. The 'soon_index' list is
 * excluded as it is in the process of timing out.
 */
static int32_t timeout_wheel_next_timeout(int32_t ticks_elapsed)
{
	struct _timeout *to = timeout_wheel_first();
	int32_t ret;
	int64_t dticks = (to != NULL) ? to->dticks : (NUM_SOON_LISTS - 1);

	dticks = ((NUM_SOON_LISTS - 1 - timeout_list.soon_index + dticks) &
		  (NUM_SOON_LISTS - 1)) + 1;

	if ((int64_t)(dticks - ticks_elapsed) > (int64_t)INT_MAX) {
		ret = SYS_CLOCK_MAX_WAIT;
	} else {
		ret = max(0, dticks - ticks_elapsed);
	}

	return ret;
}
#endif

static int32_t elapsed(void)
{
	/* While sys_clock_announce() is executing, new relative timeouts will be
	 * scheduled relatively to the currently firing timeout's original tick
	 * value (=curr_tick) rather than relative to the current
	 * sys_clock_elapsed().
	 *
	 * This means that timeouts being scheduled from within timeout callbacks
	 * will be scheduled at well-defined offsets from the currently firing
	 * timeout.
	 *
	 * As a side effect, the same will happen if an ISR with higher priority
	 * preempts a timeout callback and schedules a timeout.
	 *
	 * The distinction is implemented by looking at announce_remaining which
	 * will be non-zero while sys_clock_announce() is executing and zero
	 * otherwise.
	 */
	return announce_remaining == 0 ? sys_clock_elapsed() : 0U;
}

k_ticks_t z_add_timeout(struct _timeout *to, _timeout_func_t fn, k_timeout_t timeout)
{
	k_ticks_t ticks = 0;

	if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		return 0;
	}

#ifdef CONFIG_KERNEL_COHERENCE
	__ASSERT_NO_MSG(sys_cache_is_mem_coherent(to));
#endif /* CONFIG_KERNEL_COHERENCE */

	__ASSERT(!sys_dnode_is_linked(&to->node), "");
	to->fn = fn;

	K_SPINLOCK(&timeout_lock) {
		int32_t ticks_elapsed;
		bool has_elapsed = false;

		if (Z_IS_TIMEOUT_RELATIVE(timeout)) {
			ticks_elapsed = elapsed();
			has_elapsed = true;
			to->dticks = timeout.ticks + 1 + ticks_elapsed;
			ticks = curr_tick + to->dticks;
		} else {
			k_ticks_t dticks = Z_TICK_ABS(timeout.ticks) - curr_tick;

			to->dticks = max(1, dticks);
			ticks = timeout.ticks;
		}

		timeout_q_add_timeout(to);

		if ((to == timeout_q_first()) &&
		    (announce_remaining == 0)) {
			if (!has_elapsed) {
				/* In case of absolute timeout that is first to expire
				 * elapsed need to be read from the system clock.
				 */
				ticks_elapsed = elapsed();
			}
			sys_clock_set_timeout(timeout_q_next_timeout(ticks_elapsed), false);
		}
	}

	return ticks;
}

int z_abort_timeout(struct _timeout *to)
{
	int ret = -EINVAL;

	K_SPINLOCK(&timeout_lock) {
		if (sys_dnode_is_linked(&to->node)) {
			bool is_first = (to == timeout_q_first());

			timeout_q_remove_timeout(to);
			to->dticks = TIMEOUT_DTICKS_ABORTED;
			ret = 0;
			if (is_first) {
				sys_clock_set_timeout(timeout_q_next_timeout(elapsed()), false);
			}
		} else if (to->dticks == TIMEOUT_DTICKS_ANNOUNCING) {
			to->dticks = TIMEOUT_DTICKS_ABORTED;
		}
	}

	return ret;
}

k_ticks_t z_timeout_remaining(const struct _timeout *timeout)
{
	k_ticks_t ticks = 0;

	K_SPINLOCK(&timeout_lock) {
		if (!z_is_inactive_timeout(timeout)) {
			ticks = timeout_q_remainder(timeout) - elapsed();
		}
	}

	return ticks;
}
EXPORT_SYMBOL(z_timeout_remaining);

k_ticks_t z_timeout_expires(const struct _timeout *timeout)
{
	k_ticks_t ticks = 0;

	K_SPINLOCK(&timeout_lock) {
		ticks = curr_tick;
		if (!z_is_inactive_timeout(timeout)) {
			ticks += timeout_q_remainder(timeout);
		}
	}

	return ticks;
}
EXPORT_SYMBOL(z_timeout_expires);

int32_t z_get_next_timeout_expiry(void)
{
	int32_t ret = (int32_t) K_TICKS_FOREVER;

	K_SPINLOCK(&timeout_lock) {
		ret = timeout_q_next_timeout(elapsed());
	}
	return ret;
}

void sys_clock_announce_locked(int32_t ticks, k_spinlock_key_t key)
{
	/* We release the lock around the callbacks below, so on SMP
	 * systems someone might be already running the loop.  Don't
	 * race (which will cause parallel execution of "sequential"
	 * timeouts and confuse apps), just increment the tick count
	 * and return.
	 */
	if (IS_ENABLED(CONFIG_SMP) && (announce_remaining != 0)) {
		announce_remaining += ticks;
		k_spin_unlock(&timeout_lock, key);
		return;
	}

	announce_remaining = ticks;

	key = timeout_q_announce(key);

	sys_clock_set_timeout(timeout_q_next_timeout(0), false);

	k_spin_unlock(&timeout_lock, key);

#ifdef CONFIG_TIMESLICING
	z_time_slice();
#endif /* CONFIG_TIMESLICING */
}

#if defined(CONFIG_SMP) || defined(CONFIG_SPIN_VALIDATE)
k_spinlock_key_t sys_clock_lock(void)
{
	return k_spin_lock(&timeout_lock);
}

void sys_clock_unlock(k_spinlock_key_t key)
{
	k_spin_unlock(&timeout_lock, key);
}
#endif

#if defined(CONFIG_TEST) || defined(CONFIG_ASSERT)
bool sys_clock_is_locked(void)
{
	return z_spin_is_locked(&timeout_lock);
}
#endif

int64_t sys_clock_tick_get(void)
{
	uint64_t t = 0U;

	K_SPINLOCK(&timeout_lock) {
		t = curr_tick + elapsed();
	}
	return t;
}

uint32_t sys_clock_tick_get_32(void)
{
#ifdef CONFIG_TICKLESS_KERNEL
	return (uint32_t)sys_clock_tick_get();
#else
	return (uint32_t)curr_tick;
#endif /* CONFIG_TICKLESS_KERNEL */
}

int64_t z_impl_k_uptime_ticks(void)
{
	return sys_clock_tick_get();
}

#ifdef CONFIG_USERSPACE
static inline int64_t z_vrfy_k_uptime_ticks(void)
{
	return z_impl_k_uptime_ticks();
}
#include <zephyr/syscalls/k_uptime_ticks_mrsh.c>
#endif /* CONFIG_USERSPACE */

k_timepoint_t sys_timepoint_calc(k_timeout_t timeout)
{
	k_timepoint_t timepoint;

	if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		timepoint.tick = UINT64_MAX;
	} else if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		timepoint.tick = 0;
	} else {
		k_ticks_t dt = timeout.ticks;

		if (Z_IS_TIMEOUT_RELATIVE(timeout)) {
			timepoint.tick = sys_clock_tick_get() + max(1, dt);
		} else {
			timepoint.tick = Z_TICK_ABS(dt);
		}
	}

	return timepoint;
}

k_timeout_t sys_timepoint_timeout(k_timepoint_t timepoint)
{
	uint64_t now, remaining;

	if (timepoint.tick == UINT64_MAX) {
		return K_FOREVER;
	}
	if (timepoint.tick == 0) {
		return K_NO_WAIT;
	}

	now = sys_clock_tick_get();
	remaining = (timepoint.tick > now) ? (timepoint.tick - now) : 0;
	return K_TICKS(remaining);
}

#ifdef CONFIG_ZTEST
void z_impl_sys_clock_tick_set(uint64_t tick)
{
	curr_tick = tick;
}

void z_vrfy_sys_clock_tick_set(uint64_t tick)
{
	z_impl_sys_clock_tick_set(tick);
}
#endif /* CONFIG_ZTEST */

/* Initialize the timeout management subsystem before the timer driver */
SYS_INIT(timeout_q_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
