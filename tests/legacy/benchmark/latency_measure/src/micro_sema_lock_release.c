/* micro_sema_lock_release.c - measure time for sema lock and release */

/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * DESCRIPTION
 * This file contains the test that measures semaphore and mutex lock and
 * release time in a microkernel. There is no contention on the sema nor the
 * mutex being tested.
 */

#include <zephyr.h>

#include "timestamp.h"
#include "utils.h"

#include <arch/cpu.h>

/* the number of semaphore give/take cycles */
#define N_TEST_SEMA 1000

/* the number of mutex lock/unlock cycles */
#define N_TEST_MUTEX 1000

static uint32_t timestamp;

/**
 *
 * @brief The function tests semaphore lock/unlock time
 *
 * The routine performs unlock the quite amount of semaphores and then
 * acquires them in order to measure the necessary time.
 *
 * @return 0 on success
 */
int microSemaLockUnlock(void)
{
	int i;

	PRINT_FORMAT(" 3- Measure average time to signal a sema then test"
		     " that sema");
	bench_test_start();
	timestamp = TIME_STAMP_DELTA_GET(0);
	for (i = 0; i < N_TEST_SEMA; i++) {
		task_sem_give(SEMA_LOCK_UNLOCK);
	}
	timestamp = TIME_STAMP_DELTA_GET(timestamp);
	if (bench_test_end() == 0) {
		PRINT_FORMAT(" Average semaphore signal time %lu tcs = %lu"
			     " nsec",
			     timestamp / N_TEST_SEMA,
			     SYS_CLOCK_HW_CYCLES_TO_NS_AVG(timestamp,
							   N_TEST_SEMA));
	} else {
		errorCount++;
		PRINT_OVERFLOW_ERROR();
	}

	bench_test_start();
	timestamp = TIME_STAMP_DELTA_GET(0);
	for (i = 0; i < N_TEST_SEMA; i++) {
		task_sem_take(SEMA_LOCK_UNLOCK, TICKS_UNLIMITED);
	}
	timestamp = TIME_STAMP_DELTA_GET(timestamp);
	if (bench_test_end() == 0) {
		PRINT_FORMAT(" Average semaphore test time %lu tcs = %lu "
			     "nsec",
			     timestamp / N_TEST_SEMA,
			     SYS_CLOCK_HW_CYCLES_TO_NS_AVG(timestamp,
							   N_TEST_SEMA));
	} else {
		errorCount++;
		PRINT_OVERFLOW_ERROR();
	}
	return 0;
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
int microMutexLockUnlock(void)
{
	int i;

	PRINT_FORMAT(" 4- Measure average time to lock a mutex then"
		     " unlock that mutex");
	timestamp = TIME_STAMP_DELTA_GET(0);
	for (i = 0; i < N_TEST_MUTEX; i++) {
		task_mutex_lock(TEST_MUTEX, TICKS_UNLIMITED);
	}
	timestamp = TIME_STAMP_DELTA_GET(timestamp);
	PRINT_FORMAT(" Average time to lock the mutex %lu tcs = %lu nsec",
		     timestamp / N_TEST_MUTEX,
		     SYS_CLOCK_HW_CYCLES_TO_NS_AVG(timestamp, N_TEST_MUTEX));
	timestamp = TIME_STAMP_DELTA_GET(0);
	for (i = 0; i < N_TEST_MUTEX; i++) {
		task_mutex_unlock(TEST_MUTEX);
	}
	timestamp = TIME_STAMP_DELTA_GET(timestamp);
	PRINT_FORMAT(" Average time to unlock the mutex %lu tcs = %lu nsec",
		     timestamp / N_TEST_MUTEX,
		     SYS_CLOCK_HW_CYCLES_TO_NS_AVG(timestamp, N_TEST_MUTEX));
	return 0;
}
