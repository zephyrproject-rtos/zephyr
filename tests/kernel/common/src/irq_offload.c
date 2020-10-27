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

volatile uint32_t sentinel;
#define SENTINEL_VALUE 0xDEADBEEF

static void offload_function(const void *param)
{
	uint32_t x = POINTER_TO_INT(param);

	/* Make sure we're in IRQ context */
	zassert_true(k_is_in_isr(), "Not in IRQ context!");

	sentinel = x;
}

/**
 * @brief Verify thread context
 *
 * @ingroup kernel_interrupt_tests
 *
 * @details
 * Test Objective:
 * - To verify the kernel architecture layer shall provide a means to raise an
 *   interrupt from thread code that will run a user-provided C function under
 *   the interrupt context.
 * - We check whether offloaded running function is in interrupt context, on
 *   the IRQ stack or not.
 *
 * Testing techniques:
 * - Interface testing, function and black box testing,
 *   dynamic analysis and testing
 *
 * Prerequisite Conditions:
 * - CONFIG_IRQ_OFFLOAD=y
 *
 * Input Specifications:
 * - A pointer of C offload function
 * - A pointer of passing parameter
 *
 * Test Procedure:
 * -# Call irq_offload with parameter offload_function and SENTINEL_VALUE.
 * -# In the C offload_function handler, call k_is_in_isr() to check if it is
 *  in interrupt context currently.
 * -# Store the passing argument into global variable sentinel.
 * -# In main thread, check if the global variable sentinel equals to
 *  SENTINEL_VALUE.
 *
 * Expected Test Result:
 * - The running function is running on interrupt context.
 *
 * Pass/Fail Criteria:
 * - Successful if check points in test procedure are all passed, otherwise
 *   failure.
 *
 * Assumptions and Constraints:
 * - N/A.
 */
void test_irq_offload(void)
{
	/**TESTPOINT: Offload to IRQ context*/
	irq_offload(offload_function, (const void *)SENTINEL_VALUE);

	zassert_equal(sentinel, SENTINEL_VALUE,
		      "irq_offload() didn't work properly");
}

/**
 * @brief Verify nested irq lock
 *
 * @ingroup kernel_interrupt_tests
 *
 * @details
 * Test Objective:
 * - The kernel architecture layer shall provide a mechanism to restore the
 *   local interrupt state saved prior to a mask operation.
 * - We simply check lock function twice to make it is nested, then check if
 *   unlock function works properly.
 *
 * Testing techniques:
 * - Interface testing, function and black box testing,
 *   dynamic analysis and testing
 *
 * Prerequisite Conditions:
 * - N/A
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# In main thread, call arch_irq_lock() and store it's return value in key1.
 * -# Call arch_irq_unlocked(key1), and check if it returns true.
 * -# call arch_irq_lock() and store it's return value in key2.
 * -# Call arch_irq_unlocked(key2), and check if it returns false.
 * -# Call arch_irq_unlocked(key2).
 * -# Call arch_irq_unlocked(key1).
 *
 * Expected Test Result:
 * - The nested lock and unlock works well as expected.
 *
 * Pass/Fail Criteria:
 * - Successful if check points in test procedure are all passed, otherwise
 *   failure.
 *
 * Assumptions and Constraints:
 * - N/A.
 */
void test_nested_locking(void)
{
	/* Simple validation of nested locking. */
	unsigned int key1, key2;

	key1 = arch_irq_lock();
	zassert_true(arch_irq_unlocked(key1),
		     "IRQs should have been unlocked, but key is 0x%x\n",
		     key1);
	key2 = arch_irq_lock();
	zassert_false(arch_irq_unlocked(key2),
		      "IRQs should have been locked, but key is 0x%x\n",
		      key2);
	arch_irq_unlock(key2);
	arch_irq_unlock(key1);
}
