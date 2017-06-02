/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * This file contains the benchmark that measure the average time it takes to
 * do context switches between threads using k_yield () to force
 * context switch.
 */

#include <zephyr.h>
#include <timestamp.h>  /* reading time */
#include "utils.h"      /* PRINT () and other macros */

/* <stdlib.h> is not supported */
static int abs(int i)
{
	return (i >= 0) ? i : -i;
}

/* context switch enough time so our measurement is precise */
#define NB_OF_YIELD     1000

static u32_t helper_thread_iterations;

#define Y_STACK_SIZE    512
#define Y_PRIORITY      10

K_THREAD_STACK_DEFINE(y_stack_area, Y_STACK_SIZE);
static struct k_thread y_thread;

/**
 *
 * @brief Helper thread for measuring thread switeh latency using yield
 *
 * @return N/A
 */
void yielding_thread(void *arg1, void *arg2, void *arg3)
{
	while (helper_thread_iterations < NB_OF_YIELD) {
		k_yield();
		helper_thread_iterations++;
	}
}

/**
 *
 * @brief Entry point for thread context switch using yield test
 *
 * @return N/A
 */
void thread_switch_yield(void)
{
	u32_t iterations = 0;
	s32_t delta;
	u32_t timestamp;

	PRINT_FORMAT(" 5 - Measure average context switch time between threads"
		     " using (k_yield)");

	bench_test_start();

	/* launch helper thread of the same priority than this routine */
	k_thread_create(&y_thread, y_stack_area, Y_STACK_SIZE,
			yielding_thread, NULL, NULL, NULL,
			Y_PRIORITY, 0, K_NO_WAIT);

	/* get initial timestamp */
	timestamp = TIME_STAMP_DELTA_GET(0);

	/* loop until either helper or this routine reaches number of yields */
	while (iterations < NB_OF_YIELD &&
	       helper_thread_iterations < NB_OF_YIELD) {
		k_yield();
		iterations++;
	}

	/* get the number of cycles it took to do the test */
	timestamp = TIME_STAMP_DELTA_GET(timestamp);

	/* Ensure both helper and this routine were context switching back &
	 * forth.
	 * For execution to reach the line below, either this routine or helper
	 * routine reached NB_OF_YIELD. The other loop must be at most one
	 * iteration away from reaching NB_OF_YIELD if execute was switching
	 * back and forth.
	 */
	delta = iterations - helper_thread_iterations;
	if (bench_test_end() < 0) {
		error_count++;
		PRINT_OVERFLOW_ERROR();
	} else if (abs(delta) > 1) {
		/* expecting even alternating context switch, seems one routine
		 * called yield without the other having chance to execute
		 */
		error_count++;
		PRINT_FORMAT(" Error, iteration:%u, helper iteration:%u",
			     iterations, helper_thread_iterations);
	} else {
		/* thread_yield is called (iterations + helper_thread_iterations)
		 * times in total.
		 */
		PRINT_FORMAT(" Average thread context switch using "
			     "yield %u tcs = %u nsec",
			     timestamp / (iterations + helper_thread_iterations),
			     SYS_CLOCK_HW_CYCLES_TO_NS_AVG(timestamp,
							   (iterations + helper_thread_iterations)));
	}
}
