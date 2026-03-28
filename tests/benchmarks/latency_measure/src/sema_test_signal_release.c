/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file measure time for sema lock and release
 *
 * This file contains the test that measures semaphore give and take time
 * in the kernel. There is no contention on the semaphore being tested.
 */

#include <zephyr/kernel.h>
#include <zephyr/timing/timing.h>
#include "utils.h"
#include "timing_sc.h"

static struct k_sem  sem;

static void alt_thread_entry(void *p1, void *p2, void *p3)
{
	uint32_t   num_iterations = (uint32_t)(uintptr_t)p1;
	timing_t   mid;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	for (uint32_t i = 0; i < num_iterations; i++) {

		/*
		 * 2. Give the semaphore, thereby forcing a context switch back
		 * to <start_thread>.
		 */

		mid = timing_timestamp_get();
		k_sem_give(&sem);

		/* 5. Share the <mid> timestamp. */

		timestamp.sample = mid;

		/* 6. Give <sem> so <start_thread> resumes execution */

		k_sem_give(&sem);
	}
}

static void start_thread_entry(void *p1, void *p2, void *p3)
{
	uint32_t   num_iterations = (uint32_t)(uintptr_t)p1;
	timing_t   start;
	timing_t   mid;
	timing_t   finish;
	uint32_t   i;
	uint64_t   take_sum = 0ull;
	uint64_t   give_sum = 0ull;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_thread_start(&alt_thread);

	for (i = 0; i < num_iterations; i++) {

		/*
		 * 1. Block on taking the semaphore and force a context switch
		 * to <alt_thread>.
		 */

		start = timing_timestamp_get();
		k_sem_take(&sem, K_FOREVER);

		/* 3. Get the <finish> timestamp. */

		finish = timing_timestamp_get();

		/*
		 * 4. Let <alt_thread> run so it can share its <mid>
		 * timestamp.
		 */

		k_sem_take(&sem, K_FOREVER);

		/* 7. Retrieve the <mid> timestamp */

		mid = timestamp.sample;

		take_sum += timing_cycles_get(&start, &mid);
		give_sum += timing_cycles_get(&mid, &finish);
	}

	k_thread_join(&alt_thread, K_FOREVER);

	/* Share the totals with the main thread */

	timestamp.cycles = take_sum;

	k_sem_take(&sem, K_FOREVER);

	timestamp.cycles = give_sum;
}

void sema_context_switch(uint32_t num_iterations,
			 uint32_t start_options, uint32_t alt_options)
{
	uint64_t  cycles;
	char tag[50];
	char description[120];
	int  priority;

	timing_start();

	priority = k_thread_priority_get(k_current_get());

	k_thread_create(&start_thread, start_stack,
			K_THREAD_STACK_SIZEOF(start_stack),
			start_thread_entry,
			(void *)(uintptr_t)num_iterations, NULL, NULL,
			priority - 2, start_options, K_FOREVER);

	k_thread_create(&alt_thread, alt_stack,
			K_THREAD_STACK_SIZEOF(alt_stack),
			alt_thread_entry,
			(void *)(uintptr_t)num_iterations, NULL, NULL,
			priority - 1, alt_options, K_FOREVER);

	k_thread_access_grant(&start_thread, &sem, &alt_thread);

	k_thread_access_grant(&alt_thread, &sem);

	/* Start the test threads */

	k_thread_start(&start_thread);

	/* Retrieve the number of cycles spent taking the semaphore */

	cycles = timestamp.cycles;
	cycles -= timestamp_overhead_adjustment(start_options, alt_options);

	snprintf(tag, sizeof(tag),
		 "semaphore.take.blocking.%c_to_%c",
		 ((start_options & K_USER) == K_USER) ? 'u' : 'k',
		 ((alt_options & K_USER) == K_USER) ? 'u' : 'k');
	snprintf(description, sizeof(description),
		 "%-40s - Take a semaphore (context switch)", tag);
	PRINT_STATS_AVG(description, (uint32_t)cycles,
			num_iterations, false, "");

	/* Unblock <start_thread> */

	k_sem_give(&sem);

	/* Retrieve the number of cycles spent taking the semaphore */

	cycles = timestamp.cycles;
	cycles -= timestamp_overhead_adjustment(start_options, alt_options);

	snprintf(tag, sizeof(tag),
		 "semaphore.give.wake+ctx.%c_to_%c",
		 ((alt_options & K_USER) == K_USER) ? 'u' : 'k',
		 ((start_options & K_USER) == K_USER) ? 'u' : 'k');
	snprintf(description, sizeof(description),
		 "%-40s - Give a semaphore (context switch)", tag);
	PRINT_STATS_AVG(description, (uint32_t)cycles,
			num_iterations, false, "");

	k_thread_join(&start_thread, K_FOREVER);

	timing_stop();

	return;
}

