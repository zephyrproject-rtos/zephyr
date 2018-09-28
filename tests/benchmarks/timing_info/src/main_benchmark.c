/*
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Measure time
 *
 */
#include <kernel.h>
#include <zephyr.h>
#include <tc_util.h>
#include <ksched.h>
#include "timing_info.h"

void main(void)
{
	u32_t freq = get_core_freq_MHz();

	/* Configure and start timer */
	benchmark_timer_init();
	benchmark_timer_start();

	TC_START("Time Measurement");
	TC_PRINT("Timing Results: Clock Frequency: %d MHz\n", freq);

	/*******************************************************************/
	/* System parameters and thread Benchmarking*/
	system_thread_bench();

	/*******************************************************************/
	/* Thread yield*/
	yield_bench();

	/*******************************************************************/
	/* heap Memory benchmarking*/
	heap_malloc_free_bench();

	/*******************************************************************/
	/* Semaphore take and get*/
	semaphore_bench();

	/*******************************************************************/
	/* mutex lock and unlock*/
	mutex_bench();

	/*******************************************************************/
	/* mutex lock and unlock*/
	msg_passing_bench();

	/*******************************************************************/
#ifdef CONFIG_USERSPACE
	/* userspace related benchmarks */
	userspace_bench();
#endif


	TC_PRINT("Timing Measurement  finished\n");

	/* for sanity regression test utility. */
	TC_END_RESULT(TC_PASS);
	TC_END_REPORT(TC_PASS);
	benchmark_timer_stop();
}
