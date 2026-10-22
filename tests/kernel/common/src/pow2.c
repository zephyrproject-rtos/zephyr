/*
 * Copyright (c) 2022 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

/**
 * @brief Test the Z_POW2_CEIL() macro
 *
 * @defgroup test_pow2_ceil Z_POW2_CEIL() tests
 *
 * @ingroup all_tests
 *
 * @{
 * @}
 */

/*
 * Verify compile-time constant results: static array allocations are sized as
 * expected.
 */

char static_array1[Z_POW2_CEIL(1)];
char static_array2[Z_POW2_CEIL(2)];
char static_array3[Z_POW2_CEIL(3)];
char static_array4[Z_POW2_CEIL(4)];
char static_array5[Z_POW2_CEIL(5)];
char static_array7[Z_POW2_CEIL(7)];
char static_array8[Z_POW2_CEIL(8)];
char static_array9[Z_POW2_CEIL(9)];

BUILD_ASSERT(sizeof(static_array1) == 1);
BUILD_ASSERT(sizeof(static_array2) == 2);
BUILD_ASSERT(sizeof(static_array3) == 4);
BUILD_ASSERT(sizeof(static_array4) == 4);
BUILD_ASSERT(sizeof(static_array5) == 8);
BUILD_ASSERT(sizeof(static_array7) == 8);
BUILD_ASSERT(sizeof(static_array8) == 8);
BUILD_ASSERT(sizeof(static_array9) == 16);

/*
 * Verify run-time non-constant results are as expected. Uses a volatile
 * variable to prevent compiler optimizations.
 */

static void test_pow2_ceil_x(unsigned long test_value,
			     unsigned long expected_result)
{
	volatile unsigned long x = test_value;
	unsigned long result = Z_POW2_CEIL(x);

	zassert_equal(result, expected_result,
		      "ZPOW2_CEIL(%lu) returned %lu, expected %lu",
		      test_value, result, expected_result);
}

/**
 * @brief Verify Z_POW2_CEIL() rounds runtime values up to the next power of two.
 *
 * @ingroup test_pow2_ceil
 *
 * @details
 * Confirms the Z_POW2_CEIL() macro computes the smallest power of two greater than
 * or equal to a non-constant runtime input. Passing proves the macro evaluates
 * correctly when its argument is not a compile-time constant, complementing the
 * BUILD_ASSERT-based compile-time checks above.
 *
 * Test steps:
 * - Invoke the helper for representative inputs (1,2,3,4,5,7,8,9) passed through a
 *   volatile variable to defeat constant folding.
 * - Each invocation asserts the returned value equals the expected power of two.
 *
 * Expected result:
 * - Every input rounds up to its expected power-of-two ceiling.
 *
 * @see Z_POW2_CEIL()
 */
ZTEST(pow2, test_pow2_ceil)
{
	test_pow2_ceil_x(1, 1);
	test_pow2_ceil_x(2, 2);
	test_pow2_ceil_x(3, 4);
	test_pow2_ceil_x(4, 4);
	test_pow2_ceil_x(5, 8);
	test_pow2_ceil_x(7, 8);
	test_pow2_ceil_x(8, 8);
	test_pow2_ceil_x(9, 16);
}

extern void *common_setup(void);
ZTEST_SUITE(pow2, NULL, common_setup, NULL, NULL, NULL);
