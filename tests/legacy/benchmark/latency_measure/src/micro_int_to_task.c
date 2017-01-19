/* micro_int_to_task.c - measure time from ISR back to interrupted task */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * DESCRIPTION
 * This file contains test that measures time to switch from the interrupt
 * handler back to the interrupted task in microkernel.
 */

#include "timestamp.h"
#include "utils.h"

#include <arch/cpu.h>
#include <irq_offload.h>

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
 * @brief Interrupt preparation function
 *
 * Function makes all the test preparations: registers the interrupt handler,
 * gets the first timestamp and invokes the software interrupt.
 *
 * @return N/A
 */
static void makeInt(void)
{
	flagVar = 0;
	irq_offload(latencyTestIsr, NULL);
	if (flagVar != 1) {
		PRINT_FORMAT(" Flag variable has not changed. FAILED\n");
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
int microIntToTask(void)
{
	PRINT_FORMAT(" 1- Measure time to switch from ISR back to"
		     " interrupted task");
	TICK_SYNCH();
	makeInt();
	if (flagVar == 1) {
		PRINT_FORMAT(" switching time is %lu tcs = %lu nsec",
			     timestamp, SYS_CLOCK_HW_CYCLES_TO_NS(timestamp));
	}
	return 0;
}
