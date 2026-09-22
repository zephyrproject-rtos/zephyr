/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "mocks/aes.h"
#include "mocks/aes_expects.h"

void expect_single_call_psa_cipher_encrypt(uint8_t *out)
{
	const char *func_name = "psa_cipher_encrypt";

	zassert_equal(psa_cipher_encrypt_fake.call_count, 1, "'%s()' was called more than once",
		      func_name);

	zassert_not_equal(psa_cipher_encrypt_fake.arg1_val, 0,
			 "'%s()' was called with incorrect '%s' value", func_name, "arg1");
	zassert_not_equal(psa_cipher_encrypt_fake.arg3_val, 0,
			 "'%s()' was called with incorrect '%s' value", func_name, "arg3");
	zassert_equal_ptr(psa_cipher_encrypt_fake.arg4_val, out,
			 "'%s()' was called with incorrect '%s' value", func_name, "arg4");
	zassert_not_equal(psa_cipher_encrypt_fake.arg5_val, 0,
			 "'%s()' was called with incorrect '%s' value", func_name, "arg5");
}
