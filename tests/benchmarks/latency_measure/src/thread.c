/*
 * Copyright (c) 2020, 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file measure time for various thread operations
 *
 * This file contains the tests that measures the times for the following
 * thread operations from both kernel threads and user threads:
 *  1. Creating a thread
 *  2. Starting a thread
 *  3. Suspending a thread
 *  4. Resuming a thread
 *  5. Aborting a thread
 *
 * It is worthwhile to note that there is no measurement for creating a kernel
 * thread from a user thread as that is an invalid operation.
 */

#include <zephyr/kernel.h>
#include <zephyr/timing/timing.h>
#include "utils.h"
#include "timing_sc.h"

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)

#define START_ALT   0x01
#define ALT_USER    0x02

static void alt_thread_entry(void *p1, void *p2, void *p3)
{
	int priority;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* 3. Finish measuring time to start <alt_thread> */

	timestamp.sample = timing_timestamp_get();

	/* 4. Let <start_thread> process the time measurement. */

	k_sem_take(&pause_sem, K_FOREVER);  /* Let 'start_thread' execute */

	/* 7. Begin measuring time to suspend active thread (self/alt_thread) */

	timestamp.sample = timing_timestamp_get();
	k_thread_suspend(&alt_thread);

	/* 10. Finish measuring time to resume <alt_thread> (self) */

	timestamp.sample = timing_timestamp_get();

	/* 11. Lower the priority so <start_thread> can terminate us. */

	priority = k_thread_priority_get(&alt_thread);
	k_thread_priority_set(&alt_thread, priority + 2);
}

static void start_thread_entry(void *p1, void *p2, void *p3)
{
	uint32_t num_iterations = (uint32_t)(uintptr_t)p1;
	uint32_t bit_options = (uint32_t)(uintptr_t)p2;
	timing_t start;
	timing_t finish;
	uint64_t thread_create_sum = 0ull;
	uint64_t thread_start_sum = 0ull;
	uint64_t thread_suspend_sum = 0ull;
	uint64_t thread_resume_sum = 0ull;
	uint64_t thread_abort_sum = 0ull;
	int priority;

	ARG_UNUSED(p3);

	priority = k_thread_priority_get(&start_thread);

	for (uint32_t i = 0; i < num_iterations; i++) {

		/* 1. Measure time to create, but not start <alt_thread> */

		if ((bit_options & START_ALT) == START_ALT) {
			start = timing_timestamp_get();
			k_thread_create(&alt_thread, alt_stack,
					K_THREAD_STACK_SIZEOF(alt_stack),
					alt_thread_entry, NULL, NULL, NULL,
					priority,
					(bit_options & ALT_USER) == ALT_USER ?
						K_USER : 0, K_FOREVER);
			finish = timing_timestamp_get();

			thread_create_sum += timing_cycles_get(&start, &finish);
		} else {

			/*
			 * Wait for the "main" thread to create <alt_thread>
			 * as this thread can not do it.
			 */

			k_sem_take(&pause_sem, K_FOREVER);
		}

		if ((bit_options & ALT_USER) == ALT_USER) {
			k_thread_access_grant(&alt_thread, &pause_sem);
		}

		/*
		 * Let the main thread change the priority of <alt_thread>
		 * to a higher priority level as user threads may not create
		 * a thread of higher priority than itself.
		 */

		k_sem_take(&pause_sem, K_FOREVER);


		/* 2. Begin measuring time to start <alt_thread> */

		start = timing_timestamp_get();
		k_thread_start(&alt_thread);

		/* 5. Process the time to start <alt_thread> */

		finish = timestamp.sample;
		thread_start_sum += timing_cycles_get(&start, &finish);

		/* 6. Allow <alt_thread> to continue */

		k_sem_give(&pause_sem);

		/* 8. Finish measuring time to suspend <alt_thread> */

		start = timestamp.sample;
		finish = timing_timestamp_get();
		thread_suspend_sum += timing_cycles_get(&start, &finish);

		/* 9. Being measuring time to resume <alt_thread> */

		start = timing_timestamp_get();
		k_thread_resume(&alt_thread);

		/* 12. Process the time it took to resume <alt_thread> */

		finish = timestamp.sample;
		thread_resume_sum += timing_cycles_get(&start, &finish);

		/* 13. Process the time to terminate <alt_thread> */

		start = timing_timestamp_get();
		k_thread_abort(&alt_thread);
		finish = timing_timestamp_get();
		thread_abort_sum += timing_cycles_get(&start, &finish);
	}

	timestamp.cycles = thread_create_sum;
	k_sem_take(&pause_sem, K_FOREVER);

	timestamp.cycles = thread_start_sum;
	k_sem_take(&pause_sem, K_FOREVER);

	timestamp.cycles = thread_suspend_sum;
	k_sem_take(&pause_sem, K_FOREVER);

	timestamp.cycles = thread_resume_sum;
	k_sem_take(&pause_sem, K_FOREVER);

	timestamp.cycles = thread_abort_sum;
	k_sem_take(&pause_sem, K_FOREVER);
}

