/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file measure time for various LIFO operations
 *
 * This file contains the tests that measures the times for the following
 * LIFO operations from both kernel threads and user threads:
 *  1. Immediately adding a data item to a LIFO
 *  2. Immediately removing a data item from a LIFO
 *  3. Immediately adding a data item to a LIFO with allocation
 *  4. Immediately removing a data item from a LIFO with allocation
 *  5. Blocking on removing a data item from a LIFO
 *  6. Waking (and context switching to) a thread blocked on a LIFO via
 *     k_lifo_put().
 *  7. Waking (and context switching to) a thread blocked on a LIFO via
 *     k_lifo_alloc_put().
 */

#include <zephyr/kernel.h>
#include <zephyr/timing/timing.h>
#include "utils.h"
#include "timing_sc.h"

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)

static K_LIFO_DEFINE(lifo);

BENCH_BMEM uintptr_t lifo_data[5];

static void lifo_put_get_thread_entry(void *p1, void *p2, void *p3)
{
	uint32_t num_iterations = (uint32_t)(uintptr_t)p1;
	uint32_t options = (uint32_t)(uintptr_t)p2;
	timing_t start;
	timing_t mid;
	timing_t finish;
	uint64_t put_sum = 0ULL;
	uint64_t get_sum = 0ULL;
	uintptr_t *data;

	if ((options & K_USER) == 0) {
		for (uint32_t i = 0; i < num_iterations; i++) {
			start = timing_timestamp_get();

			k_lifo_put(&lifo, lifo_data);

			mid = timing_timestamp_get();

			data = k_lifo_get(&lifo, K_NO_WAIT);

			finish = timing_timestamp_get();

			put_sum += timing_cycles_get(&start, &mid);
			get_sum += timing_cycles_get(&mid, &finish);
		}

		timestamp.cycles = put_sum;
		k_sem_take(&pause_sem, K_FOREVER);

		timestamp.cycles = get_sum;
		k_sem_take(&pause_sem, K_FOREVER);

		put_sum = 0ULL;
		get_sum = 0ULL;
	}

	for (uint32_t i = 0; i < num_iterations; i++) {
		start = timing_timestamp_get();

		k_lifo_alloc_put(&lifo, lifo_data);

		mid = timing_timestamp_get();

		data = k_lifo_get(&lifo, K_NO_WAIT);

		finish = timing_timestamp_get();

		put_sum += timing_cycles_get(&start, &mid);
		get_sum += timing_cycles_get(&mid, &finish);
	}

	timestamp.cycles = put_sum;
	k_sem_take(&pause_sem, K_FOREVER);

	timestamp.cycles = get_sum;
}

int lifo_ops(uint32_t num_iterations, uint32_t options)
{
	int      priority;
	uint64_t cycles;
	char     tag[50];
	char     description[120];

	priority = k_thread_priority_get(k_current_get());

	timing_start();

	k_thread_create(&start_thread, start_stack,
			K_THREAD_STACK_SIZEOF(start_stack),
			lifo_put_get_thread_entry,
			(void *)(uintptr_t)num_iterations,
			(void *)(uintptr_t)options, NULL,
			priority - 1, options, K_FOREVER);

	k_thread_access_grant(&start_thread, &pause_sem, &lifo);

	k_thread_start(&start_thread);

	if ((options & K_USER) == 0) {
		snprintf(tag, sizeof(tag),
			 "lifo.put.immediate.%s",
			 options & K_USER ? "user" : "kernel");
		snprintf(description, sizeof(description),
			 "%-40s - Add data to LIFO (no ctx switch)", tag);

		cycles = timestamp.cycles;
		cycles -= timestamp_overhead_adjustment(options, options);
		PRINT_STATS_AVG(description, (uint32_t)cycles,
				num_iterations, false, "");
		k_sem_give(&pause_sem);

		snprintf(tag, sizeof(tag),
			 "lifo.get.immediate.%s",
			 options & K_USER ? "user" : "kernel");
		snprintf(description, sizeof(description),
			 "%-40s - Get data from LIFO (no ctx switch)", tag);
		cycles = timestamp.cycles;
		cycles -= timestamp_overhead_adjustment(options, options);
		PRINT_STATS_AVG(description, (uint32_t)cycles,
				num_iterations, false, "");
		k_sem_give(&pause_sem);
	}

	snprintf(tag, sizeof(tag),
		 "lifo.put.alloc.immediate.%s",
		 options & K_USER ? "user" : "kernel");
	snprintf(description, sizeof(description),
		 "%-40s - Allocate to add data to LIFO (no ctx switch)", tag);

	cycles = timestamp.cycles;
	PRINT_STATS_AVG(description, (uint32_t)cycles,
			num_iterations, false, "");
	k_sem_give(&pause_sem);

	snprintf(tag, sizeof(tag),
		 "lifo.get.free.immediate.%s",
		 options & K_USER ? "user" : "kernel");
	snprintf(description, sizeof(description),
		 "%-40s - Free when getting data from LIFO (no ctx switch)", tag);
	cycles = timestamp.cycles;
	PRINT_STATS_AVG(description, (uint32_t)cycles,
			num_iterations, false, "");

	k_thread_join(&start_thread, K_FOREVER);

	timing_stop();

	return 0;
}

