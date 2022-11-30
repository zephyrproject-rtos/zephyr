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
