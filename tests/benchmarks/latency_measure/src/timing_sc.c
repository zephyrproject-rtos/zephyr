/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file
 * This file contains routines for implementing a timestamp system call
 * as well as routines for determining the overhead associated with it.
 */

#include <zephyr/kernel.h>
#include "utils.h"
#include "timing_sc.h"

BENCH_BMEM uint64_t timestamp_overhead;
#ifdef CONFIG_USERSPACE
BENCH_BMEM uint64_t user_timestamp_overhead;
#endif

timing_t z_impl_timing_timestamp_get(void)
{
	return timing_counter_get();
}

#ifdef CONFIG_USERSPACE
timing_t z_vrfy_timing_timestamp_get(void)
{
	return z_impl_timing_timestamp_get();
}
#include <syscalls/timing_timestamp_get_mrsh.c>
#endif

static void start_thread_entry(void *p1, void *p2, void *p3)
{
	uint32_t  num_iterations = (uint32_t)(uintptr_t)p1;
	timing_t  start;
	timing_t  finish;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	start = timing_timestamp_get();
	for (uint32_t i = 0; i < num_iterations; i++) {
		timing_timestamp_get();
	}
	finish = timing_timestamp_get();

	timestamp.cycles = timing_cycles_get(&start, &finish);
}

void timestamp_overhead_init(uint32_t num_iterations)
{
	int  priority;

	priority = k_thread_priority_get(k_current_get());

	k_thread_create(&start_thread, start_stack,
			K_THREAD_STACK_SIZEOF(start_stack),
			start_thread_entry,
			(void *)(uintptr_t)num_iterations, NULL, NULL,
			priority - 1, 0, K_FOREVER);

	k_thread_start(&start_thread);

	k_thread_join(&start_thread, K_FOREVER);

	timestamp_overhead = timestamp.cycles;

#ifdef CONFIG_USERSPACE
	k_thread_create(&start_thread, start_stack,
			K_THREAD_STACK_SIZEOF(start_stack),
			start_thread_entry,
			(void *)(uintptr_t)num_iterations, NULL, NULL,
			priority - 1, K_USER, K_FOREVER);

	k_thread_start(&start_thread);

	k_thread_join(&start_thread, K_FOREVER);

	user_timestamp_overhead = timestamp.cycles;
#endif
}

uint64_t timestamp_overhead_adjustment(uint32_t options1, uint32_t options2)
{
#ifdef CONFIG_USERSPACE
	if (((options1 | options2) & K_USER) == K_USER) {
		if (((options1 & options2) & K_USER) == K_USER) {
			/*
			 * Both start and finish timestamps were obtained
			 * from userspace.
			 */
			return user_timestamp_overhead;
		}
		/*
		 * One timestamp came from userspace, and the other came
		 * from kernel space. Estimate the overhead as the mean
		 * between the two.
		 */
		return (timestamp_overhead + user_timestamp_overhead) / 2;
	}
#endif

	/*
	 * Both start and finish timestamps were obtained
	 * from kernel space.
	 */
	return timestamp_overhead;
}