/**
 * This is the entry point for the test that performs uncontested operations
 * on the semaphore. It gives the semaphore many times, takes the semaphore
 * many times and then sends the results back to the main thread.
 */
static void immediate_give_take(void *p1, void *p2, void *p3)
{
	uint32_t   num_iterations = (uint32_t)(uintptr_t)p1;
	timing_t   start;
	timing_t   finish;
	uint64_t   give_cycles;
	uint64_t   take_cycles;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* 1. Give a semaphore. No threads are waiting on it */

	start = timing_timestamp_get();

	for (uint32_t i = 0; i < num_iterations; i++) {
		k_sem_give(&sem);
	}

	finish = timing_timestamp_get();
	give_cycles = timing_cycles_get(&start, &finish);

	/* 2. Take a semaphore--no contention */

	start = timing_timestamp_get();

	for (uint32_t i = 0; i < num_iterations; i++) {
		k_sem_take(&sem, K_NO_WAIT);
	}

	finish = timing_timestamp_get();
	take_cycles = timing_cycles_get(&start, &finish);

	/* 3. Post the number of cycles spent giving the semaphore */

	timestamp.cycles = give_cycles;

	/* 4. Wait for the main thread to retrieve the data */

	k_sem_take(&sem, K_FOREVER);

	/* 7. Post the number of cycles spent taking the semaphore */

	timestamp.cycles = take_cycles;
}


/**
 *
 * @brief The function tests semaphore test/signal time
 *
 * The routine performs unlock the quite amount of semaphores and then
 * acquires them in order to measure the necessary time.
 *
 * @return 0 on success
 */
int sema_test_signal(uint32_t num_iterations, uint32_t options)
{
	uint64_t cycles;
	int priority;
	char tag[50];
	char description[120];

	timing_start();

	k_sem_init(&sem, 0, num_iterations);

	priority = k_thread_priority_get(k_current_get());

	k_thread_create(&start_thread, start_stack,
			K_THREAD_STACK_SIZEOF(start_stack),
			immediate_give_take,
			(void *)(uintptr_t)num_iterations, NULL, NULL,
			priority - 1, options, K_FOREVER);

	k_thread_access_grant(&start_thread, &sem);
	k_thread_start(&start_thread);

	/* 5. Retrieve the number of cycles spent giving the semaphore */

	cycles = timestamp.cycles;

	snprintf(tag, sizeof(tag),
		 "semaphore.give.immediate.%s",
		 (options & K_USER) == K_USER ? "user" : "kernel");
	snprintf(description, sizeof(description),
		 "%-40s - Give a semaphore (no waiters)", tag);

	PRINT_STATS_AVG(description, (uint32_t)cycles,
			num_iterations, false, "");

	/* 6. Unblock <start_thread> */

	k_sem_give(&sem);

	/* 8. Wait for <start_thread> to finish */

	k_thread_join(&start_thread, K_FOREVER);

	/* 9. Retrieve the number of cycles spent taking the semaphore */

	cycles = timestamp.cycles;

	snprintf(tag, sizeof(tag),
		 "semaphore.take.immediate.%s",
		 (options & K_USER) == K_USER ? "user" : "kernel");
	snprintf(description, sizeof(description),
		 "%-40s - Take a semaphore (no blocking)", tag);

	PRINT_STATS_AVG(description, (uint32_t)cycles,
			num_iterations, false, "");

	timing_stop();

	return 0;
}