static void alt_thread_entry(void *p1, void *p2, void *p3)
{
	uint32_t num_iterations = (uint32_t)(uintptr_t)p1;
	uint32_t options = (uint32_t)(uintptr_t)p2;
	timing_t  start;
	timing_t  mid;
	timing_t  finish;
	uint64_t  sum[4] = {0ULL, 0ULL, 0ULL, 0ULL};
	uintptr_t *data;
	uint32_t  i;

	if ((options & K_USER) == 0) {

		/* Used with k_lifo_put() */

		for (i = 0; i < num_iterations; i++) {

			/* 1. Block waiting for data on LIFO */

			start = timing_timestamp_get();

			data = k_lifo_get(&lifo, K_FOREVER);

			/* 3. Data obtained. */

			finish = timing_timestamp_get();

			mid = timestamp.sample;

			sum[0] += timing_cycles_get(&start, &mid);
			sum[1] += timing_cycles_get(&mid, &finish);
		}
	}

	/* Used with k_lifo_alloc_put() */

	for (i = 0; i < num_iterations; i++) {

		/* 4. Block waiting for data on LIFO */

		start = timing_timestamp_get();

		data = k_lifo_get(&lifo, K_FOREVER);

		/* 6. Data obtained */

		finish = timing_timestamp_get();

		mid = timestamp.sample;

		sum[2] += timing_cycles_get(&start, &mid);
		sum[3] += timing_cycles_get(&mid, &finish);
	}

	if ((options & K_USER) == 0) {
		timestamp.cycles = sum[0];
		k_sem_take(&pause_sem, K_FOREVER);
		timestamp.cycles = sum[1];
		k_sem_take(&pause_sem, K_FOREVER);
	}

	timestamp.cycles = sum[2];
	k_sem_take(&pause_sem, K_FOREVER);
	timestamp.cycles = sum[3];
}

static void start_thread_entry(void *p1, void *p2, void *p3)
{
	uint32_t num_iterations = (uint32_t)(uintptr_t)p1;
	uint32_t options = (uint32_t)(uintptr_t)p2;
	uint32_t i;

	k_thread_start(&alt_thread);

	if ((options & K_USER) == 0) {
		for (i = 0; i < num_iterations; i++) {

			/* 2. Add data thereby waking alt thread */

			timestamp.sample = timing_timestamp_get();

			k_lifo_put(&lifo, lifo_data);

		}
	}

	for (i = 0; i < num_iterations; i++) {

		/* 5. Add data thereby waking alt thread */

		timestamp.sample = timing_timestamp_get();

		k_lifo_alloc_put(&lifo, lifo_data);

	}

	k_thread_join(&alt_thread, K_FOREVER);
}

int lifo_blocking_ops(uint32_t num_iterations, uint32_t start_options,
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
			(void *)(uintptr_t)(start_options | alt_options), NULL,
			priority - 1, start_options, K_FOREVER);

	k_thread_create(&alt_thread, alt_stack,
			K_THREAD_STACK_SIZEOF(alt_stack),
			alt_thread_entry,
			(void *)(uintptr_t)num_iterations,
			(void *)(uintptr_t)(start_options | alt_options), NULL,
			priority - 2, alt_options, K_FOREVER);

	k_thread_access_grant(&start_thread, &alt_thread, &pause_sem, &lifo);
	k_thread_access_grant(&alt_thread, &pause_sem, &lifo);

	k_thread_start(&start_thread);

	if (((start_options | alt_options) & K_USER) == 0) {
		snprintf(tag, sizeof(tag),
			 "lifo.get.blocking.%s_to_%s",
			 alt_options & K_USER ? "u" : "k",
			 start_options & K_USER ? "u" : "k");
		snprintf(description, sizeof(description),
			 "%-40s - Get data from LIFO (w/ ctx switch)", tag);

		cycles = timestamp.cycles;
		PRINT_STATS_AVG(description, (uint32_t)cycles,
				num_iterations, false, "");
		k_sem_give(&pause_sem);

		snprintf(tag, sizeof(tag),
			 "lifo.put.wake+ctx.%s_to_%s",
			 start_options & K_USER ? "u" : "k",
			 alt_options & K_USER ? "u" : "k");
		snprintf(description, sizeof(description),
			 "%-40s - Add data to LIFO (w/ ctx switch)", tag);
		cycles = timestamp.cycles;
		PRINT_STATS_AVG(description, (uint32_t)cycles,
				num_iterations, false, "");
		k_sem_give(&pause_sem);
	}

	snprintf(tag, sizeof(tag),
		 "lifo.get.free.blocking.%s_to_%s",
		 alt_options & K_USER ? "u" : "k",
		 start_options & K_USER ? "u" : "k");
	snprintf(description, sizeof(description),
		 "%-40s - Free when getting data from LIFO (w/ ctx switch)", tag);

	cycles = timestamp.cycles;
	PRINT_STATS_AVG(description, (uint32_t)cycles,
			num_iterations, false, "");
	k_sem_give(&pause_sem);

	snprintf(tag, sizeof(tag),
		 "lifo.put.alloc.wake+ctx.%s_to_%s",
		 start_options & K_USER ? "u" : "k",
		 alt_options & K_USER ? "u" : "k");
	snprintf(description, sizeof(description),
		 "%-40s - Allocate to add data to LIFO (w/ ctx switch)", tag);
	cycles = timestamp.cycles;
	PRINT_STATS_AVG(description, (uint32_t)cycles,
			num_iterations, false, "");

	k_thread_join(&start_thread, K_FOREVER);

	timing_stop();

	return 0;
}
