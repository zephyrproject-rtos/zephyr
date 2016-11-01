/* nano_int.c - measure the time from task to ISR */

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
 * This file contains test that measures time to switch time from a fiber
 * to the interrupt handler when an interrupt is generated.
 */

#include "timestamp.h"
#include "utils.h"

#include <arch/cpu.h>
#include <irq_offload.h>

#ifndef STACKSIZE
#define STACKSIZE 512
#endif

/* stack used by the fiber that generates the interrupt */
static char __stack fiberStack[STACKSIZE];

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

	timestamp = TIME_STAMP_DELTA_GET(timestamp);
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
	timestamp = TIME_STAMP_DELTA_GET(0);
	irq_offload(latencyTestIsr, NULL);
}

/**
 *
 * @brief The test main function
 *
 * @return 0 on success
 */
int nanoIntLatency(void)
{
	PRINT_FORMAT(" 1- Measure time to switch from fiber to ISR execution");
	TICK_SYNCH();
	task_fiber_start(&fiberStack[0], STACKSIZE,
			 (nano_fiber_entry_t) fiberInt, 0, 0, 6, 0);
	PRINT_FORMAT(" switching time is %lu tcs = %lu nsec",
		     timestamp, SYS_CLOCK_HW_CYCLES_TO_NS(timestamp));
	return 0;
}
