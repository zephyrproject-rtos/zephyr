/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/keys.h"
#include "mocks/keys_expects.h"

#include <zephyr/kernel.h>

void expect_single_call_bt_keys_find_irk(uint8_t id, const bt_addr_le_t *addr)
{
	const char *func_name = "bt_keys_find_irk";

	zassert_equal(bt_keys_find_irk_fake.call_count, 1, "'%s()' was called more than once",
		      func_name);

	zassert_equal(bt_keys_find_irk_fake.arg0_val, id,
		      "'%s()' was called with incorrect '%s' value", func_name, "id");
	zassert_equal(bt_keys_find_irk_fake.arg1_val, addr,
		      "'%s()' was called with incorrect '%s' value", func_name, "addr");
}

void expect_not_called_bt_keys_find_irk(void)
{
	const char *func_name = "bt_keys_find_irk";

	zassert_equal(bt_keys_find_irk_fake.call_count, 0, "'%s()' was called unexpectedly",
		      func_name);
}
