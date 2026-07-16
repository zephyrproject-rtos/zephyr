/*
 * Copyright (c) 2024 Keith Packard
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

/**
 * @defgroup kernel_constructor_tests Constructors
 * @ingroup all_tests
 * @{
 * @}
 *
 * @addtogroup kernel_constructor_tests
 * @{
 */

static int constructor_number;
static int constructor_values[3];

void __attribute__((__constructor__)) __constructor_init(void)
{
	constructor_values[constructor_number++] = 31415;
}

void __attribute__((__constructor__(101))) __constructor_init_priority_101(void)
{
	constructor_values[constructor_number++] = 101;
}

void __attribute__((__constructor__(1000))) __constructor_init_priority_1000(void)
{
	constructor_values[constructor_number++] = 1000;
}
/**
 * @brief Verify C constructors run before main in priority order.
 *
 * @ingroup kernel_constructor_tests
 *
 * @details
 * Passing proves that functions marked with the GCC __constructor__ attribute
 * are invoked during system startup before the test runs, and that prioritized
 * constructors execute in ascending priority order ahead of unprioritized ones.
 *
 * Test steps:
 * - Register three constructors that each record an identifier on invocation:
 *   priority 101, priority 1000, and one with no explicit priority.
 * - In the test body, inspect the recorded count and ordering.
 *
 * Expected result:
 * - All three constructors ran (count is 3), priority 101 ran first, priority
 *   1000 ran second, and the unprioritized constructor ran last.
 *
 * @see ZTEST_SUITE()
 * @verifies ZEP-SRS-12-10
 */
ZTEST(constructor, test_constructor)
{
	zassert_equal(constructor_number, 3,
		      "constructor test failed: constructor missing");
	zassert_equal(constructor_values[0], 101,
		      "constructor priority test failed:"
		      "constructor 101 not called first");
	zassert_equal(constructor_values[1], 1000,
		      "constructor priority test failed:"
		      "constructor 1000 not called second");
	zassert_equal(constructor_values[2], 31415,
		      "constructor priority test failed:"
		      "constructor without priority not called last");
}

/**
 * @}
 */

extern void *common_setup(void);
ZTEST_SUITE(constructor, NULL, common_setup, NULL, NULL, NULL);
