/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/buf.h>
#include "mocks/settings.h"
#include "mocks/settings_expects.h"

void expect_single_call_bt_settings_encode_key_with_not_null_key(const bt_addr_le_t *addr)
{
	const char *func_name = "bt_settings_encode_key";

	zassert_equal(bt_settings_encode_key_fake.call_count, 1, "'%s()' was called more than once",
		      func_name);
	zassert_not_null(bt_settings_encode_key_fake.arg0_val,
			 "'%s()' was called with incorrect '%s' value", func_name, "path");
	zassert_true(bt_settings_encode_key_fake.arg1_val != 0,
		     "'%s()' was called with incorrect '%s' value", func_name, "path_size");
	zassert_not_null(bt_settings_encode_key_fake.arg2_val,
			 "'%s()' was called with incorrect '%s' value", func_name, "subsys");
	zassert_equal_ptr(bt_settings_encode_key_fake.arg3_val, addr,
			  "'%s()' was called with incorrect '%s' value", func_name, "addr");
	zassert_not_null(bt_settings_encode_key_fake.arg4_val,
			 "'%s()' was called with incorrect '%s' value", func_name, "key");
}

void expect_single_call_bt_settings_encode_key_with_null_key(const bt_addr_le_t *addr)
{
	const char *func_name = "bt_settings_encode_key";

	zassert_equal(bt_settings_encode_key_fake.call_count, 1, "'%s()' was called more than once",
		      func_name);
	zassert_not_null(bt_settings_encode_key_fake.arg0_val,
			 "'%s()' was called with incorrect '%s' value", func_name, "path");
	zassert_true(bt_settings_encode_key_fake.arg1_val != 0,
		     "'%s()' was called with incorrect '%s' value", func_name, "path_size");
	zassert_not_null(bt_settings_encode_key_fake.arg2_val,
			 "'%s()' was called with incorrect '%s' value", func_name, "subsys");
	zassert_equal_ptr(bt_settings_encode_key_fake.arg3_val, addr,
			  "'%s()' was called with incorrect '%s' value", func_name, "addr");
	zassert_equal_ptr(bt_settings_encode_key_fake.arg4_val, NULL,
			  "'%s()' was called with incorrect '%s' value", func_name, "key");
}

void expect_not_called_bt_settings_encode_key(void)
{
	const char *func_name = "bt_settings_encode_key";

	zassert_equal(bt_settings_encode_key_fake.call_count, 0, "'%s()' was called unexpectedly",
		      func_name);
}
