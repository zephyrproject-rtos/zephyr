/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/buf.h>
#include "mocks/util.h"
#include "mocks/util_expects.h"

void expect_single_call_u8_to_dec(uint8_t value)
{
	const char *func_name = "u8_to_dec";

	zassert_equal(u8_to_dec_fake.call_count, 1, "'%s()' was called more than once",
		      func_name);

	zassert_not_null(u8_to_dec_fake.arg0_val, "'%s()' was called with incorrect '%s' value",
			 func_name, "buf");
	zassert_true(u8_to_dec_fake.arg1_val != 0, "'%s()' was called with incorrect '%s' value",
		     func_name, "buflen");
	zassert_equal(u8_to_dec_fake.arg2_val, value, "'%s()' was called with incorrect '%s' value",
		      func_name, "value");
}

void expect_not_called_u8_to_dec(void)
{
	const char *func_name = "u8_to_dec";

	zassert_equal(u8_to_dec_fake.call_count, 0, "'%s()' was called unexpectedly",
		      func_name);
}
