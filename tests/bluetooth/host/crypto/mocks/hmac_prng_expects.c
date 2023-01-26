/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "mocks/hmac_prng.h"
#include "mocks/hmac_prng_expects.h"

void expect_single_call_tc_hmac_prng_init(TCHmacPrng_t prng, unsigned int plen)
{
	const char *func_name = "tc_hmac_prng_init";

	zassert_equal(tc_hmac_prng_init_fake.call_count, 1, "'%s()' was called more than once",
		      func_name);

	zassert_equal_ptr(tc_hmac_prng_init_fake.arg0_val, prng,
		      "'%s()' was called with incorrect '%s' value", func_name, "prng");
	zassert_not_null(tc_hmac_prng_init_fake.arg1_val,
			 "'%s()' was called with incorrect '%s' value", func_name, "buffer");
	zassert_equal(tc_hmac_prng_init_fake.arg2_val, plen,
		      "'%s()' was called with incorrect '%s' value", func_name, "plen");
}

void expect_single_call_tc_hmac_prng_reseed(TCHmacPrng_t prng, unsigned int seedlen,
					    unsigned int additionallen)
{
	const char *func_name = "tc_hmac_prng_reseed";

	zassert_equal(tc_hmac_prng_reseed_fake.call_count, 1, "'%s()' was called more than once",
		      func_name);

	zassert_equal_ptr(tc_hmac_prng_reseed_fake.arg0_val, prng,
			  "'%s()' was called with incorrect '%s' value", func_name, "prng");
	zassert_not_null(tc_hmac_prng_reseed_fake.arg1_val,
			 "'%s()' was called with incorrect '%s' value", func_name, "seed");
	zassert_equal(tc_hmac_prng_reseed_fake.arg2_val, seedlen,
		      "'%s()' was called with incorrect '%s' value", func_name, "seedlen");
	zassert_not_null(tc_hmac_prng_reseed_fake.arg3_val,
			 "'%s()' was called with incorrect '%s' value", func_name,
			 "additional_input");
	zassert_equal(tc_hmac_prng_reseed_fake.arg4_val, additionallen,
		      "'%s()' was called with incorrect '%s' value", func_name, "additionallen");
}

void expect_call_count_tc_hmac_prng_generate(int call_count, uint8_t *out, unsigned int outlen,
					      TCHmacPrng_t prng)
{
	const char *func_name = "tc_hmac_prng_generate";

	zassert_equal(tc_hmac_prng_generate_fake.call_count, call_count,
		      "'%s()' was called more than once", func_name);

	zassert_equal_ptr(tc_hmac_prng_generate_fake.arg0_val, out,
			  "'%s()' was called with incorrect '%s' value", func_name, "out");
	zassert_equal(tc_hmac_prng_generate_fake.arg1_val, outlen,
		      "'%s()' was called with incorrect '%s' value", func_name, "outlen");
	zassert_equal_ptr(tc_hmac_prng_generate_fake.arg2_val, prng,
			  "'%s()' was called with incorrect '%s' value", func_name, "prng");
}
