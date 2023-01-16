/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "mocks/aes.h"
#include "mocks/aes_expects.h"

void expect_single_call_tc_aes_encrypt(uint8_t *out)
{
	const char *func_name = "tc_aes_encrypt";

	zassert_equal(tc_aes_encrypt_fake.call_count, 1, "'%s()' was called more than once",
		      func_name);

	zassert_equal_ptr(tc_aes_encrypt_fake.arg0_val, out,
		      "'%s()' was called with incorrect '%s' value", func_name, "out");
	zassert_not_null(tc_aes_encrypt_fake.arg1_val,
			 "'%s()' was called with incorrect '%s' value", func_name, "in");
	zassert_not_null(tc_aes_encrypt_fake.arg2_val,
			 "'%s()' was called with incorrect '%s' value", func_name, "s");
}
