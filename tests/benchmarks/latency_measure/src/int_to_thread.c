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

#include <kernel.h>
#include "utils.h"

#include <irq_offload.h>

static volatile int flag_var;

static timing_t timestamp_start;
static timing_t timestamp_end;

/**
 *
 * @brief Test ISR used to measure best case interrupt latency
 *
 * The interrupt handler gets the second timestamp.
 *
 * @return N/A
 */
static void latency_test_isr(const void *unused)
{
	ARG_UNUSED(unused);
	flag_var = 1;

	timestamp_start = timing_counter_get();
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
		printk(" Flag variable has not changed. FAILED\n");
	} else {
		timestamp_end = timing_counter_get();
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
	uint32_t diff;

	timing_start();
	TICK_SYNCH();
	make_int();
	if (flag_var == 1) {
		diff = timing_cycles_get(&timestamp_start, &timestamp_end);
		PRINT_STATS("Switch from ISR back to interrupted thread", diff);
	}
	timing_stop();
	return 0;
}
