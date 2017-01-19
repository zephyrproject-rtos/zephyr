/* test_xip_helper.c - XIP helper module */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
DESCRIPTION
This module contains support code for the XIP regression
test.
 */

#include <zephyr.h>
#include "test.h"

/*
 * This array is deliberately defined outside of the scope of the main test
 * module to avoid optimization issues.
 */
uint32_t xip_array[XIP_TEST_ARRAY_SZ] = {
	TEST_VAL_1, TEST_VAL_2, TEST_VAL_3, TEST_VAL_4};
