/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/rpa.h"
#include "mocks/rpa_expects.h"

#include <zephyr/kernel.h>

void expect_single_call_bt_rpa_create(const uint8_t irk[16])
{
	const char *func_name = "bt_rpa_create";

	zassert_equal(bt_rpa_create_fake.call_count, 1, "'%s()' was called more than once",
		      func_name);
	zassert_equal(bt_rpa_create_fake.arg0_val, irk,
		      "'%s()' was called with incorrect '%s' value", func_name, "irk");
	zassert_not_null(bt_rpa_create_fake.arg1_val, "'%s()' was called with incorrect '%s' value",
			 func_name, "rpa");
}

void expect_not_called_bt_rpa_create(void)
{
	const char *func_name = "bt_rpa_create";

	zassert_equal(bt_rpa_create_fake.call_count, 0, "'%s()' was called unexpectedly",
		      func_name);
}
