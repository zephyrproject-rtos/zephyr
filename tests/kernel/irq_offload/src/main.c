/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <tc_util.h>
#include <kernel_structs.h>
#include <irq_offload.h>

volatile uint32_t sentinel;
#define SENTINEL_VALUE 0xDEADBEEF

void offload_function(void *param)
{
	uint32_t x = (uint32_t)param;

	TC_PRINT("offload_function running\n");

	/* Make sure we're in IRQ context */
	if (!_is_in_isr()) {
		TC_PRINT("Not in IRQ context!\n");
		return;
	}

	sentinel = x;
}

void main(void)
{
	int rv = TC_PASS;

	TC_START("test_irq_offload");

	irq_offload(offload_function, (void *)SENTINEL_VALUE);

	if (sentinel != SENTINEL_VALUE) {
		TC_PRINT("irq_offload() didn't work properly\n");
		rv = TC_FAIL;
	}

	TC_END_RESULT(rv);
	TC_END_REPORT(rv);
}
