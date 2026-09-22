/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file measure time for various event operations
 *
 * This file contains the tests that measure the times for manipulating
 * event objects from both kernel and user threads:
 * 1. Immediately posting and setting events
 * 2. Immediately receiving any or all events.
 * 3. Blocking to receive either any or all events.
 * 4. Waking (and switching to) a thread waiting for any or all events.
 */

#include <zephyr/kernel.h>
#include <zephyr/timing/timing.h>
#include "utils.h"
#include "timing_sc.h"

#define BENCH_EVENT_SET  0x1234
#define ALL_EVENTS       0xFFFFFFFF

static K_EVENT_DEFINE(event_set);

static void event_ops_entry(void *p1, void *p2, void *p3)
{
	uint32_t  num_iterations = (uint32_t)(uintptr_t)p1;
	timing_t  start;
	timing_t  finish;
	uint32_t  i;

	/* 2. Benchmark k_event_post() with no waiters */

	k_event_clear(&event_set, ALL_EVENTS);

	start = timing_timestamp_get();
	for (i = 0; i < num_iterations; i++) {
		k_event_post(&event_set, BENCH_EVENT_SET);
	}
	finish = timing_timestamp_get();

	timestamp.cycles = timing_cycles_get(&start, &finish);

	/* 3. Pause to allow main thread to print results */

	k_sem_take(&pause_sem, K_FOREVER);

	/* 5. Benchmark k_event_set() with no waiters */

	start = timing_timestamp_get();
	for (i = 0; i < num_iterations; i++) {
		k_event_set(&event_set, BENCH_EVENT_SET);
	}
	finish = timing_timestamp_get();

	timestamp.cycles = timing_cycles_get(&start, &finish);

	/* 6. Pause to allow main thread to print results */

	k_sem_take(&pause_sem, K_FOREVER);

	/* 8. Benchmark k_event_wait() (events have already been set) */

	start = timing_timestamp_get();
	for (i = 0; i < num_iterations; i++) {
		k_event_wait(&event_set, BENCH_EVENT_SET, false, K_FOREVER);
	}
	finish = timing_timestamp_get();

	timestamp.cycles = timing_cycles_get(&start, &finish);

	/* 9. Pause to allow main thread to print results */

	k_sem_take(&pause_sem, K_FOREVER);

	/* 11. Benchmark k_event_wait_all() (events have already been set) */

	start = timing_timestamp_get();
	for (i = 0; i < num_iterations; i++) {
		k_event_wait_all(&event_set, BENCH_EVENT_SET, false, K_FOREVER);
	}
	finish = timing_timestamp_get();

	timestamp.cycles = timing_cycles_get(&start, &finish);

	/* 12. Thread finishes */
}

static void start_thread_entry(void *p1, void *p2, void *p3)
{
	uint32_t  num_iterations = (uint32_t)(uintptr_t)p1;
	uint32_t  i;

	k_thread_start(&alt_thread);

	for (i = 0; i < num_iterations; i++) {

		/* 2. Set the events to wake alt_thread */

		timestamp.sample = timing_timestamp_get();
		k_event_set(&event_set, BENCH_EVENT_SET);
	}

	for (i = 0; i < num_iterations; i++) {

		/* 5. Post the events to wake alt_thread */

		timestamp.sample = timing_timestamp_get();
		k_event_post(&event_set, BENCH_EVENT_SET);
	}

	k_thread_join(&alt_thread, K_FOREVER);
}

static void alt_thread_entry(void *p1, void *p2, void *p3)
{
	uint32_t  num_iterations = (uint32_t)(uintptr_t)p1;
	uint32_t  i;
	timing_t  start;
	timing_t  mid;
	timing_t  finish;
	uint64_t  sum[4] = {0ULL, 0ULL, 0ULL, 0ULL};

	for (i = 0; i < num_iterations; i++) {

		/* 1. Wait for any of the events */

		start = timing_timestamp_get();
		k_event_wait(&event_set, BENCH_EVENT_SET, true, K_FOREVER);

		/* 3. Record the final timestamp */

		finish = timing_timestamp_get();
		mid = timestamp.sample;

		sum[0] += timing_cycles_get(&start, &mid);
		sum[1] += timing_cycles_get(&mid, &finish);
	}

	for (i = 0; i < num_iterations; i++) {

		/* 4. Wait for all of the events */

		start = timing_timestamp_get();
		k_event_wait_all(&event_set, BENCH_EVENT_SET, true, K_FOREVER);

		/* 6. Record the final timestamp */

		finish = timing_timestamp_get();
		mid = timestamp.sample;

		sum[2] += timing_cycles_get(&start, &mid);
		sum[3] += timing_cycles_get(&mid, &finish);
	}

	/* Let the main thread print the results */

	timestamp.cycles = sum[0];
	k_sem_take(&pause_sem, K_FOREVER);

	timestamp.cycles = sum[1];
	k_sem_take(&pause_sem, K_FOREVER);

	timestamp.cycles = sum[2];
	k_sem_take(&pause_sem, K_FOREVER);

	timestamp.cycles = sum[3];
}

