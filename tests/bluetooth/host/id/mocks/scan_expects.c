/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/scan.h"
#include "mocks/scan_expects.h"

#include <zephyr/kernel.h>

void expect_call_count_bt_le_scan_set_enable(int call_count, uint8_t args_history[])
{
	const char *func_name = "bt_le_scan_set_enable";

	zassert_equal(bt_le_scan_set_enable_fake.call_count, call_count,
		      "'%s()' was called more than once", func_name);

	for (size_t i = 0; i < call_count; i++) {
		zassert_equal(bt_le_scan_set_enable_fake.arg0_history[i], args_history[i],
			      "'%s()' was called with incorrect '%s' value", func_name, "enable");
	}
}

void expect_not_called_bt_le_scan_set_enable(void)
{
	const char *func_name = "bt_le_scan_set_enable";

	zassert_equal(bt_le_scan_set_enable_fake.call_count, 0, "'%s()' was called unexpectedly",
		      func_name);
}
