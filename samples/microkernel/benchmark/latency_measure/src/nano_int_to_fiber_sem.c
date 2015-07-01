/* nano_int_to_fiber_sem.c - measure switching time from ISR to diff fiber */

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
 * This file contains a test which measure switching time from interrupt
 * handler execution to executing a different fiber than the one which got
 * interrupted.
 * The test starts a higher priority fiber (fiberWaiter) which blocks on a
 * semaphore thus can't run. Then a lower priority fiber (fiberInt) is started
 * which sets up an interrupt handler and invokes the software interrupt. The
 * interrupt handler releases the semaphore which enabled the high priority
 * fiberWaiter to run and exit. The high priority fiber acquire the sema and
 * read the time. The time delta is measured from the time
 * semaphore is released in interrup handler to the time fiberWaiter
 * starts to executing.
 */

#include "timestamp.h"
#include "utils.h"

#include <arch/cpu.h>

#define STACKSIZE 2000

/* stack used by the fibers */
static char __stack waiterStack[STACKSIZE];
static char __stack intStack[STACKSIZE];

/* semaphore taken by waiting fiber ad released by the interrupt handler */
static struct nano_sem testSema;

static uint32_t timestamp = 0;

/**
 *
 * latencyTestIsr - test ISR used to measure best case interrupt latency
 *
 * The interrupt handler gets the second timestamp.
 *
 * RETURNS: N/A
 *
 * \NOMANUAL
 */

static void latencyTestIsr(void *unused)
{
	ARG_UNUSED(unused);

	nano_isr_sem_give(&testSema);
	timestamp = TIME_STAMP_DELTA_GET(0);
}

/**
 *
 * fiberInt - interrupt preparation fiber
 *
 * Fiber makes all the test preparations: registers the interrupt handler,
 * gets the first timestamp and invokes the software interrupt.
 *
 * RETURNS: N/A
 *
 * \NOMANUAL
 */

static void fiberInt(void)
{
	setSwInterrupt(latencyTestIsr);
	raiseIntFunc();
	fiber_yield();
}

/**
 *
 * fiberWaiter - check the time when it gets executed after the semaphore
 *
 * Fiber starts, waits on semaphore. When the interrupt handler releases
 * the semaphore, fiber measures the time.
 *
 * RETURNS: N/A
 *
 * \NOMANUAL
 */

static void fiberWaiter(void)
{
	nano_fiber_sem_take_wait(&testSema);
	timestamp = TIME_STAMP_DELTA_GET(timestamp);
}

/**
 *
 * nanoIntToFiberSem - the test main function
 *
 * RETURNS: 0 on success
 *
 * \NOMANUAL
 */

int nanoIntToFiberSem(void)
{
	PRINT_FORMAT(" 3- Measure time from ISR to executing a different fiber"
				 " (rescheduled)");
	nano_sem_init(&testSema);

	TICK_SYNCH();
	task_fiber_start(&waiterStack[0], STACKSIZE,
					 (nano_fiber_entry_t) fiberWaiter, 0, 0, 5, 0);
	task_fiber_start(&intStack[0], STACKSIZE,
					 (nano_fiber_entry_t) fiberInt, 0, 0, 6, 0);

	PRINT_FORMAT(" switching time is %lu tcs = %lu nsec",
				 timestamp, SYS_CLOCK_HW_CYCLES_TO_NS(timestamp));
	return 0;
}