int thread_ops(uint32_t num_iterations, uint32_t start_options, uint32_t alt_options)
{
	int priority;
	uint64_t  cycles;
	uint32_t  bit_options = START_ALT;
	char tag[50];
	char description[120];

	priority = k_thread_priority_get(k_current_get());

	timing_start();

	/*
	 * Determine if <start_thread> is allowed to start <alt_thread>.
	 * If it can not, then <alt_thread) must be created by the current
	 * thread.
	 */

	if (((start_options & K_USER) == K_USER) &&
	    ((alt_options & K_USER) == 0)) {
		bit_options = 0;
	}

	if ((alt_options & K_USER) == K_USER) {
		bit_options |= ALT_USER;
	}

	k_thread_create(&start_thread, start_stack,
			K_THREAD_STACK_SIZEOF(start_stack),
			start_thread_entry,
			(void *)(uintptr_t)num_iterations,
			(void *)(uintptr_t)bit_options, NULL,
			priority - 1, start_options, K_FOREVER);

	if ((start_options & K_USER) == K_USER) {
		k_thread_access_grant(&start_thread, &alt_thread, &alt_stack,
				      &pause_sem);
	}

	k_thread_start(&start_thread);

	for (uint32_t i = 0; i < num_iterations; i++) {
		if ((bit_options & START_ALT) == 0) {

			/*
			 * <start_thread> can not create <alt_thread> as it
			 * would be a user thread trying to create a kernel
			 * thread. Instead, create <alt_thread> here.
			 */

			k_thread_create(&alt_thread, alt_stack,
					K_THREAD_STACK_SIZEOF(alt_stack),
					alt_thread_entry,
					NULL, NULL, NULL,
					priority - 1, alt_options, K_FOREVER);

			/* Give <pause_sem> sends us back to <start_thread> */

			k_sem_give(&pause_sem);
		}

		/*
		 * <alt_thread> needs to be of higher priority than
		 * <start_thread>, which can not always be done in
		 * <start_thread> as sometimes it is a user thread.
		 */

		k_thread_priority_set(&alt_thread, priority - 2);
		k_sem_give(&pause_sem);
	}

	cycles = timestamp.cycles;
	cycles -= timestamp_overhead_adjustment(start_options, alt_options);
	k_sem_give(&pause_sem);

	if ((bit_options & START_ALT) == START_ALT) {

		/* Only report stats if <start_thread> created <alt_thread> */

		snprintf(tag, sizeof(tag),
			 "thread.create.%s.from.%s",
			 (alt_options & K_USER) != 0 ? "user" : "kernel",
			 (start_options & K_USER) != 0 ? "user" : "kernel");
		snprintf(description, sizeof(description),
			 "%-40s - Create thread", tag);

		PRINT_STATS_AVG(description, (uint32_t)cycles,
				num_iterations, false, "");
	}

	cycles = timestamp.cycles;
	cycles -= timestamp_overhead_adjustment(start_options, alt_options);
	k_sem_give(&pause_sem);

	snprintf(tag, sizeof(tag),
		 "thread.start.%s.from.%s",
		 (alt_options & K_USER) != 0 ? "user" : "kernel",
		 (start_options & K_USER) != 0 ? "user" : "kernel");
	snprintf(description, sizeof(description),
		 "%-40s - Start thread", tag);

	PRINT_STATS_AVG(description, (uint32_t)cycles,
			num_iterations, false, "");

	cycles = timestamp.cycles;
	cycles -= timestamp_overhead_adjustment(start_options, alt_options);
	k_sem_give(&pause_sem);

	snprintf(tag, sizeof(tag),
		 "thread.suspend.%s.from.%s",
		 (alt_options & K_USER) != 0 ? "user" : "kernel",
		 (start_options & K_USER) != 0 ? "user" : "kernel");
	snprintf(description, sizeof(description),
		 "%-40s - Suspend thread", tag);

	PRINT_STATS_AVG(description, (uint32_t)cycles,
			num_iterations, false, "");

	cycles = timestamp.cycles;
	cycles -= timestamp_overhead_adjustment(start_options, alt_options);
	k_sem_give(&pause_sem);

	snprintf(tag, sizeof(tag),
		 "thread.resume.%s.from.%s",
		 (alt_options & K_USER) != 0 ? "user" : "kernel",
		 (start_options & K_USER) != 0 ? "user" : "kernel");
	snprintf(description, sizeof(description),
		 "%-40s - Resume thread", tag);

	PRINT_STATS_AVG(description, (uint32_t)cycles,
			num_iterations, false, "");

	cycles = timestamp.cycles;
	cycles -= timestamp_overhead_adjustment(start_options, alt_options);
	k_sem_give(&pause_sem);

	snprintf(tag, sizeof(tag),
		 "thread.abort.%s.from.%s",
		 (alt_options & K_USER) != 0 ? "user" : "kernel",
		 (start_options & K_USER) != 0 ? "user" : "kernel");
	snprintf(description, sizeof(description),
		 "%-40s - Abort thread", tag);

	PRINT_STATS_AVG(description, (uint32_t)cycles,
			num_iterations, false, "");

	timing_stop();
	return 0;
}
