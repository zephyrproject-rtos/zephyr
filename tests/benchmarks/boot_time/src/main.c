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
#include <kernel_internal.h>

void main(void)
{
	uint32_t task_time_stamp;	/* timestamp at beginning of first task */
	uint32_t main_us;		/* begin of main timestamp in us */
	uint32_t task_us;		/* begin of task timestamp in us */
	uint32_t idle_us;		/* begin of idle timestamp in us */

	task_time_stamp = k_cycle_get_32();

	/*
	 * Go to sleep for 1 tick in order to timestamp when idle thread halts.
	 */
	k_sleep(K_MSEC(1));

	main_us = (uint32_t)ceiling_fraction(USEC_PER_SEC *
					  (uint64_t)z_timestamp_main,
					  sys_clock_hw_cycles_per_sec());
	task_us = (uint32_t)ceiling_fraction(USEC_PER_SEC *
					  (uint64_t)task_time_stamp,
					  sys_clock_hw_cycles_per_sec());
	idle_us = (uint32_t)ceiling_fraction(USEC_PER_SEC *
					  (uint64_t)z_timestamp_idle,
					  sys_clock_hw_cycles_per_sec());

	TC_START("Boot Time Measurement");
	TC_PRINT("Boot Result: Clock Frequency: %d Hz\n",
					  sys_clock_hw_cycles_per_sec());
	TC_PRINT("_start->main(): %u cycles, %u us\n", z_timestamp_main,
						       main_us);
	TC_PRINT("_start->task  : %u cycles, %u us\n", task_time_stamp,
						       task_us);
	TC_PRINT("_start->idle  : %u cycles, %u us\n", z_timestamp_idle,
						       idle_us);
	TC_PRINT("Boot Time Measurement finished\n");

	TC_END_RESULT(TC_PASS);
	TC_END_REPORT(TC_PASS);
}
