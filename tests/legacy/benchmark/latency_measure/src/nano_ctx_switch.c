/* nano_ctx_switch.c - measure context switch time between fibers */

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
 * This file contains fiber context switch time measurement.
 * The task starts two fibers. One fiber waits on a semaphore. The other,
 * after starting, releases a semaphore which enable the first fiber to run.
 * Each fiber increases a common global counter and context switch back and
 * forth by yielding the cpu. When counter reaches the maximal value, fibers
 * stop and the average time of context switch is displayed.
 */

#include "timestamp.h"
#include "utils.h"

#include <arch/cpu.h>

/* number of context switches */
#define NCTXSWITCH   10000
#ifndef STACKSIZE
#define STACKSIZE    512
#endif

/* stack used by the fibers */
static char __stack fiberOneStack[STACKSIZE];
static char __stack fiberTwoStack[STACKSIZE];

/* semaphore used for fibers synchronization */
static struct nano_sem syncSema;

static uint32_t timestamp;

/* context switches counter */
static volatile uint32_t ctxSwitchCounter;

/* context switch balancer. Incremented by one fiber, decremented by another*/
static volatile int ctxSwitchBalancer;

/**
 *
 * fiberOne
 *
 * Fiber makes all the test preparations: registers the interrupt handler,
 * gets the first timestamp and invokes the software interrupt.
 *
 * @return N/A
 */
static void fiberOne(void)
{
	nano_fiber_sem_take(&syncSema, TICKS_UNLIMITED);
	timestamp = TIME_STAMP_DELTA_GET(0);
	while (ctxSwitchCounter < NCTXSWITCH) {
		fiber_yield();
		ctxSwitchCounter++;
		ctxSwitchBalancer--;
	}
	timestamp = TIME_STAMP_DELTA_GET(timestamp);
}

/**
 *
 * @brief Check the time when it gets executed after the semaphore
 *
 * Fiber starts, waits on semaphore. When the interrupt handler releases
 * the semaphore, fiber measures the time.
 *
 * @return 0 on success
 */
static void fiberTwo(void)
{
	nano_fiber_sem_give(&syncSema);
	while (ctxSwitchCounter < NCTXSWITCH) {
		fiber_yield();
		ctxSwitchCounter++;
		ctxSwitchBalancer++;
	}
}

/**
 *
 * @brief The test main function
 *
 * @return 0 on success
 */
int nanoCtxSwitch(void)
{
	PRINT_FORMAT(" 4- Measure average context switch time between fibers");
	nano_sem_init(&syncSema);
	ctxSwitchCounter = 0;
	ctxSwitchBalancer = 0;

	bench_test_start();
	task_fiber_start(&fiberOneStack[0], STACKSIZE,
			 (nano_fiber_entry_t) fiberOne, 0, 0, 6, 0);
	task_fiber_start(&fiberTwoStack[0], STACKSIZE,
			 (nano_fiber_entry_t) fiberTwo, 0, 0, 6, 0);
	if (ctxSwitchBalancer > 3 || ctxSwitchBalancer < -3) {
		PRINT_FORMAT(" Balance is %d. FAILED", ctxSwitchBalancer);
	} else if (bench_test_end() != 0) {
		errorCount++;
		PRINT_OVERFLOW_ERROR();
	} else {
		PRINT_FORMAT(" Average context switch time is %lu tcs = %lu"
			     " nsec",
			     timestamp / ctxSwitchCounter,
			     SYS_CLOCK_HW_CYCLES_TO_NS_AVG(timestamp,
							   ctxSwitchCounter));
	}
	return 0;
}
