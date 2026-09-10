/*
 * Copyright (c) 2016 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

/**
 * @brief Multilib / compiler runtime tests
 *
 * @defgroup kernel_multilib_tests Multilib
 *
 * @ingroup all_tests
 * @{
 * @}
 */

/**
 * @brief Verify the correct multilib variant is selected by the toolchain.
 *
 * @ingroup kernel_multilib_tests
 *
 * @details
 * Passing proves that the build is linked against a multilib that provides
 * working 64-bit integer arithmetic (compiler runtime support), so that
 * long long operations produce correct results at runtime.
 *
 * Test steps:
 * - Compute a 64-bit integer division (100 / 3) using volatile operands.
 * - Assert the quotient equals the expected value.
 *
 * Expected result:
 * - The division yields 33, confirming the proper multilib is linked.
 *
 * @see ZTEST_SUITE()
 */
ZTEST(multilib, test_multilib)
{
	volatile long long a = 100;
	volatile long long b = 3;
	volatile long long c = a / b;

	zassert_equal(c, 33, "smoke-test failed: wrong multilib selected");
}

extern void *common_setup(void);
ZTEST_SUITE(multilib, NULL, common_setup, NULL, NULL, NULL);
