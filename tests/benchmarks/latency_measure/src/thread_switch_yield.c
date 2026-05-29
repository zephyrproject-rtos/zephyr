/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * This file contains the benchmarking code that measures the average time it
 * takes to perform context switches between threads using k_yield().
 *
 * When user threads are supported, there are four cases to consider. These are
 *   1. Kernel thread -> Kernel thread
 *   2. User thread   -> User thread
 *   3. Kernel thread -> User thread
 *   4. User thread   -> Kernel thread
 */

#include <zephyr/kernel.h>
#include <zephyr/timing/timing.h>
#include <stdlib.h>
#include <zephyr/timestamp.h>

#include "utils.h"
#include "timing_sc.h"

static void alt_thread_entry(void *p1, void *p2, void *p3)
{
	uint32_t  num_iterations;

	ARG_UNUSED(p2);
	ARG_UNUSED(p2);

	num_iterations = (uint32_t)(uintptr_t)p1;

	for (uint32_t i = 0; i < num_iterations; i++) {

		/* 3. Obtain the 'finish' timestamp */

		timestamp.sample = timing_timestamp_get();

		/* 4. Switch to <start_thread>  */

		k_yield();
	}

}

static void start_thread_entry(void *p1, void *p2, void *p3)
{
	uint64_t  sum = 0ull;
	uint32_t  num_iterations;
	timing_t  start;
	timing_t  finish;

	ARG_UNUSED(p2);
	ARG_UNUSED(p2);

	num_iterations = (uint32_t)(uintptr_t)p1;

	k_thread_start(&alt_thread);

	for (uint32_t i = 0; i < num_iterations; i++) {

		/* 1. Get 'start' timestamp */

		start = timing_timestamp_get();

		/* 2. Switch to <alt_thread> */

		k_yield();

		/* 5. Get the 'finish' timestamp obtained in <alt_thread> */

		finish = timestamp.sample;

		/* 6. Track the sum of elapsed times */

		sum += timing_cycles_get(&start, &finish);
	}

	/* Wait for <alt_thread> to complete */

	k_thread_join(&alt_thread, K_FOREVER);

	/* Record the number of cycles for use by the main thread */

	timestamp.cycles = sum;
}

static void thread_switch_yield_common(const char *description,
				       uint32_t num_iterations,
				       uint32_t start_options,
				       uint32_t alt_options,
				       int priority)
{
	uint64_t  sum;
	char tag[50];
	char summary[120];

	/* Create the two threads */

	k_thread_create(&start_thread, start_stack,
			K_THREAD_STACK_SIZEOF(start_stack),
			start_thread_entry,
			(void *)(uintptr_t)num_iterations, NULL, NULL,
			priority - 1, start_options, K_FOREVER);

	k_thread_create(&alt_thread, alt_stack,
			K_THREAD_STACK_SIZEOF(alt_stack),
			alt_thread_entry,
			(void *)(uintptr_t)num_iterations, NULL, NULL,
			priority - 1, alt_options, K_FOREVER);

	/* Grant access rights if necessary */

	if ((start_options & K_USER) == K_USER) {
		k_thread_access_grant(&start_thread, &alt_thread);
	}

	k_thread_start(&start_thread);

	/* Wait until <start_thread> finishes */

	k_thread_join(&start_thread, K_FOREVER);

	/* Get the sum total of measured cycles */

	sum = timestamp.cycles;

	sum -= timestamp_overhead_adjustment(start_options, alt_options);

	snprintf(tag, sizeof(tag),
		 "%s.%c_to_%c", description,
		 (start_options & K_USER) == K_USER ? 'u' : 'k',
		 (alt_options & K_USER) == K_USER ? 'u' : 'k');
	snprintf(summary, sizeof(summary),
		 "%-40s - Context switch via k_yield", tag);

	PRINT_STATS_AVG(summary, (uint32_t)sum, num_iterations, 0, "");
}

void thread_switch_yield(uint32_t num_iterations, bool is_cooperative)
{
	int  priority;
	char description[40];

	priority = is_cooperative ? K_PRIO_COOP(6)
				  : k_thread_priority_get(k_current_get()) - 1;

	snprintf(description, sizeof(description),
		 "thread.yield.%s.ctx",
		 is_cooperative ? "cooperative" : "preemptive");

	/* Kernel -> Kernel */
	thread_switch_yield_common(description, num_iterations, 0, 0,
				   priority);

#if CONFIG_USERSPACE
	/* User   -> User   */
	thread_switch_yield_common(description, num_iterations, K_USER, K_USER,
				   priority);

	/* Kernel -> User   */
	thread_switch_yield_common(description, num_iterations, 0, K_USER,
				   priority);

	/* User   -> Kernel */
	thread_switch_yield_common(description, num_iterations, K_USER, 0,
				   priority);
#endif
}
