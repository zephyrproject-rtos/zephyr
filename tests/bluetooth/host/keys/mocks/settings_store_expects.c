/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/buf.h>
#include <host/keys.h>
#include "mocks/settings_store.h"
#include "mocks/settings_store_expects.h"

void expect_single_call_bt_settings_delete_keys(void)
{
	const char *func_name = "bt_settings_delete_keys";

	zassert_equal(bt_settings_delete_keys_fake.call_count, 1,
		      "'%s()' was called more than once", func_name);
}

void expect_single_call_bt_settings_store_keys(const void *value)
{
	const char *func_name = "bt_settings_store_keys";

	zassert_equal(bt_settings_store_keys_fake.call_count, 1, "'%s()' was called more than once",
		      func_name);

	zassert_equal(bt_settings_store_keys_fake.arg2_val, value,
		      "'%s()' was called with incorrect '%s' value", func_name, "value");
	zassert_equal(bt_settings_store_keys_fake.arg3_val, BT_KEYS_STORAGE_LEN,
		      "'%s()' was called with incorrect '%s' value", func_name, "val_len");
}

void expect_not_called_bt_settings_store_keys(void)
{
	const char *func_name = "bt_settings_store_keys";

	zassert_equal(bt_settings_store_keys_fake.call_count, 0, "'%s()' was called unexpectedly",
		      func_name);
}
