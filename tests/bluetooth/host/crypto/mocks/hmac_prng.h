/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fff.h>
#include <tinycrypt/hmac_prng.h>
#include <tinycrypt/constants.h>

/* List of fakes used by this unit tester */
#define HMAC_PRNG_FFF_FAKES_LIST(FAKE)         \
		FAKE(tc_hmac_prng_init)                \
		FAKE(tc_hmac_prng_reseed)              \
		FAKE(tc_hmac_prng_generate)

DECLARE_FAKE_VALUE_FUNC(int, tc_hmac_prng_init, TCHmacPrng_t, const uint8_t *, unsigned int);
DECLARE_FAKE_VALUE_FUNC(int, tc_hmac_prng_reseed, TCHmacPrng_t, const uint8_t *, unsigned int,
			const uint8_t *, unsigned int);
DECLARE_FAKE_VALUE_FUNC(int, tc_hmac_prng_generate, uint8_t *, unsigned int, TCHmacPrng_t);
