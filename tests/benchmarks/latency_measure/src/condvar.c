/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file measure time for various condition variable operations
 * 1. Block waiting for a condition variable
 * 2. Signal a condition variable (with context switch)
 */

#include <zephyr/kernel.h>
#include <zephyr/timing/timing.h>
#include "utils.h"
#include "timing_sc.h"

static K_CONDVAR_DEFINE(condvar);
static K_MUTEX_DEFINE(mutex);

static void start_thread_entry(void *p1, void *p2, void *p3)
{
	uint32_t  num_iterations = (uint32_t)(uintptr_t)p1;
	uint32_t  i;
	timing_t  start;
	timing_t  finish;
	uint64_t  sum[2] = {0ull, 0ull};

	k_mutex_lock(&mutex, K_FOREVER);

	k_thread_start(&alt_thread);

	for (i = 0; i < num_iterations; i++) {
		/* 1. Get the first timestamp and block on condvar */

		start = timing_timestamp_get();
		k_condvar_wait(&condvar, &mutex, K_FOREVER);

		/* 3. Get the final timstamp */

		finish = timing_timestamp_get();

		sum[0] += timing_cycles_get(&start, &timestamp.sample);
		sum[1] += timing_cycles_get(&timestamp.sample, &finish);
	}

	/* Wait for alt_thread to finish */

	k_thread_join(&alt_thread, K_FOREVER);

	timestamp.cycles = sum[0];
	k_sem_take(&pause_sem, K_FOREVER);

	timestamp.cycles = sum[1];
}

static void alt_thread_entry(void *p1, void *p2, void *p3)
{
	uint32_t  num_iterations = (uint32_t)(uintptr_t)p1;
	uint32_t  i;

	for (i = 0; i < num_iterations; i++) {

		/* 2. Get midpoint timestamp and signal the condvar */

		timestamp.sample = timing_timestamp_get();
		k_condvar_signal(&condvar);
	}
}

int condvar_blocking_ops(uint32_t num_iterations, uint32_t start_options,
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
			priority - 2, start_options, K_FOREVER);

	k_thread_create(&alt_thread, alt_stack,
			K_THREAD_STACK_SIZEOF(alt_stack),
			alt_thread_entry,
			(void *)(uintptr_t)num_iterations,
			NULL, NULL,
			priority - 1, alt_options, K_FOREVER);

	k_thread_access_grant(&start_thread, &alt_thread,
			      &condvar, &mutex, &pause_sem);
	k_thread_access_grant(&alt_thread, &condvar);

	/* Start test thread */

	k_thread_start(&start_thread);

	/* Stats gathered. Display them. */

	snprintf(tag, sizeof(tag), "condvar.wait.blocking.%c_to_%c",
		 (start_options & K_USER) ? 'u' : 'k',
		 (alt_options & K_USER) ? 'u' : 'k');
	snprintf(description, sizeof(description),
		 "%-40s - Wait for a condvar (context switch)", tag);

	cycles = timestamp.cycles;
	PRINT_STATS_AVG(description, (uint32_t)cycles,
			num_iterations, false, "");

	k_sem_give(&pause_sem);

	snprintf(tag, sizeof(tag), "condvar.signal.wake+ctx.%c_to_%c",
		 (alt_options & K_USER) ? 'u' : 'k',
		 (start_options & K_USER) ? 'u' : 'k');
	snprintf(description, sizeof(description),
		 "%-40s - Signal a condvar (context switch)", tag);
	cycles = timestamp.cycles;
	PRINT_STATS_AVG(description, (uint32_t)cycles,
			num_iterations, false, "");

	k_thread_join(&start_thread, K_FOREVER);

	timing_stop();

	return 0;
}
