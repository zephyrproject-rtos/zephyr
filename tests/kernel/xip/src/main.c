/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Test to verify XIP
 *
 * @defgroup kernel_xip_tests XIP Tests
 *
 * @ingroup all_tests
 *
 * @details This module tests that XIP performs as expected. If the first
 * task is even activated that is a good indication that XIP is
 * working. However, the test does do some some testing on
 * global variables for completeness sake.
 *
 * @{
 * @}
 */

#include <zephyr/ztest.h>

/* This test relies on these values being one larger than the one before */
#define TEST_VAL_1  0x1
#define TEST_VAL_2  0x2
#define TEST_VAL_3  0x3
#define TEST_VAL_4  0x4

#define XIP_TEST_ARRAY_SZ 4

extern uint32_t xip_array[XIP_TEST_ARRAY_SZ];

/*
 * This array is deliberately defined outside of the scope of the main test
 * module to avoid optimization issues.
 */
uint32_t xip_array[XIP_TEST_ARRAY_SZ] = {
	TEST_VAL_1, TEST_VAL_2, TEST_VAL_3, TEST_VAL_4};

/**
 * @brief Test XIP
 *
 * @ingroup kernel_xip_tests
 */
ZTEST(xip, test_globals)
{
	int  i;

	/* Array should be filled with monotonically incrementing values */
	for (i = 0; i < XIP_TEST_ARRAY_SZ; i++) {

		/**TESTPOINT: Check if the array value is correct*/
		zassert_equal(xip_array[i], (i+1), "Array value is incorrect");
	}
}

/**test case main entry*/
ZTEST_SUITE(xip, NULL, NULL, NULL, NULL, NULL);
