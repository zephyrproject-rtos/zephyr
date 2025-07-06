/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Test psa_crypto_init() and psa_generate_random() on the PSA implementation
 * provided by Mbed TLS (platforms using TFM are filtered out in the yaml file).
 */

#include <zephyr/ztest.h>

#include <psa/crypto.h>

ZTEST_USER(test_fn, test_mbedtls_psa)
{
	uint8_t tmp[64];

	zassert_equal(psa_crypto_init(), PSA_SUCCESS, "psa_crypto_init failed");
	zassert_equal(psa_generate_random(tmp, sizeof(tmp)), PSA_SUCCESS,
					"psa_generate_random failed");

}

ZTEST_SUITE(test_fn, NULL, NULL, NULL, NULL, NULL);
