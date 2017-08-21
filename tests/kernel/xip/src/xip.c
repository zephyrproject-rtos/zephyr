/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * DESCRIPTION
 * This module tests that XIP performs as expected. If the first
 * task is even activated that is a good indication that XIP is
 * working. However, the test does do some some testing on
 * global variables for completeness sake.
 */

#include <zephyr.h>
#include <tc_util.h>
#include "test.h"
#include <ztest.h>

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
