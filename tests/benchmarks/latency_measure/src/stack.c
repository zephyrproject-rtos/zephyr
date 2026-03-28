/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file measure time for various k_stack operations
 *
 * This file contains the tests that measures the times for the following
 * k_stack operations from both kernel threads and user threads:
 *  1. Immediately adding a data item to a k_stack
 *  2. Immediately removing a data item from a k_stack
 *  3. Blocking on removing a data item from a k_stack
 *  4. Waking (and context switching to) a thread blocked on a k_stack
 */

#include <zephyr/kernel.h>
#include <zephyr/timing/timing.h>
#include "utils.h"
#include "timing_sc.h"

#define MAX_ITEMS  16

static BENCH_BMEM stack_data_t stack_array[MAX_ITEMS];

static struct k_stack stack;

static void stack_push_pop_thread_entry(void *p1, void *p2, void *p3)
{
	uint32_t num_iterations = (uint32_t)(uintptr_t)p1;
	timing_t start;
	timing_t mid;
	timing_t finish;
	uint64_t put_sum = 0ULL;
	uint64_t get_sum = 0ULL;
	stack_data_t  data;

	for (uint32_t i = 0; i < num_iterations; i++) {
		start = timing_timestamp_get();

		(void) k_stack_push(&stack, 1234);

		mid = timing_timestamp_get();

		(void) k_stack_pop(&stack, &data, K_NO_WAIT);

		finish = timing_timestamp_get();

		put_sum += timing_cycles_get(&start, &mid);
		get_sum += timing_cycles_get(&mid, &finish);
	}

	timestamp.cycles = put_sum;
	k_sem_take(&pause_sem, K_FOREVER);

	timestamp.cycles = get_sum;
}

int stack_ops(uint32_t num_iterations, uint32_t options)
{
	int      priority;
	uint64_t cycles;
	char     tag[50];
	char     description[120];

	priority = k_thread_priority_get(k_current_get());

	timing_start();

	k_stack_init(&stack, stack_array, MAX_ITEMS);

	k_thread_create(&start_thread, start_stack,
			K_THREAD_STACK_SIZEOF(start_stack),
			stack_push_pop_thread_entry,
			(void *)(uintptr_t)num_iterations,
			NULL, NULL,
			priority - 1, options, K_FOREVER);

	k_thread_access_grant(&start_thread, &pause_sem, &stack);

	k_thread_start(&start_thread);

	snprintf(tag, sizeof(tag),
		 "stack.push.immediate.%s",
		 options & K_USER ? "user" : "kernel");
	snprintf(description, sizeof(description),
		 "%-40s - Add data to k_stack (no ctx switch)", tag);

	cycles = timestamp.cycles;
	cycles -= timestamp_overhead_adjustment(options, options);
	PRINT_STATS_AVG(description, (uint32_t)cycles,
			num_iterations, false, "");
	k_sem_give(&pause_sem);

	snprintf(tag, sizeof(tag),
		 "stack.pop.immediate.%s",
		 options & K_USER ? "user" : "kernel");
	snprintf(description, sizeof(description),
		 "%-40s - Get data from k_stack (no ctx switch)", tag);
	cycles = timestamp.cycles;
	cycles -= timestamp_overhead_adjustment(options, options);
	PRINT_STATS_AVG(description, (uint32_t)cycles,
			num_iterations, false, "");

	k_thread_join(&start_thread, K_FOREVER);

	timing_stop();

	return 0;
}

static void alt_thread_entry(void *p1, void *p2, void *p3)
{
	uint32_t num_iterations = (uint32_t)(uintptr_t)p1;
	timing_t  start;
	timing_t  mid;
	timing_t  finish;
	uint64_t  sum[2] = {0ULL, 0ULL};
	uint32_t  i;
	stack_data_t data;

	for (i = 0; i < num_iterations; i++) {

		/* 1. Block waiting for data on k_stack */

		start = timing_timestamp_get();

		k_stack_pop(&stack, &data, K_FOREVER);

		/* 3. Data obtained. */

		finish = timing_timestamp_get();

		mid = timestamp.sample;

		sum[0] += timing_cycles_get(&start, &mid);
		sum[1] += timing_cycles_get(&mid, &finish);
	}

	timestamp.cycles = sum[0];
	k_sem_take(&pause_sem, K_FOREVER);
	timestamp.cycles = sum[1];
}

static void start_thread_entry(void *p1, void *p2, void *p3)
{
	uint32_t num_iterations = (uint32_t)(uintptr_t)p1;
	uint32_t i;

	k_thread_start(&alt_thread);

	for (i = 0; i < num_iterations; i++) {

		/* 2. Add data thereby waking alt thread */

		timestamp.sample = timing_timestamp_get();

		k_stack_push(&stack, (stack_data_t)123);
	}

	k_thread_join(&alt_thread, K_FOREVER);
}

int stack_blocking_ops(uint32_t num_iterations, uint32_t start_options,
		      uint32_t alt_options)
{
	int      priority;
	uint64_t cycles;
	char     tag[50];
	char     description[120];

	priority = k_thread_priority_get(k_current_get());

	timing_start();

	k_thread_create(&start_thread, start_stack,
			K_THREAD_STACK_SIZEOF(start_stack),
			start_thread_entry,
			(void *)(uintptr_t)num_iterations,
			NULL, NULL,
			priority - 1, start_options, K_FOREVER);

	k_thread_create(&alt_thread, alt_stack,
			K_THREAD_STACK_SIZEOF(alt_stack),
			alt_thread_entry,
			(void *)(uintptr_t)num_iterations,
			NULL, NULL,
			priority - 2, alt_options, K_FOREVER);

	k_thread_access_grant(&start_thread, &alt_thread, &pause_sem, &stack);
	k_thread_access_grant(&alt_thread, &pause_sem, &stack);

	k_thread_start(&start_thread);

	snprintf(tag, sizeof(tag),
		 "stack.pop.blocking.%s_to_%s",
		 alt_options & K_USER ? "u" : "k",
		 start_options & K_USER ? "u" : "k");
	snprintf(description, sizeof(description),
		 "%-40s - Get data from k_stack (w/ ctx switch)", tag);

	cycles = timestamp.cycles;
	PRINT_STATS_AVG(description, (uint32_t)cycles,
			num_iterations, false, "");
	k_sem_give(&pause_sem);

	snprintf(tag, sizeof(tag),
		 "stack.push.wake+ctx.%s_to_%s",
		 start_options & K_USER ? "u" : "k",
		 alt_options & K_USER ? "u" : "k");
	snprintf(description, sizeof(description),
		 "%-40s - Add data to k_stack (w/ ctx switch)", tag);
	cycles = timestamp.cycles;
	PRINT_STATS_AVG(description, (uint32_t)cycles,
			num_iterations, false, "");

	k_thread_join(&start_thread, K_FOREVER);

	timing_stop();

	return 0;
}
