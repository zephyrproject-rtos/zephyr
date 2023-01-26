/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/crypto.h"
#include "mocks/crypto_expects.h"

#include <zephyr/kernel.h>

void expect_single_call_bt_rand(void *buf, size_t len)
{
	const char *func_name = "bt_rand";

	zassert_equal(bt_rand_fake.call_count, 1, "'%s()' was called more than once", func_name);

	zassert_equal(bt_rand_fake.arg0_val, buf, "'%s()' was called with incorrect '%s' value",
		      func_name, "buf");
	zassert_equal(bt_rand_fake.arg1_val, len, "'%s()' was called with incorrect '%s' value",
		      func_name, "len");
}

void expect_not_called_bt_rand(void)
{
	const char *func_name = "bt_rand";

	zassert_equal(bt_rand_fake.call_count, 0, "'%s()' was called unexpectedly", func_name);
}
