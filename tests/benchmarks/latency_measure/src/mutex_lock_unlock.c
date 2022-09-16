/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/timing/timing.h>
#include "utils.h"

/* the number of mutex lock/unlock cycles */
#define N_TEST_MUTEX 1000

K_MUTEX_DEFINE(test_mutex);


/**
 *
 * @brief Test for the multiple mutex lock/unlock time
 *
 * The routine performs multiple mutex locks and then multiple mutex
 * unlocks to measure the necessary time.
 *
 * @return 0 on success
 */
int mutex_lock_unlock(void)
{
	int i;
	uint32_t diff;
	timing_t timestamp_start;
	timing_t timestamp_end;

	timing_start();

	timestamp_start = timing_counter_get();

	for (i = 0; i < N_TEST_MUTEX; i++) {
		k_mutex_lock(&test_mutex, K_FOREVER);
	}

	timestamp_end = timing_counter_get();

	diff = timing_cycles_get(&timestamp_start, &timestamp_end);
	PRINT_STATS_AVG("Average time to lock a mutex", diff, N_TEST_MUTEX);

	timestamp_start = timing_counter_get();

	for (i = 0; i < N_TEST_MUTEX; i++) {
		k_mutex_unlock(&test_mutex);
	}

	timestamp_end = timing_counter_get();
	diff = timing_cycles_get(&timestamp_start, &timestamp_end);

	PRINT_STATS_AVG("Average time to unlock a mutex", diff, N_TEST_MUTEX);
	timing_stop();
	return 0;
}
