/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
 * Copyright (c) 2020,2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file measure time for mutex lock and unlock
 *
 * This file contains the test that measures mutex lock and unlock times
 * in the kernel. There is no contention on the mutex being tested.
 */

#include <zephyr/kernel.h>
#include <zephyr/timing/timing.h>
#include "utils.h"
#include "timing_sc.h"

static K_MUTEX_DEFINE(test_mutex);

static void start_lock_unlock(void *p1, void *p2, void *p3)
{
	uint32_t  i;
	uint32_t  num_iterations = (uint32_t)(uintptr_t)p1;
	timing_t  start;
	timing_t  finish;
	uint64_t  lock_cycles;
	uint64_t  unlock_cycles;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	start = timing_timestamp_get();

	/* Recursively lock take the mutex */

	for (i = 0; i < num_iterations; i++) {
		k_mutex_lock(&test_mutex, K_NO_WAIT);
	}

	finish = timing_timestamp_get();

	lock_cycles = timing_cycles_get(&start, &finish);

	start = timing_timestamp_get();

	/* Recursively unlock the mutex */

	for (i = 0; i < num_iterations; i++) {
		k_mutex_unlock(&test_mutex);
	}

	finish = timing_timestamp_get();

	unlock_cycles = timing_cycles_get(&start, &finish);

	timestamp.cycles = lock_cycles;
	k_sem_take(&pause_sem, K_FOREVER);

	timestamp.cycles = unlock_cycles;
}


/**
 *
 * @brief Test for the multiple mutex lock/unlock time
 *
 * The routine performs multiple mutex locks and then multiple mutex
 * unlocks to measure the necessary time.
 *
 * @return 0 on success
 */
int mutex_lock_unlock(uint32_t num_iterations, uint32_t options)
{
	char tag[50];
	char description[120];
	int  priority;
	uint64_t  cycles;

	timing_start();

	priority = k_thread_priority_get(k_current_get());

	k_thread_create(&start_thread, start_stack,
			K_THREAD_STACK_SIZEOF(start_stack),
			start_lock_unlock,
			(void *)(uintptr_t)num_iterations, NULL, NULL,
			priority - 1, options, K_FOREVER);

	k_thread_access_grant(&start_thread, &test_mutex, &pause_sem);
	k_thread_start(&start_thread);

	cycles = timestamp.cycles;
	k_sem_give(&pause_sem);

	snprintf(tag, sizeof(tag),
		 "mutex.lock.immediate.recursive.%s",
		 (options & K_USER) == K_USER ? "user" : "kernel");
	snprintf(description, sizeof(description),
		 "%-40s - Lock a mutex", tag);
	PRINT_STATS_AVG(description, (uint32_t)cycles, num_iterations,
			false, "");

	cycles = timestamp.cycles;

	snprintf(tag, sizeof(tag),
		 "mutex.unlock.immediate.recursive.%s",
		 (options & K_USER) == K_USER ? "user" : "kernel");
	snprintf(description, sizeof(description),
		 "%-40s - Unlock a mutex", tag);
	PRINT_STATS_AVG(description, (uint32_t)cycles, num_iterations,
			false, "");

	timing_stop();
	return 0;
}
