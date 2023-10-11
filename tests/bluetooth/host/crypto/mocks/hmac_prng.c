/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "mocks/hmac_prng.h"

DEFINE_FAKE_VALUE_FUNC(int, tc_hmac_prng_init, TCHmacPrng_t, const uint8_t *, unsigned int);
DEFINE_FAKE_VALUE_FUNC(int, tc_hmac_prng_reseed, TCHmacPrng_t, const uint8_t *, unsigned int,
		       const uint8_t *, unsigned int);
DEFINE_FAKE_VALUE_FUNC(int, tc_hmac_prng_generate, uint8_t *, unsigned int, TCHmacPrng_t);