int event_ops(uint32_t num_iterations, uint32_t options)
{
	int       priority;
	char      tag[50];
	char      description[120];
	uint64_t  cycles;

	priority = k_thread_priority_get(k_current_get());

	timing_start();

	k_thread_create(&start_thread, start_stack,
			K_THREAD_STACK_SIZEOF(start_stack),
			event_ops_entry,
			(void *)(uintptr_t)num_iterations,
			NULL, NULL,
			priority - 1, options, K_FOREVER);

	k_thread_access_grant(&start_thread, &event_set, &pause_sem);

	/* 1. Start test thread */

	k_thread_start(&start_thread);

	/* 4. Benchmark thread has paused */

	snprintf(tag, sizeof(tag), "events.post.immediate.%s",
		 (options & K_USER) ? "user" : "kernel");
	snprintf(description, sizeof(description),
		 "%-40s - Post events (nothing wakes)", tag);

	cycles = timestamp.cycles;
	PRINT_STATS_AVG(description, (uint32_t)cycles,
			num_iterations, false, "");

	k_sem_give(&pause_sem);

	/* 7. Benchmark thread has paused */

	snprintf(tag, sizeof(tag), "events.set.immediate.%s",
		 (options & K_USER) ? "user" : "kernel");
	snprintf(description, sizeof(description),
		 "%-40s - Set events (nothing wakes)", tag);
	cycles = timestamp.cycles;
	PRINT_STATS_AVG(description, (uint32_t)cycles,
			num_iterations, false, "");

	k_sem_give(&pause_sem);

	/* 10. Benchmark thread has paused */

	snprintf(tag, sizeof(tag), "events.wait.immediate.%s",
		 (options & K_USER) ? "user" : "kernel");
	snprintf(description, sizeof(description),
		 "%-40s - Wait for any events (no ctx switch)", tag);
	cycles = timestamp.cycles;
	PRINT_STATS_AVG(description, (uint32_t)cycles,
			num_iterations, false, "");

	k_sem_give(&pause_sem);

	/* 13. Benchmark thread has finished */

	snprintf(tag, sizeof(tag), "events.wait_all.immediate.%s",
		 (options & K_USER) ? "user" : "kernel");
	snprintf(description, sizeof(description),
		 "%-40s - Wait for all events (no ctx switch)", tag);
	cycles = timestamp.cycles;
	PRINT_STATS_AVG(description, (uint32_t)cycles,
			num_iterations, false, "");
	k_thread_join(&start_thread, K_FOREVER);

	timing_stop();

	return 0;
}

int event_blocking_ops(uint32_t num_iterations, uint32_t start_options,
		       uint32_t alt_options)
{
	int       priority;
	char      tag[50];
	char      description[120];
	uint64_t  cycles;

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

	k_thread_access_grant(&start_thread, &alt_thread, &event_set,
			      &pause_sem);
	k_thread_access_grant(&alt_thread, &event_set, &pause_sem);

	k_thread_start(&start_thread);

	snprintf(tag, sizeof(tag),
		 "events.wait.blocking.%c_to_%c",
		 (alt_options & K_USER) ? 'u' : 'k',
		 (start_options & K_USER) ? 'u' : 'k');
	snprintf(description, sizeof(description),
		 "%-40s - Wait for any events (w/ ctx switch)", tag);
	cycles = timestamp.cycles -
		 timestamp_overhead_adjustment(start_options, alt_options);
	PRINT_STATS_AVG(description, (uint32_t)cycles,
			num_iterations, false, "");
	k_sem_give(&pause_sem);

	snprintf(tag, sizeof(tag),
		 "events.set.wake+ctx.%c_to_%c",
		 (start_options & K_USER) ? 'u' : 'k',
		 (alt_options & K_USER) ? 'u' : 'k');
	snprintf(description, sizeof(description),
		 "%-40s - Set events (w/ ctx switch)", tag);
	cycles = timestamp.cycles -
		 timestamp_overhead_adjustment(start_options, alt_options);
	PRINT_STATS_AVG(description, (uint32_t)cycles,
			num_iterations, false, "");
	k_sem_give(&pause_sem);

	snprintf(tag, sizeof(tag),
		 "events.wait_all.blocking.%c_to_%c",
		 (alt_options & K_USER) ? 'u' : 'k',
		 (start_options & K_USER) ? 'u' : 'k');
	snprintf(description, sizeof(description),
		 "%-40s - Wait for all events (w/ ctx switch)", tag);
	cycles = timestamp.cycles;
	PRINT_STATS_AVG(description, (uint32_t)cycles,
			num_iterations, false, "");
	k_sem_give(&pause_sem);

	snprintf(tag, sizeof(tag),
		 "events.post.wake+ctx.%c_to_%c",
		 (start_options & K_USER) ? 'u' : 'k',
		 (alt_options & K_USER) ? 'u' : 'k');
	snprintf(description, sizeof(description),
		 "%-40s - Post events (w/ ctx switch)", tag);
	cycles = timestamp.cycles;
	PRINT_STATS_AVG(description, (uint32_t)cycles,
			num_iterations, false, "");

	k_thread_join(&start_thread, K_FOREVER);

	timing_stop();

	return 0;
}
