/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fff.h>
#include <psa/crypto.h>

/* List of fakes used by this unit tester */
#define PRNG_FFF_FAKES_LIST(FAKE)         \
		FAKE(psa_crypto_init)                \
		FAKE(psa_generate_random)

DECLARE_FAKE_VALUE_FUNC(psa_status_t, psa_crypto_init);
DECLARE_FAKE_VALUE_FUNC(psa_status_t, psa_generate_random, uint8_t *, size_t);
