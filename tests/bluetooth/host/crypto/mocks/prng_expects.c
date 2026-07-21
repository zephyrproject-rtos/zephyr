/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "mocks/prng.h"
#include "mocks/prng_expects.h"

void expect_single_call_tc_psa_crypto_init(void)
{
	const char *func_name = "psa_crypto_init";

	zassert_equal(psa_crypto_init_fake.call_count, 1, "'%s()' was called more than once",
		      func_name);
}

void expect_single_call_psa_generate_random(uint8_t *out, size_t outlen)
{
	const char *func_name = "psa_generate_random";

	zassert_equal(psa_generate_random_fake.call_count, 1,
		      "'%s()' was called more than once", func_name);

	zassert_equal_ptr(psa_generate_random_fake.arg0_val, out,
			  "'%s()' was called with incorrect '%s' value", func_name, "out");
	zassert_equal(psa_generate_random_fake.arg1_val, outlen,
		      "'%s()' was called with incorrect '%s' value", func_name, "outlen");
}
