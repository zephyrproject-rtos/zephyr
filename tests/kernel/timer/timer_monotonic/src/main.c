/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <inttypes.h>
#include <tc_util.h>

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


void main(void)
{
	u32_t t_last, t_now, i, errors;
	s32_t diff;
	int rv = TC_PASS;

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
			TC_PRINT("diff = %" PRId32 " (t_last = %" PRIu32
				 " : t_now = %" PRIu32 "); i = %u\n",
				 diff, t_last, t_now, i);
			errors++;
		}
		t_last = t_now;
	}

	if (errors) {
		TC_PRINT("errors = %d\n", errors);
		rv = TC_FAIL;
	} else {
		TC_PRINT("Cycle results appear to be monotonic\n");
	}

	if (test_frequency()) {
		rv = TC_FAIL;
	}

	TC_END_REPORT(rv);
}

