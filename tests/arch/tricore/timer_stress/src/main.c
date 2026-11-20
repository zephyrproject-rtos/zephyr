/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#define SHORT_SLEEP_ITERS 1000
#define STRESS_ITERS      2000

static atomic_t timer_fires;

static void on_timer(struct k_timer *t)
{
	ARG_UNUSED(t);
	atomic_inc(&timer_fires);
}

static K_TIMER_DEFINE(stress_timer, on_timer, NULL);

/*
 * A short k_sleep used to stall for a full 32-bit STM compare window
 * (~8.6 s at 500 MHz, ~43 s at 100 MHz) when the compare value landed just
 * behind the live counter. No short sleep may take anywhere near that long.
 */
ZTEST(tricore_timer, test_short_sleep_no_stall)
{
	uint64_t ceiling = sys_clock_hw_cycles_per_sec() / 10;

	for (int i = 0; i < SHORT_SLEEP_ITERS; i++) {
		int ticks = 1 + (i % 4);
		uint64_t t0 = k_cycle_get_64();

		k_sleep(K_TICKS(ticks));

		uint64_t dt = k_cycle_get_64() - t0;

		zassert_true(dt < ceiling,
			     "k_sleep(%d ticks) iter %d took %llu cycles (>%llu) - STM wrap stall",
			     ticks, i, dt, ceiling);
	}
}

ZTEST(tricore_timer, test_sleep_accuracy)
{
	uint64_t hz = sys_clock_hw_cycles_per_sec();
	uint64_t lo = (hz * 45) / 1000;
	uint64_t hi = (hz * 80) / 1000;

	for (int i = 0; i < 10; i++) {
		uint64_t t0 = k_cycle_get_64();

		k_sleep(K_MSEC(50));

		uint64_t dt = k_cycle_get_64() - t0;

		zassert_true(dt >= lo && dt <= hi,
			     "k_sleep(50ms) iter %d = %llu cycles, want [%llu,%llu]", i, dt, lo,
			     hi);
	}
}

ZTEST(tricore_timer, test_timer_periodic)
{
	atomic_set(&timer_fires, 0);
	k_timer_start(&stress_timer, K_MSEC(5), K_MSEC(5));
	k_sleep(K_MSEC(500));
	k_timer_stop(&stress_timer);

	int fires = atomic_get(&timer_fires);

	zassert_true(fires >= 90 && fires <= 110, "periodic timer fired %d times, want ~100",
		     fires);
}

ZTEST(tricore_timer, test_timer_sleep_stress)
{
	uint64_t ceiling = sys_clock_hw_cycles_per_sec() / 10;

	atomic_set(&timer_fires, 0);
	k_timer_start(&stress_timer, K_MSEC(1), K_MSEC(1));

	for (int i = 0; i < STRESS_ITERS; i++) {
		int ticks = 1 + (i % 3);
		uint64_t t0 = k_cycle_get_64();

		k_sleep(K_TICKS(ticks));

		uint64_t dt = k_cycle_get_64() - t0;

		zassert_true(dt < ceiling, "stress iter %d k_sleep(%d) = %llu cycles - stall", i,
			     ticks, dt);
	}

	k_timer_stop(&stress_timer);

	zassert_true(atomic_get(&timer_fires) > 0, "timer never fired under sleep load");
}

ZTEST_SUITE(tricore_timer, NULL, NULL, NULL, NULL, NULL);
