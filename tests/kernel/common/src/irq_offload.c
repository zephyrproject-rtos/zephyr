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
	u32_t x = POINTER_TO_INT(param);

	/* Make sure we're in IRQ context */
	zassert_true(k_is_in_isr(), "Not in IRQ context!");

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
	/* Simple validation of nested locking. */
	unsigned int key1, key2;

	key1 = z_arch_irq_lock();
	zassert_true(z_arch_irq_unlocked(key1),
		     "IRQs should have been unlocked, but key is 0x%x\n",
		     key1);
	key2 = z_arch_irq_lock();
	zassert_false(z_arch_irq_unlocked(key2),
		      "IRQs should have been locked, but key is 0x%x\n",
		      key2);
	z_arch_irq_unlock(key2);
	z_arch_irq_unlock(key1);

	/**TESTPOINT: Offload to IRQ context*/
	irq_offload(offload_function, (void *)SENTINEL_VALUE);

	zassert_equal(sentinel, SENTINEL_VALUE,
		"irq_offload() didn't work properly");
}

