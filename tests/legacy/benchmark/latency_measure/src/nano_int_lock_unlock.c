/* nano_int_lock_unlock.c - measure time for interrupts lock and unlock */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
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
 * This file contains test that measures average time needed to do a call
 * to lock the interrupt lock and a call to unlock the interrupts. Typically
 * users calls both of these functions to ensure interrupts are lock while
 * some code executes. No explicit interrupts are generated during the test
 * so the interrupt handler does not run.
 */

#include "timestamp.h"
#include "utils.h"

#include <arch/cpu.h>

/* total number of interrupt lock/unlock cycles */
#define NTESTS 100000

static uint32_t timestamp;

/**
 *
 * @brief The test main function
 *
 * @return 0 on success
 */
int nanoIntLockUnlock(void)
{
	int i;
	unsigned int mask;

	PRINT_FORMAT(" 5- Measure average time to lock then unlock interrupts");
	bench_test_start();
	timestamp = TIME_STAMP_DELTA_GET(0);
	for (i = 0; i < NTESTS; i++) {
		mask = irq_lock();
		irq_unlock(mask);
	}
	timestamp = TIME_STAMP_DELTA_GET(timestamp);
	if (bench_test_end() == 0) {
		PRINT_FORMAT(" Average time for lock then unlock "
			     "is %lu tcs = %lu nsec",
			     timestamp / NTESTS,
			     SYS_CLOCK_HW_CYCLES_TO_NS_AVG(timestamp, NTESTS));
	} else {
		errorCount++;
		PRINT_OVERFLOW_ERROR();
	}
	return 0;
}
