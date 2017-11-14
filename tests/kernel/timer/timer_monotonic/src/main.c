/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <tc_util.h>
#include <ztest.h>

int test_frequency(void)
{
	u32_t start, end, delta, pct;

	TC_PRINT("Testing system tick frequency\n");

	start = k_cycle_get_32();
	k_sleep(1000);
	end = k_cycle_get_32();

	delta = end - start;
	pct = (u64_t)delta * 100 / sys_clock_hw_cycles_per_sec;

	printk("delta: %u  expected: %u  %u%%\n", delta,
	       sys_clock_hw_cycles_per_sec, pct);

	/* Heuristic: if we're more than 10% off, throw an error */
	if (pct < 90 || pct > 110) {
		TC_PRINT("Clock calibration is way off!\n");
		return -1;
	}

	return 0;
}


void test_timer(void)
{
	u32_t t_last, t_now, i, errors;
	s32_t diff;

	errors = 0;

	TC_PRINT("sys_clock_us_per_tick = %d\n", sys_clock_us_per_tick);
	TC_PRINT("sys_clock_hw_cycles_per_tick = %d\n",
		 sys_clock_hw_cycles_per_tick);
	TC_PRINT("sys_clock_hw_cycles_per_sec = %d\n",
		 sys_clock_hw_cycles_per_sec);

	TC_START("test monotonic timer");

	t_last = k_cycle_get_32();

	for (i = 0; i < 1000000; i++) {
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

void test_main(void)
{
	ztest_test_suite(test_timer_fn, ztest_unit_test(test_timer));
	ztest_run_test_suite(test_timer_fn);
}
