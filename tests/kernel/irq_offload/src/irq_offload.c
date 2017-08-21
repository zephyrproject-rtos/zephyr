/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <tc_util.h>
#include <kernel_structs.h>
#include <irq_offload.h>
#include <ztest.h>

volatile u32_t sentinel;
#define SENTINEL_VALUE 0xDEADBEEF

static void offload_function(void *param)
{
	u32_t x = (u32_t)param;

	SYS_LOG_INF("offload_function running\n");

	/* Make sure we're in IRQ context */
	zassert_true(_is_in_isr(), "Not in IRQ context!\n");

	sentinel = x;
}

void test_irq_offload(void)
{
	/**TESTPOINT: Offload to IRQ context*/
	irq_offload(offload_function, (void *)SENTINEL_VALUE);

	zassert_equal(sentinel, SENTINEL_VALUE,
		"irq_offload() didn't work properly\n");
}
