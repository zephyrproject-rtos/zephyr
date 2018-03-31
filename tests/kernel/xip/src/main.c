/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file
 *
 * This module tests that XIP performs as expected. If the first
 * task is even activated that is a good indication that XIP is
 * working. However, the test does do some some testing on
 * global variables for completeness sake.
 */

#include <zephyr.h>
#include <ztest.h>
#include "test.h"

/**
 *
 * @brief Regression test's entry point
 *
 * @return N/A
 */

void test_globals(void)
{
	int  i;

	/* Array should be filled with monotomically incrementing values */
	for (i = 0; i < XIP_TEST_ARRAY_SZ; i++) {

		/**TESTPOINT: Check if the array value is correct*/
		zassert_equal(xip_array[i], (i+1), "Array value is incorrect");
	}
}

/**test case main entry*/
void test_main(void)
{
	ztest_test_suite(xip,
		ztest_unit_test(test_globals));
	ztest_run_test_suite(xip);
}
