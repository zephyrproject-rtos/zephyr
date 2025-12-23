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

ZTEST_USER(test_mbedtls_psa, test_sha224)
{
	uint8_t in_buf[] = { 'a' };
	uint8_t out_buf[PSA_HASH_LENGTH(PSA_ALG_SHA_224)] = { 0 };
	uint8_t out_buf_ref[PSA_HASH_LENGTH(PSA_ALG_SHA_224)] = {
		0xab, 0xd3, 0x75, 0x34, 0xc7, 0xd9, 0xa2, 0xef, 0xb9, 0x46,
		0x5d, 0xe9, 0x31, 0xcd, 0x70, 0x55, 0xff, 0xdb, 0x88, 0x79,
		0x56, 0x3a, 0xe9, 0x80, 0x78, 0xd6, 0xd6, 0xd5
	};
	size_t out_len;
	psa_status_t status;

	status = psa_hash_compute(PSA_ALG_SHA_224, in_buf, sizeof(in_buf),
				  out_buf, sizeof(out_buf), &out_len);
	zassert_equal(status, PSA_SUCCESS);
	zassert_mem_equal(out_buf, out_buf_ref, sizeof(out_buf_ref));
}

ZTEST_USER(test_mbedtls_psa, test_sha256)
{
	uint8_t in_buf[] = { 'a' };
	uint8_t out_buf[PSA_HASH_LENGTH(PSA_ALG_SHA_256)] = { 0 };
	uint8_t out_buf_ref[PSA_HASH_LENGTH(PSA_ALG_SHA_256)] = {
		0xca, 0x97, 0x81, 0x12, 0xca, 0x1b, 0xbd, 0xca, 0xfa, 0xc2,
		0x31, 0xb3, 0x9a, 0x23, 0xdc, 0x4d, 0xa7, 0x86, 0xef, 0xf8,
		0x14, 0x7c, 0x4e, 0x72, 0xb9, 0x80, 0x77, 0x85, 0xaf, 0xee,
		0x48, 0xbb
	};
	size_t out_len;
	psa_status_t status;

	status = psa_hash_compute(PSA_ALG_SHA_256, in_buf, sizeof(in_buf),
				  out_buf, sizeof(out_buf), &out_len);
	zassert_equal(status, PSA_SUCCESS);
	zassert_mem_equal(out_buf, out_buf_ref, sizeof(out_buf_ref));
}

ZTEST_USER(test_mbedtls_psa, test_sha384)
{
	uint8_t in_buf[] = { 'a' };
	uint8_t out_buf[PSA_HASH_LENGTH(PSA_ALG_SHA_384)] = { 0 };
	uint8_t out_buf_ref[PSA_HASH_LENGTH(PSA_ALG_SHA_384)] = {
		0x54, 0xa5, 0x9b, 0x9f, 0x22, 0xb0, 0xb8, 0x08, 0x80, 0xd8,
		0x42, 0x7e, 0x54, 0x8b, 0x7c, 0x23, 0xab, 0xd8, 0x73, 0x48,
		0x6e, 0x1f, 0x03, 0x5d, 0xce, 0x9c, 0xd6, 0x97, 0xe8, 0x51,
		0x75, 0x03, 0x3c, 0xaa, 0x88, 0xe6, 0xd5, 0x7b, 0xc3, 0x5e,
		0xfa, 0xe0, 0xb5, 0xaf, 0xd3, 0x14, 0x5f, 0x31
	};
	size_t out_len;
	psa_status_t status;

	status = psa_hash_compute(PSA_ALG_SHA_384, in_buf, sizeof(in_buf),
				  out_buf, sizeof(out_buf), &out_len);
	zassert_equal(status, PSA_SUCCESS);
	zassert_mem_equal(out_buf, out_buf_ref, sizeof(out_buf_ref));
}

ZTEST_USER(test_mbedtls_psa, test_sha512)
{
	uint8_t in_buf[] = { 'a' };
	uint8_t out_buf[PSA_HASH_LENGTH(PSA_ALG_SHA_512)] = { 0 };
	uint8_t out_buf_ref[PSA_HASH_LENGTH(PSA_ALG_SHA_512)] = {
		0x1f, 0x40, 0xfc, 0x92, 0xda, 0x24, 0x16, 0x94, 0x75, 0x09,
		0x79, 0xee, 0x6c, 0xf5, 0x82, 0xf2, 0xd5, 0xd7, 0xd2, 0x8e,
		0x18, 0x33, 0x5d, 0xe0, 0x5a, 0xbc, 0x54, 0xd0, 0x56, 0x0e,
		0x0f, 0x53, 0x02, 0x86, 0x0c, 0x65, 0x2b, 0xf0, 0x8d, 0x56,
		0x02, 0x52, 0xaa, 0x5e, 0x74, 0x21, 0x05, 0x46, 0xf3, 0x69,
		0xfb, 0xbb, 0xce, 0x8c, 0x12, 0xcf, 0xc7, 0x95, 0x7b, 0x26,
		0x52, 0xfe, 0x9a, 0x75
	};
	size_t out_len;
	psa_status_t status;

	status = psa_hash_compute(PSA_ALG_SHA_512, in_buf, sizeof(in_buf),
				  out_buf, sizeof(out_buf), &out_len);
	zassert_equal(status, PSA_SUCCESS);
	zassert_mem_equal(out_buf, out_buf_ref, sizeof(out_buf_ref));
}

