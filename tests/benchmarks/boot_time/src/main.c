/*
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Measure boot time
 *
 * Measuring the boot time
 *  1. From __start to main()
 *  2. From __start to task
 *  3. From __start to idle
 */

#include <zephyr.h>
#include <tc_util.h>

void main(void)
{
	u32_t task_time_stamp;	/* timestamp at beginning of first task */
	u32_t main_us;		/* begin of main timestamp in us */
	u32_t task_us;		/* begin of task timestamp in us */
	u32_t idle_us;		/* begin of idle timestamp in us */

	task_time_stamp = k_cycle_get_32();

	/*
	 * Go to sleep for 1 tick in order to timestamp when idle thread halts.
	 */
	k_sleep(1);

	int freq = sys_clock_hw_cycles_per_sec() / 1000000;

	main_us = __main_time_stamp / freq;
	task_us = task_time_stamp / freq;
	idle_us = __idle_time_stamp / freq;

	TC_START("Boot Time Measurement");
	TC_PRINT("Boot Result: Clock Frequency: %d MHz\n", freq);
	TC_PRINT("_start->main(): %u cycles, %u us\n", __main_time_stamp,
						       main_us);
	TC_PRINT("_start->task  : %u cycles, %u us\n", task_time_stamp,
						       task_us);
	TC_PRINT("_start->idle  : %u cycles, %u us\n", __idle_time_stamp,
						       idle_us);
	TC_PRINT("Boot Time Measurement finished\n");

	TC_END_RESULT(TC_PASS);
	TC_END_REPORT(TC_PASS);
}
