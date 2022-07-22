/*
 * Copyright (c) 2020 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <ztest.h>

ZTEST_USER(arm64_high_addresses, test_arm64_high_addresses)
{
	ztest_test_pass();
}

ZTEST_SUITE(arm64_high_addresses, NULL, NULL, NULL, NULL, NULL);
