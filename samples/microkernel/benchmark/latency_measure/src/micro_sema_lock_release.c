/* micro_sema_lock_release.c - measure time for sema lock and release */

/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * DESCRIPTION
 * This file contains the test that measures semaphore and mutex lock and
 * release time in a microkernel. There is no contention on the sema nor the
 * mutex being tested.
 */

#ifdef CONFIG_MICROKERNEL
#include <vxmicro.h>

#include "timestamp.h"
#include "utils.h"

#include <nanokernel/cpu.h>

/* the number of semaphores used in lock/unlock test */
#define N_TEST_SEMA (SEMAEND - SEMASTART)

/* the number of mutex lock/unlock cycles */
#define N_TEST_MUTEX 1000

static uint32_t timestamp;

/*******************************************************************************
 *
 * microSemaLockUnlock - the function tests semaphore lock/unlock time
 *
 * The routine performs unlock the quite amount of semaphores and then
 * acquires them in order to measure the necessary time.
 *
 * RETURNS: 0 on success
 *
 * \NOMANUAL
 */

int microSemaLockUnlock(void)
	{
	int i;

	PRINT_FORMAT(" 3- Measure average time to signal a sema then test"
		  " that sema");
	bench_test_start();
	timestamp = TIME_STAMP_DELTA_GET(0);
	for (i = SEMASTART; i <= SEMAEND; i++) {
	task_sem_give(i);
	}
	timestamp = TIME_STAMP_DELTA_GET(timestamp);
	if (bench_test_end () == 0) {
	PRINT_FORMAT(" Average semaphore signal time %lu tcs = %lu nsec",
		      timestamp / N_TEST_SEMA,
		      SYS_CLOCK_HW_CYCLES_TO_NS_AVG(timestamp, N_TEST_SEMA));
	}
	else {
	errorCount++;
	PRINT_OVERFLOW_ERROR();
	}

	bench_test_start();
	timestamp = TIME_STAMP_DELTA_GET(0);
	for (i = SEMASTART; i <= SEMAEND; i++) {
	task_sem_take_wait(i);
	}
	timestamp = TIME_STAMP_DELTA_GET(timestamp);
	if (bench_test_end () == 0) {
	PRINT_FORMAT(" Average semaphore test time %lu tcs = %lu nsec",
		      timestamp / N_TEST_SEMA,
		      SYS_CLOCK_HW_CYCLES_TO_NS_AVG(timestamp, N_TEST_SEMA));
	}
	else {
	errorCount++;
	PRINT_OVERFLOW_ERROR();
	}
	return 0;
	}

/*******************************************************************************
 *
 * microMutexLockUnlock - test for the multiple mutex lock/unlock time
 *
 * The routine performs multiple mutex locks and then multiple mutex
 * unlocks to measure the necessary time.
 *
 * RETURNS: 0 on success
 *
 * \NOMANUAL
 */

int microMutexLockUnlock(void)
	{
	int i;

	PRINT_FORMAT(" 4- Measure average time to lock a mutex then"
		  " unlock that mutex");
	timestamp = TIME_STAMP_DELTA_GET(0);
	for (i = 0; i < N_TEST_MUTEX; i++) {
	task_mutex_lock_wait(TEST_MUTEX);
	}
	timestamp = TIME_STAMP_DELTA_GET(timestamp);
	PRINT_FORMAT(" Average time to lock the mutex %lu tcs = %lu nsec",
		  timestamp / N_TEST_MUTEX,
		  SYS_CLOCK_HW_CYCLES_TO_NS_AVG(timestamp, N_TEST_MUTEX));
	timestamp = TIME_STAMP_DELTA_GET(0);
	for (i = 0; i <= N_TEST_MUTEX; i++) {
	task_mutex_unlock(TEST_MUTEX);
	}
	timestamp = TIME_STAMP_DELTA_GET(timestamp);
	PRINT_FORMAT(" Average time to unlock the mutex %lu tcs = %lu nsec",
		  timestamp / N_TEST_MUTEX,
		  SYS_CLOCK_HW_CYCLES_TO_NS_AVG(timestamp, N_TEST_MUTEX));
	return 0;
	}

#endif /* CONFIG_MICROKERNEL */
