/* nano_int_to_fiber.c - measure switching time from ISR back to fiber */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * DESCRIPTION
 * This file contains test that measures the switching time from the
 * interrupt handler back to the executing fiber that got interrupted.
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

static volatile int flagVar;

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

	flagVar = 1;
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
	flagVar = 0;
	irq_offload(latencyTestIsr, NULL);
	if (flagVar != 1) {
		PRINT_FORMAT(" Flag variable has not changed. FAILED");
	} else {
		timestamp = TIME_STAMP_DELTA_GET(timestamp);
	}
}

/**
 *
 * @brief The test main function
 *
 * @return 0 on success
 */
int nanoIntToFiber(void)
{
	PRINT_FORMAT(" 2- Measure time to switch from ISR back to interrupted"
		     " fiber");
	TICK_SYNCH();
	task_fiber_start(&fiberStack[0], STACKSIZE,
			 (nano_fiber_entry_t) fiberInt, 0, 0, 6, 0);
	if (flagVar == 1) {
		PRINT_FORMAT(" switching time is %lu tcs = %lu nsec",
			     timestamp, SYS_CLOCK_HW_CYCLES_TO_NS(timestamp));
	}
	return 0;
}
