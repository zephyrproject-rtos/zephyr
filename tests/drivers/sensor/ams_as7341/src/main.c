/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Dotcom IoT LLP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/sensor.h>

ZTEST_SUITE(ams_as7341_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(ams_as7341_tests, test_basic_assertions)
{
	/* Add your sensor driver test cases here */
	zassert_true(1, "basic assertion failed");
}
