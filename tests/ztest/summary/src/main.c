/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

ZTEST(suite1, test_foo)
{
	ztest_test_pass();
}

ZTEST(suite2, test_foo)
{
	ztest_test_skip();
}

ZTEST_SUITE(suite1, NULL, NULL, NULL, NULL, NULL);
ZTEST_SUITE(suite2, NULL, NULL, NULL, NULL, NULL);
