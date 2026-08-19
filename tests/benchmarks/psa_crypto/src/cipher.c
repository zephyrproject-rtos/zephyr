/*
 * Copyright (C) 2025, BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <psa/crypto.h>

#define BUF_SIZE 1024

static uint8_t in_buf[BUF_SIZE];
static uint8_t out_buf[BUF_SIZE];
static psa_key_id_t key_id;

static void suite_setup(void)
{
	memset(in_buf, 0xaa, sizeof(in_buf));
}

ZTEST_BENCHMARK_SUITE(psa_crypto_cipher, suite_setup, NULL);

static void setup_key(psa_key_type_t key_type)
{
	uint8_t tmp_key[32] = {0x5};
	psa_key_attributes_t key_attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_type(&key_attr, key_type);
	psa_set_key_algorithm(&key_attr, PSA_ALG_ECB_NO_PADDING);
	psa_set_key_usage_flags(&key_attr, PSA_KEY_USAGE_ENCRYPT);
	psa_import_key(&key_attr, tmp_key, sizeof(tmp_key), &key_id);
}

static void teardown_key(void)
{
	psa_destroy_key(key_id);
}

static void setup_aes_256(void)
{
	setup_key(PSA_KEY_TYPE_AES);
}

ZTEST_BENCHMARK_TIMED(psa_crypto_cipher, aes_256_ecb, 1000U, setup_aes_256, teardown_key)
{
	size_t out_len;

	psa_cipher_encrypt(key_id, PSA_ALG_ECB_NO_PADDING,
			   in_buf, sizeof(in_buf),
			   out_buf, sizeof(out_buf), &out_len);
}

static void setup_aria_256(void)
{
	setup_key(PSA_KEY_TYPE_ARIA);
}

ZTEST_BENCHMARK_TIMED(psa_crypto_cipher, aria_256_ecb, 1000U, setup_aria_256, teardown_key)
{
	size_t out_len;

	psa_cipher_encrypt(key_id, PSA_ALG_ECB_NO_PADDING,
			   in_buf, sizeof(in_buf),
			   out_buf, sizeof(out_buf), &out_len);
}

static void setup_camellia_256(void)
{
	setup_key(PSA_KEY_TYPE_CAMELLIA);
}

ZTEST_BENCHMARK_TIMED(psa_crypto_cipher, camellia_256_ecb, 1000U, setup_camellia_256, teardown_key)
{
	size_t out_len;

	psa_cipher_encrypt(key_id, PSA_ALG_ECB_NO_PADDING,
			   in_buf, sizeof(in_buf),
			   out_buf, sizeof(out_buf), &out_len);
}
