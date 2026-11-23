/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#define STACK_SIZE 1024
#define WORKERS    2

K_THREAD_STACK_ARRAY_DEFINE(stacks, WORKERS, STACK_SIZE);

static uint32_t rounds = 10000;
static uint32_t start_cyc[WORKERS];
static uint32_t end_cyc[WORKERS];

/* A bounded float series whose result depends on every intermediate rounding */
static __noinline float series(float seed)
{
	float acc = seed;

	for (uint32_t i = 0; i < rounds; i++) {
		acc = acc * 0.999f + seed / (float)((i & 1023U) + 1U);
	}

	return acc;
}

static void worker(void *p1, void *p2, void *p3)
{
	int idx = (int)(uintptr_t)p3;

	start_cyc[idx] = k_cycle_get_32();
	*(float *)p2 = series(*(float *)p1);
	end_cyc[idx] = k_cycle_get_32();
}

/* Two time-sliced threads must match a run without preemption */
ZTEST(tricore_fpu_sharing, test_preempted_float_threads)
{
	static struct k_thread threads[WORKERS];
	static float seeds[WORKERS] = {1.5f, 2.25f};
	static float results[WORKERS];
	uint32_t slice_ms = MAX(CONFIG_TIMESLICE_SIZE, k_ticks_to_ms_ceil32(1));
	float expected[WORKERS];
	uint32_t start;

	for (;;) {
		start = k_uptime_get_32();
		expected[0] = series(seeds[0]);
		if (k_uptime_get_32() - start >= 4U * slice_ms) {
			break;
		}
		rounds *= 2U;
	}
	expected[1] = series(seeds[1]);

	for (int i = 0; i < WORKERS; i++) {
		k_thread_create(&threads[i], stacks[i], STACK_SIZE, worker, &seeds[i], &results[i],
				(void *)(uintptr_t)i, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
	}
	for (int i = 0; i < WORKERS; i++) {
		k_thread_join(&threads[i], K_FOREVER);
		zassert_equal(results[i], expected[i], "thread %d: %f != %f", i, (double)results[i],
			      (double)expected[i]);
	}

	/* Time slicing must have interleaved the two threads */
	zassert_true((int32_t)(start_cyc[1] - end_cyc[0]) < 0, "threads did not interleave");
}

ZTEST(tricore_fpu_sharing, test_float_disable_enable)
{
	zassert_equal(k_float_disable(k_current_get()), -ENOTSUP);
	zassert_equal(k_float_enable(k_current_get(), 0), -ENOTSUP);
}

ZTEST_SUITE(tricore_fpu_sharing, NULL, NULL, NULL, NULL, NULL);
