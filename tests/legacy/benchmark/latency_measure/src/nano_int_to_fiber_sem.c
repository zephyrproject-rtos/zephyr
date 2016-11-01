/* nano_int_to_fiber_sem.c - measure switching time from ISR to diff fiber */

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
 * This file contains a test which measure switching time from interrupt
 * handler execution to executing a different fiber than the one which got
 * interrupted.
 * The test starts a higher priority fiber (fiberWaiter) which blocks on a
 * semaphore thus can't run. Then a lower priority fiber (fiberInt) is started
 * which sets up an interrupt handler and invokes the software interrupt. The
 * interrupt handler releases the semaphore which enabled the high priority
 * fiberWaiter to run and exit. The high priority fiber acquire the sema and
 * read the time. The time delta is measured from the time
 * semaphore is released in interrupt handler to the time fiberWaiter
 * starts to executing.
 */

#include "timestamp.h"
#include "utils.h"

#include <arch/cpu.h>
#include <irq_offload.h>

#ifndef STACKSIZE
#define STACKSIZE 512
#endif

/* stack used by the fibers */
static char __stack waiterStack[STACKSIZE];
static char __stack intStack[STACKSIZE];

/* semaphore taken by waiting fiber ad released by the interrupt handler */
static struct nano_sem testSema;

static uint32_t timestamp;

/**
 *
 * @brief Test ISR used to measure best case interrupt latency
 *
 * The interrupt handler gets the second timestamp.
 *
 * @return N/A
 */
static void latencyTestIsr(void *unused)
{
	ARG_UNUSED(unused);

	nano_isr_sem_give(&testSema);
	timestamp = TIME_STAMP_DELTA_GET(0);
}

/**
 *
 * @brief Interrupt preparation fiber
 *
 * Fiber makes all the test preparations: registers the interrupt handler,
 * gets the first timestamp and invokes the software interrupt.
 *
 * @return N/A
 */
static void fiberInt(void)
{
	irq_offload(latencyTestIsr, NULL);
	fiber_yield();
}

/**
 *
 * @brief Check the time when it gets executed after the semaphore
 *
 * Fiber starts, waits on semaphore. When the interrupt handler releases
 * the semaphore, fiber measures the time.
 *
 * @return N/A
 */
static void fiberWaiter(void)
{
	nano_fiber_sem_take(&testSema, TICKS_UNLIMITED);
	timestamp = TIME_STAMP_DELTA_GET(timestamp);
}

/**
 *
 * @brief The test main function
 *
 * @return 0 on success
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
