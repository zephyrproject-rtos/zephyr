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

/**
 * @brief Verify compile-time constant results
 *
 * @ingroup test_pow2_ceil
 *
 * @details Check if static array allocations are sized as expected.
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

/**
 * @brief Verify run-time non-constant results
 *
 * @ingroup test_pow2_ceil
 *
 * @details Check if run-time non-constant results are as expected.
 *          Use a volatile variable to prevent compiler optimizations.
 */

static void test_pow2_ceil_x(unsigned long test_value,
			     unsigned long expected_result)
{
	volatile unsigned int x = test_value;
	unsigned int result = Z_POW2_CEIL(x);

	zassert_equal(result, expected_result,
		      "ZPOW2_CEIL(%lu) returned %lu, expected %lu",
		      test_value, result, expected_result);
}

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
