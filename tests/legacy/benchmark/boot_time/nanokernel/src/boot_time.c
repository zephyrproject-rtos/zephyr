/* boot_time.c - Boot Time measurement task */

/*
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Measure boot time for nanokernel project
 *
 * Measuring the boot time for the nanokernel project includes
 *  1. From reset to kernel's __start
 *  2. From __start to main()
 *  3. From __start to task
 */

#include <zephyr.h>
#include <tc_util.h>

/* externs */
extern uint64_t __start_tsc; /* timestamp when kernel begins executing */
extern uint64_t __main_tsc;  /* timestamp when main() begins executing */

void bootTimeTask(void)
{
	uint64_t task_tsc;  /* timestamp at beginning of first task  */
	uint64_t _start_us; /* being of __start timestamp in us	 */
	uint64_t main_us;   /* begin of main timestamp in us	 */
	uint64_t task_us;   /* begin of task timestamp in us	 */
	uint64_t s_main_tsc; /* __start->main timestamp		 */
	uint64_t s_task_tsc;  /*__start->task timestamp		 */

	task_tsc = _tsc_read();

	_start_us = __start_tsc / CONFIG_CPU_CLOCK_FREQ_MHZ;
	s_main_tsc = __main_tsc-__start_tsc;
	main_us   = s_main_tsc / CONFIG_CPU_CLOCK_FREQ_MHZ;
	s_task_tsc = task_tsc-__start_tsc;
	task_us   = s_task_tsc / CONFIG_CPU_CLOCK_FREQ_MHZ;

	/* Indicate start for sanity test suite */
	TC_START("Boot Time Measurement");

	/* Only print lower 32bit of time result */
	TC_PRINT("NanoKernel Boot Result: Clock Frequency: %d MHz\n",
			 CONFIG_CPU_CLOCK_FREQ_MHZ);
	TC_PRINT("__start       : %d cycles, %d us\n",
			 (uint32_t)(__start_tsc & 0xFFFFFFFFULL),
			 (uint32_t) (_start_us  & 0xFFFFFFFFULL));
	TC_PRINT("_start->main(): %d cycles, %d us\n",
			 (uint32_t)(s_main_tsc & 0xFFFFFFFFULL),
			 (uint32_t)  (main_us  & 0xFFFFFFFFULL));
	TC_PRINT("_start->task  : %d cycles, %d us\n",
			 (uint32_t)(s_task_tsc & 0xFFFFFFFFULL),
			 (uint32_t)  (task_us  & 0xFFFFFFFFULL));

	TC_PRINT("Boot Time Measurement finished\n");

	/* for sanity regression test utility. */
	TC_END_RESULT(TC_PASS);
	TC_END_REPORT(TC_PASS);

}

char __stack fiberStack[512];

/**
 *
 * @brief Nanokernel entry point
 *
 * @return N/A
 */

void main(void)
{
	/* record timestamp for nanokernel's main() function */
	__main_tsc = _tsc_read();

	/* create bootTime fibers */
	task_fiber_start(fiberStack, 512, (nano_fiber_entry_t) bootTimeTask,
			 0, 0, 6, 0);
}
