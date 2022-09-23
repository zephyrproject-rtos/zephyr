/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

int test_frequency(void)
{
	volatile uint32_t start, end;
	uint32_t delta, pct;

	TC_PRINT("Testing system tick frequency\n");

	start = k_cycle_get_32();
	k_sleep(K_MSEC(1000));
	end = k_cycle_get_32();

	delta = end - start;
	pct = (uint64_t)delta * 100U / sys_clock_hw_cycles_per_sec();

	printk("delta: %u  expected: %u  %u%%\n", delta,
	       sys_clock_hw_cycles_per_sec(), pct);

	/* Heuristic: if we're more than 10% off, throw an error */
	if (pct < 90 || pct > 110) {
		TC_PRINT("Clock calibration is way off!\n");
		return -1;
	}

	return 0;
}

/**
 * @brief Test monotonic timer
 *
 * Validates monotonic timer's clock calibration.
 *
 * It reads the System clock’s h/w timer frequency value continuously
 * using k_cycle_get_32() to verify its working and correctness.
 * It also checks system tick frequency by checking the delta error
 * between generated and system clock provided HW cycles per sec values.
 *
 * @ingroup kernel_timer_tests
 *
 * @see k_cycle_get_32(), sys_clock_hw_cycles_per_sec()
 */
ZTEST(timer_fn, test_timer)
{
	volatile uint32_t t_last, t_now;
	uint32_t i, errors;
	int32_t diff;

	errors = 0U;

	TC_PRINT("k_ticks_to_cyc_floor32(1) = %d\n",
		 k_ticks_to_cyc_floor32(1));
	TC_PRINT("sys_clock_hw_cycles_per_sec() = %d\n",
		 sys_clock_hw_cycles_per_sec());

	TC_START("test monotonic timer");

	t_last = k_cycle_get_32();

	for (i = 0U; i < 1000000; i++) {
		t_now = k_cycle_get_32();

		if (t_now < t_last) {
			diff = t_now - t_last;
			TC_PRINT("diff = %d (t_last = %u : t_now = %u);"
				"i = %u\n", diff, t_last, t_now, i);
			errors++;
		}
		t_last = t_now;
	}

	zassert_false(errors, "errors = %d\n", errors);

	zassert_false(test_frequency(), "test frequency failed");
}

ZTEST_SUITE(timer_fn, NULL, NULL, NULL, NULL, NULL);
