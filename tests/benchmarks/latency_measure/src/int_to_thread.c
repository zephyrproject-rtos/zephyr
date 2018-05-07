/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Measure time from ISR back to interrupted thread
 *
 * This file contains test that measures time to switch from the interrupt
 * handler back to the interrupted thread.
 */

#include "timestamp.h"
#include "utils.h"

#include <arch/cpu.h>
#include <irq_offload.h>

static volatile int flag_var;

static u32_t timestamp;

/**
 *
 * @brief Test ISR used to measure best case interrupt latency
 *
 * The interrupt handler gets the second timestamp.
 *
 * @return N/A
 */
static void latency_test_isr(void *unused)
{
	ARG_UNUSED(unused);

	flag_var = 1;
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
static void make_int(void)
{
	flag_var = 0;
	irq_offload(latency_test_isr, NULL);
	if (flag_var != 1) {
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
int int_to_thread(void)
{
	PRINT_FORMAT(" 1 - Measure time to switch from ISR back to"
		     " interrupted thread");
	TICK_SYNCH();
	make_int();
	if (flag_var == 1) {
		PRINT_FORMAT(" switching time is %u tcs = %u nsec",
			     timestamp, SYS_CLOCK_HW_CYCLES_TO_NS(timestamp));
	}
	return 0;
}
