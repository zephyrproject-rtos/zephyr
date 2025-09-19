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

ZTEST_USER(test_mbedtls_psa, test_generate_random)
{
	uint8_t tmp[64];
	psa_status_t status;

	status = psa_generate_random(tmp, sizeof(tmp));
	zassert_equal(status, PSA_SUCCESS);
}

ZTEST_USER(test_mbedtls_psa, test_md5)
{
	uint8_t in_buf[] = { 'a' };
	uint8_t out_buf[PSA_HASH_LENGTH(PSA_ALG_MD5)] = { 0 };
	uint8_t out_buf_ref[PSA_HASH_LENGTH(PSA_ALG_MD5)] = {
		0x0c, 0xc1, 0x75, 0xb9, 0xc0, 0xf1, 0xb6, 0xa8,
		0x31, 0xc3, 0x99, 0xe2, 0x69, 0x77, 0x26, 0x61
	};
	size_t out_len;
	psa_status_t status;

	status = psa_hash_compute(PSA_ALG_MD5, in_buf, sizeof(in_buf),
				  out_buf, sizeof(out_buf), &out_len);
	zassert_equal(status, PSA_SUCCESS);
	zassert_mem_equal(out_buf, out_buf_ref, sizeof(out_buf_ref));
}

ZTEST_USER(test_mbedtls_psa, test_sha1)
{
	uint8_t in_buf[] = { 'a' };
	uint8_t out_buf[PSA_HASH_LENGTH(PSA_ALG_SHA_1)] = { 0 };
	uint8_t out_buf_ref[PSA_HASH_LENGTH(PSA_ALG_SHA_1)] = {
		0x86, 0xf7, 0xe4, 0x37, 0xfa, 0xa5, 0xa7, 0xfc, 0xe1, 0x5d,
		0x1d, 0xdc, 0xb9, 0xea, 0xea, 0xea, 0x37, 0x76, 0x67, 0xb8
	};
	size_t out_len;
	psa_status_t status;

	status = psa_hash_compute(PSA_ALG_SHA_1, in_buf, sizeof(in_buf),
				  out_buf, sizeof(out_buf), &out_len);
	zassert_equal(status, PSA_SUCCESS);
	zassert_mem_equal(out_buf, out_buf_ref, sizeof(out_buf_ref));
}

ZTEST_SUITE(test_mbedtls_psa, NULL, NULL, NULL, NULL, NULL);
