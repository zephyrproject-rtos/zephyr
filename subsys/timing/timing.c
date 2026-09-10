/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/timing/timing.h>

static bool has_inited;
static atomic_val_t started_ref;

void timing_init(void)
{
	if (has_inited) {
		return;
	}

#if defined(CONFIG_BOARD_HAS_TIMING_FUNCTIONS)
	board_timing_init();
#elif defined(CONFIG_SOC_HAS_TIMING_FUNCTIONS)
	soc_timing_init();
#else
	arch_timing_init();
#endif

	has_inited = true;
}

void timing_start(void)
{
	if (atomic_inc(&started_ref) != 0) {
		return;
	}

#if defined(CONFIG_BOARD_HAS_TIMING_FUNCTIONS)
	board_timing_start();
#elif defined(CONFIG_SOC_HAS_TIMING_FUNCTIONS)
	soc_timing_start();
#else
	arch_timing_start();
#endif
}

void timing_stop(void)
{
	atomic_t old_value, new_value;

	/* Make sure this does decrement past zero. */
	do {
		old_value = atomic_get(&started_ref);
		if (old_value <= 0) {
			break;
		}

		new_value = old_value - 1;
	} while (atomic_cas(&started_ref, old_value, new_value) == 0);

	/*
	 * new_value may be uninitialized, so use old_value here.
	 */
	if (old_value > 1) {
		return;
	}

#if defined(CONFIG_BOARD_HAS_TIMING_FUNCTIONS)
	board_timing_stop();
#elif defined(CONFIG_SOC_HAS_TIMING_FUNCTIONS)
	soc_timing_stop();
#else
	arch_timing_stop();
#endif
}

uint64_t timing_ns_get(void)
{
	static struct k_spinlock lock;
	static timing_t last;
	static uint64_t total_cycles;
	k_spinlock_key_t key = k_spin_lock(&lock);
	timing_t now = timing_counter_get();

	/*
	 * The timing counter may be narrower than 64 bits and wrap. Accumulate
	 * the interval since the previous reading rather than the raw counter,
	 * so the returned timestamp stays monotonic across wraps. Each interval
	 * is a non-negative value, so the total never decreases regardless of
	 * how often this is called: monotonicity is guaranteed unconditionally.
	 * timing_cycles_get() resolves only a single wrap though, so if more
	 * than one wrap happens between two calls the total undercounts the
	 * elapsed time; the timestamp stays monotonic but loses absolute
	 * accuracy. The spinlock keeps the shared accumulator consistent on SMP.
	 *
	 * The accumulator is deliberately not seeded on the first call, so
	 * timestamps are offset by the counter value at that point rather than
	 * starting from zero. That only shifts the origin, not monotonicity, and
	 * avoids a first-call test on every subsequent call.
	 */
	total_cycles += timing_cycles_get(&last, &now);
	last = now;

	k_spin_unlock(&lock, key);

	return timing_cycles_to_ns(total_cycles);
}
