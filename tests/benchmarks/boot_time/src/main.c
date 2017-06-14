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
 *  1. From reset to kernel's __start
 *  2. From __start to main()
 *  3. From __start to task
 *  4. From __start to idle
 */

#include <zephyr.h>
#include <tc_util.h>

/* externs */
extern u64_t __start_time_stamp;    /* timestamp when kernel begins executing */
extern u64_t __main_time_stamp;     /* timestamp when main() begins executing */
extern u64_t __idle_time_stamp;     /* timestamp when CPU went idle */

void main(void)
{
	u64_t task_time_stamp;      /* timestamp at beginning of first task  */
	u64_t _start_us;     /* being of __start timestamp in us	 */
	u64_t main_us;       /* begin of main timestamp in us	 */
	u64_t task_us;       /* begin of task timestamp in us	 */
	u64_t s_main_time_stamp;    /* __start->main timestamp		 */
	u64_t s_task_time_stamp;    /*__start->task timestamp		 */
	u64_t idle_us;       /* begin of idle timestamp in us	 */
	u64_t s_idle_time_stamp;    /*__start->idle timestamp		 */

	task_time_stamp = (u64_t)k_cycle_get_32();

	/*
	 * Go to sleep for 1 tick in order to timestamp when idle thread halts.
	 */
	k_sleep(1);

	int freq = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / 1000000;

	_start_us  =  __start_time_stamp / freq;
	s_main_time_stamp =  __main_time_stamp - __start_time_stamp;
	main_us    =  s_main_time_stamp / freq;
	s_task_time_stamp =  task_time_stamp   - __start_time_stamp;
	task_us    =  s_task_time_stamp / freq;
	s_idle_time_stamp =  __idle_time_stamp - __start_time_stamp;
	idle_us    =  s_idle_time_stamp / freq;

	/* Indicate start for sanity test suite */
	TC_START("Boot Time Measurement");

	/* Only print lower 32bit of time result */
	TC_PRINT("Boot Result: Clock Frequency: %d MHz\n",
		 freq);
	TC_PRINT("__start       : %d cycles, %d us\n",
		 (u32_t)(__start_time_stamp & 0xFFFFFFFFULL),
		 (u32_t) (_start_us  & 0xFFFFFFFFULL));
	TC_PRINT("_start->main(): %d cycles, %d us\n",
		 (u32_t)(s_main_time_stamp & 0xFFFFFFFFULL),
		 (u32_t)  (main_us  & 0xFFFFFFFFULL));
	TC_PRINT("_start->task  : %d cycles, %d us\n",
		 (u32_t)(s_task_time_stamp & 0xFFFFFFFFULL),
		 (u32_t)  (task_us  & 0xFFFFFFFFULL));
	TC_PRINT("_start->idle  : %d cycles, %d us\n",
		 (u32_t)(s_idle_time_stamp & 0xFFFFFFFFULL),
		 (u32_t)  (idle_us  & 0xFFFFFFFFULL));

	TC_PRINT("Boot Time Measurement finished\n");

	/* for sanity regression test utility. */
	TC_END_RESULT(TC_PASS);
	TC_END_REPORT(TC_PASS);

}
