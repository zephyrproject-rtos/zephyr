/*
 * Copyright (c) 2016 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <errno.h>
#include <tc_util.h>
#include <ztest.h>

void test_multilib(void)
{
	volatile long long a = 100;
	volatile long long b = 3;
	volatile long long c = a / b;

	TC_START("test_multilib");

	/**TESTPOINT: Check if the correct multilib is selected*/
	zassert_equal(c, 33, "smoke-test failed: wrong multilib selected");
}
