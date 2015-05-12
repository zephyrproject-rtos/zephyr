/* nano_ctx_switch.c - measure context switch time between fibers */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
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
 * This file contains fiber context switch time measurement.
 * The task starts two fibers. One fiber waits on a semaphore. The other,
 * after starting, releases a semaphore which enable the first fiber to run.
 * Each fiber increases a common global counter and context switch back and
 * forth by yielding the cpu. When counter reaches the maximal value, fibers
 * stop and the average time of context switch is displayed.
 */

#include "timestamp.h"
#include "utils.h"

#include <nanokernel/cpu.h>

/* number of context switches */
#define NCTXSWITCH   10000
#define STACKSIZE    2000

/* stack used by the fibers */
static char fiberOneStack[STACKSIZE];
static char fiberTwoStack[STACKSIZE];

/* semaphore used for fibers synchronization */
static struct nano_sem syncSema;

static uint32_t timestamp = 0;

/* context switches counter */
static volatile uint32_t ctxSwitchCounter = 0;

/* context switch balancer. Incremented by one fiber, decremented by another*/
static volatile int ctxSwitchBalancer = 0;

/*******************************************************************************
*
* fiberOne
*
* Fiber makes all the test preparations: registers the interrupt handler,
* gets the first timestamp and invokes the software interrupt.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

static void fiberOne(void)
	{
	nano_fiber_sem_take_wait(&syncSema);
	timestamp = TIME_STAMP_DELTA_GET(0);
	while (ctxSwitchCounter < NCTXSWITCH) {
	fiber_yield();
	ctxSwitchCounter++;
	ctxSwitchBalancer--;
	}
	timestamp = TIME_STAMP_DELTA_GET(timestamp);
	}

/*******************************************************************************
 *
 * fiberWaiter - check the time when it gets executed after the semaphore
 *
 * Fiber starts, waits on semaphore. When the interrupt handler releases
 * the semaphore, fiber measures the time.
 *
 * RETURNS: 0 on success
 *
 * \NOMANUAL
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

/*******************************************************************************
 *
 * nanoCtxSwitch - the test main function
 *
 * RETURNS: 0 on success
 *
 * \NOMANUAL
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
	}
	else if (bench_test_end() != 0) {
	errorCount++;
	PRINT_OVERFLOW_ERROR();
	}
	else
	PRINT_FORMAT(" Average context switch time is %lu tcs = %lu nsec",
		      timestamp / ctxSwitchCounter,
		      SYS_CLOCK_HW_CYCLES_TO_NS_AVG(timestamp, ctxSwitchCounter));
	return 0;
	}