ZTEST_USER(test_mbedtls_psa, test_hmac_sha256)
{
	uint8_t key[] = { 'a' };
	psa_key_attributes_t key_attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	uint8_t in_buf[] = { 'a' };
	uint8_t out_buf[PSA_HASH_LENGTH(PSA_ALG_SHA_256)] = { 0 };
	uint8_t out_buf_ref[PSA_HASH_LENGTH(PSA_ALG_SHA_256)] = {
		0x3e, 0xcf, 0x53, 0x88, 0xe2, 0x20, 0xda, 0x9e,
		0x0f, 0x91, 0x94, 0x85, 0xde, 0xb6, 0x76, 0xd8,
		0xbe, 0xe3, 0xae, 0xc0, 0x46, 0xa7, 0x79, 0x35,
		0x3b, 0x46, 0x34, 0x18, 0x51, 0x1e, 0xe6, 0x22
	};
	size_t out_len;
	psa_status_t status;

	psa_set_key_type(&key_attr, PSA_KEY_TYPE_HMAC);
	psa_set_key_algorithm(&key_attr, PSA_ALG_HMAC(PSA_ALG_SHA_256));
	psa_set_key_usage_flags(&key_attr, PSA_KEY_USAGE_SIGN_MESSAGE);

	status = psa_import_key(&key_attr, key, sizeof(key), &key_id);
	zassert_equal(status, PSA_SUCCESS);

	status = psa_mac_compute(key_id, PSA_ALG_HMAC(PSA_ALG_SHA_256),
				 in_buf, sizeof(in_buf),
				 out_buf, sizeof(out_buf), &out_len);
	zassert_equal(status, PSA_SUCCESS);
	zassert_mem_equal(out_buf, out_buf_ref, sizeof(out_buf_ref));

	status = psa_destroy_key(key_id);
	zassert_equal(status, PSA_SUCCESS);
}

ZTEST_USER(test_mbedtls_psa, test_aes_ecb)
{
	uint8_t key[] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
			 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf};
	psa_key_attributes_t key_attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	uint8_t in_buf[PSA_BLOCK_CIPHER_BLOCK_LENGTH(PSA_KEY_TYPE_AES)];
#define AES_ENCRYPTED_OUTPUT_SIZE \
	PSA_CIPHER_ENCRYPT_OUTPUT_SIZE(PSA_KEY_TYPE_AES, PSA_ALG_ECB_NO_PADDING, sizeof(in_buf))
	uint8_t out_buf[AES_ENCRYPTED_OUTPUT_SIZE] = { 0 };
	uint8_t out_buf_ref[AES_ENCRYPTED_OUTPUT_SIZE] = {
		0xea, 0x5e, 0x61, 0xae, 0x81, 0x67, 0xca, 0xa0,
		0x58, 0x63, 0x88, 0xeb, 0x9a, 0x7c, 0xb7, 0x55
	};
	size_t out_len;
	psa_status_t status;

	memset(in_buf, 0x5, sizeof(in_buf));

	psa_set_key_type(&key_attr, PSA_KEY_TYPE_AES);
	psa_set_key_algorithm(&key_attr, PSA_ALG_ECB_NO_PADDING);
	psa_set_key_usage_flags(&key_attr, PSA_KEY_USAGE_ENCRYPT);
	status = psa_import_key(&key_attr, key, sizeof(key), &key_id);
	zassert_equal(status, PSA_SUCCESS);

	status = psa_cipher_encrypt(key_id, PSA_ALG_ECB_NO_PADDING, in_buf, sizeof(in_buf),
				    out_buf, sizeof(out_buf), &out_len);
	zassert_equal(status, PSA_SUCCESS);

	zassert_mem_equal(out_buf, out_buf_ref, sizeof(out_buf_ref));

	status = psa_destroy_key(key_id);
	zassert_equal(status, PSA_SUCCESS);
}

ZTEST_SUITE(test_mbedtls_psa, NULL, NULL, NULL, NULL, NULL);
