/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>

ZTEST_SUITE(test_pytest, NULL, NULL, NULL, NULL, NULL);

ZTEST(test_pytest, test_pytest)
{
	TC_PRINT("Hello world\n");
}
