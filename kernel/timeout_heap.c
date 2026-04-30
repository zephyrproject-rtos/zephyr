/**
 * Copyright (c) 2026 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Kernel timeout subsystem - min-heap backend
 */

#ifndef CONFIG_TIMEOUT_USE_MIN_HEAP
#error "timeout_heap.c requires CONFIG_TIMEOUT_USE_MIN_HEAP=y"
#endif

#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/min_heap.h>
#include <ksched.h>
#include <timeout_q.h>
#include <zephyr/internal/syscall_handler.h>
#include <timeslicing.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <zephyr/llext/symbol.h>
#include "timeout_priv.h"

#define TIMEOUT_HEAP_MAX CONFIG_TIMEOUT_HEAP_MAX_ENTRIES

struct k_spinlock timeout_lock;

/* Absolute tick counter: number of ticks since boot */
uint64_t curr_tick;

/* Ticks left to process in the currently executing sys_clock_announce(). */
int announce_remaining;

static int timeout_cmp(const struct min_heap_handle *a, const struct min_heap_handle *b)
{
	const struct _timeout *ta = CONTAINER_OF(a, struct _timeout, heap_handle);
	const struct _timeout *tb = CONTAINER_OF(b, struct _timeout, heap_handle);

	if (ta->abs_ticks < tb->abs_ticks) {
		return -1;
	}

	return ta->abs_ticks > tb->abs_ticks ? 1 : 0;
}

MIN_HEAP_DEFINE_STATIC(timeout_heap_inst, TIMEOUT_HEAP_MAX, timeout_cmp);

/* Helper functions */

static int32_t heap_next_ticks(int32_t ticks_elapsed)
{
	if (min_heap_is_empty(&timeout_heap_inst)) {
		return SYS_CLOCK_MAX_WAIT;
	}

	const struct _timeout *t =
		CONTAINER_OF(min_heap_peek(&timeout_heap_inst), struct _timeout, heap_handle);
	int64_t remaining = t->abs_ticks - (int64_t)curr_tick - ticks_elapsed;

	if (remaining > (int64_t)INT32_MAX) {
		return SYS_CLOCK_MAX_WAIT;
	}

	return (int32_t)MAX(0, remaining);
}

/**
 * Public kernel timeout API
 */

k_ticks_t z_add_timeout(struct _timeout *to, _timeout_func_t fn, k_timeout_t timeout)
{
	k_ticks_t ret;

	if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		return 0;
	}

#ifdef CONFIG_KERNEL_COHERENCE
	__ASSERT_NO_MSG(sys_cache_is_mem_coherent(to));
#endif
	__ASSERT_NO_MSG(z_is_inactive_timeout(to));

	to->fn = fn;

	K_SPINLOCK(&timeout_lock) {
		int32_t ticks_elapsed;
		bool reprogram = false;

		if (Z_IS_TIMEOUT_RELATIVE(timeout)) {
			ticks_elapsed = elapsed();
			to->abs_ticks = (int64_t)curr_tick + ticks_elapsed + timeout.ticks + 1;
			ret = to->abs_ticks;
		} else {
			int64_t abs = Z_TICK_ABS(timeout.ticks);

			to->abs_ticks = MAX(abs, (int64_t)curr_tick + 1);
			ret = to->abs_ticks;
			ticks_elapsed = elapsed();
		}

		if (unlikely(min_heap_push(&timeout_heap_inst, &to->heap_handle) != 0)) {
			/*
			 * Heap overflow is always a configuration error.
			 * Increase CONFIG_TIMEOUT_HEAP_MAX_ENTRIES.
			 */
			k_panic();
		}

		const struct _timeout *root = CONTAINER_OF(min_heap_peek(&timeout_heap_inst),
							   struct _timeout, heap_handle);

		if (root == to && announce_remaining == 0) {
			reprogram = true;
		}

		if (reprogram) {
			sys_clock_set_timeout(heap_next_ticks(ticks_elapsed), false);
		}
	}

	return ret;
}

int z_abort_timeout(struct _timeout *to)
{
	int ret = -EINVAL;

	K_SPINLOCK(&timeout_lock) {
		if (to->heap_handle.idx > 0U) {
			/* Timeout is active in the heap - remove it */
			bool was_min = (to->heap_handle.idx == 1U);

			min_heap_remove(&timeout_heap_inst, &to->heap_handle);
			to->abs_ticks = TIMEOUT_ABS_TICKS_ABORTED;
			ret = 0;

			if (was_min && announce_remaining == 0) {
				sys_clock_set_timeout(heap_next_ticks(elapsed()), false);
			}
		} else if (to->abs_ticks == TIMEOUT_ABS_TICKS_ANNOUNCING) {
			to->abs_ticks = TIMEOUT_ABS_TICKS_ABORTED;
			ret = 0;
		}
	}

	return ret;
}

k_ticks_t z_timeout_remaining(const struct _timeout *timeout)
{
	k_ticks_t ticks = 0;

	K_SPINLOCK(&timeout_lock) {
		if (!z_is_inactive_timeout(timeout)) {
			int64_t rem = timeout->abs_ticks - (int64_t)curr_tick - elapsed();

			ticks = (k_ticks_t)MAX(0, rem);
		}
	}

	return ticks;
}
EXPORT_SYMBOL(z_timeout_remaining);

k_ticks_t z_timeout_expires(const struct _timeout *timeout)
{
	k_ticks_t ticks = 0;

	K_SPINLOCK(&timeout_lock) {
		ticks = z_is_inactive_timeout(timeout) ? (k_ticks_t)curr_tick
						       : (k_ticks_t)timeout->abs_ticks;
	}

	return ticks;
}
EXPORT_SYMBOL(z_timeout_expires);

int32_t z_get_next_timeout_expiry(void)
{
	int32_t ret = (int32_t)K_TICKS_FOREVER;

	K_SPINLOCK(&timeout_lock) {
		ret = heap_next_ticks(elapsed());
	}

	return ret;
}

void sys_clock_announce_locked(int32_t ticks, k_spinlock_key_t key)
{
	if (IS_ENABLED(CONFIG_SMP) && announce_remaining != 0) {
		announce_remaining += ticks;
		k_spin_unlock(&timeout_lock, key);

		return;
	}

	announce_remaining = ticks;

	while (!min_heap_is_empty(&timeout_heap_inst)) {
		const struct _timeout *t = CONTAINER_OF(min_heap_peek(&timeout_heap_inst),
							struct _timeout, heap_handle);

		if (t->abs_ticks > (int64_t)curr_tick + announce_remaining) {
			break;
		}

		int32_t dt = (int32_t)(t->abs_ticks - (int64_t)curr_tick);

		curr_tick = t->abs_ticks;

		struct min_heap_handle *popped_h = min_heap_pop(&timeout_heap_inst);
		struct _timeout *popped = CONTAINER_OF(popped_h, struct _timeout, heap_handle);

		popped->abs_ticks = TIMEOUT_ABS_TICKS_ANNOUNCING;

		k_spin_unlock(&timeout_lock, key);
		popped->fn(popped);
		key = k_spin_lock(&timeout_lock);
		announce_remaining -= dt;
	}

	curr_tick += announce_remaining;
	announce_remaining = 0;

	sys_clock_set_timeout(heap_next_ticks(0), false);

	k_spin_unlock(&timeout_lock, key);

#ifdef CONFIG_TIMESLICING
	z_time_slice();
#endif
}
