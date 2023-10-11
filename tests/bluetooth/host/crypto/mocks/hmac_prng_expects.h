/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/*
 *  Validate expected behaviour when tc_hmac_prng_init() is called
 *
 *  Expected behaviour:
 *   - tc_hmac_prng_init() to be called once with correct parameters
 */
void expect_single_call_tc_hmac_prng_init(TCHmacPrng_t prng, unsigned int plen);

/*
 *  Validate expected behaviour when tc_hmac_prng_reseed() is called
 *
 *  Expected behaviour:
 *   - tc_hmac_prng_reseed() to be called once with correct parameters
 */
void expect_single_call_tc_hmac_prng_reseed(TCHmacPrng_t prng, unsigned int seedlen,
					    unsigned int additionallen);

/*
 *  Validate expected behaviour when tc_hmac_prng_generate() is called
 *
 *  Expected behaviour:
 *   - tc_hmac_prng_generate() to be called once with correct parameters
 */
void expect_call_count_tc_hmac_prng_generate(int call_count, uint8_t *out, unsigned int outlen,
					      TCHmacPrng_t prng);
