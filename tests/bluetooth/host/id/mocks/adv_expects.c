/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/adv.h"
#include "mocks/adv_expects.h"

#include <zephyr/kernel.h>

void expect_single_call_bt_le_adv_lookup_legacy(void)
{
	const char *func_name = "bt_le_adv_lookup_legacy";

	zassert_equal(bt_le_adv_lookup_legacy_fake.call_count, 1,
		      "'%s()' was called more than once", func_name);
}

void expect_not_called_bt_le_adv_lookup_legacy(void)
{
	const char *func_name = "bt_le_adv_lookup_legacy";

	zassert_equal(bt_le_adv_lookup_legacy_fake.call_count, 0, "'%s()' was called unexpectedly",
		      func_name);
}

void expect_single_call_bt_le_ext_adv_foreach(void)
{
	const char *func_name = "bt_le_ext_adv_foreach";

	zassert_equal(bt_le_ext_adv_foreach_fake.call_count, 1, "'%s()' was called more than once",
		      func_name);

	zassume_not_null(bt_le_ext_adv_foreach_fake.arg0_val,
			 "'%s()' was called with incorrect '%s' value", func_name, "func");
}

void expect_not_called_bt_le_ext_adv_foreach(void)
{
	const char *func_name = "bt_le_ext_adv_foreach";

	zassert_equal(bt_le_ext_adv_foreach_fake.call_count, 0, "'%s()' was called unexpectedly",
		      func_name);
}
