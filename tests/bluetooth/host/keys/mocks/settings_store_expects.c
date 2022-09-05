/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/buf.h>
#include "mocks/settings_store.h"
#include "mocks/settings_store_expects.h"

void expect_single_call_settings_delete(void)
{
	const char *func_name = "settings_delete";

	zassert_equal(settings_delete_fake.call_count, 1, "'%s()' was called more than once",
		      func_name);

	zassert_not_null(settings_delete_fake.arg0_val,
			 "'%s()' was called with incorrect '%s' value", func_name, "key");
}
