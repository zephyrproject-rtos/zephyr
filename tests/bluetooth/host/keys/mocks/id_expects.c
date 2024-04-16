/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/buf.h>
#include "mocks/id.h"
#include "mocks/id_expects.h"

void expect_single_call_bt_id_del(struct bt_keys *keys)
{
	const char *func_name = "bt_id_del";

	zassert_equal(bt_id_del_fake.call_count, 1, "'%s()' was called more than once",
		      func_name);

	zassert_equal(bt_id_del_fake.arg0_val, keys,
			  "'%s()' was called with incorrect '%s' value", func_name, "keys");
}

void expect_not_called_bt_id_del(void)
{
	const char *func_name = "bt_id_del";

	zassert_equal(bt_id_del_fake.call_count, 0, "'%s()' was called unexpectedly",
		      func_name);
}
