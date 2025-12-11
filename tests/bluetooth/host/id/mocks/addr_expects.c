/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/addr.h"
#include "mocks/addr_expects.h"

#include <zephyr/kernel.h>

void expect_call_count_bt_addr_le_create_static(int call_count)
{
	const char *func_name = "bt_addr_le_create_static";

	zassert_equal(bt_addr_le_create_static_fake.call_count, call_count,
		      "'%s()' was called more than once", func_name);

	zassert_not_null(bt_addr_le_create_static_fake.arg0_val,
			 "'%s()' was called with incorrect '%s' value", func_name, "buf");
}

void expect_not_called_bt_addr_le_create_static(void)
{
	const char *func_name = "bt_addr_le_create_static";

	zassert_equal(bt_addr_le_create_static_fake.call_count, 0, "'%s()' was called unexpectedly",
		      func_name);
}
