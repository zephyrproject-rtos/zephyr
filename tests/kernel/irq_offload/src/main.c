/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * This test case verifies the correctness of irq_offload(), an important
 * routine used in many other test cases for running a function in interrupt
 * context, on the IRQ stack.
 *
 */
#include <zephyr.h>
#include <ztest.h>
#include <kernel_structs.h>
#include <irq_offload.h>

volatile u32_t sentinel;
#define SENTINEL_VALUE 0xDEADBEEF

static void offload_function(void *param)
{
	u32_t x = (u32_t)param;

	/* Make sure we're in IRQ context */
	zassert_true(_is_in_isr(), "Not in IRQ context!");

	sentinel = x;
}

/**
 * @brief Verify thread context
 *
 * @ingroup kernel_interrupt_tests
 *
 * @details Check whether offloaded running function is in interrupt
 * context, on the IRQ stack or not.
 */
void test_irq_offload(void)
{
	/**TESTPOINT: Offload to IRQ context*/
	irq_offload(offload_function, (void *)SENTINEL_VALUE);

	zassert_equal(sentinel, SENTINEL_VALUE,
		"irq_offload() didn't work properly");
}


void test_main(void)
{
	ztest_test_suite(irq_offload_fn,
			ztest_unit_test(test_irq_offload));
	ztest_run_test_suite(irq_offload_fn);

}

