/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Exercise OpenThread platform crypto with contexts allocated by the platform
 * (CONFIG_OPENTHREAD_CRYPTO_PLATFORM_ALLOCS_CONTEXT).
 */

#include <stdint.h>

#include <zephyr/ztest.h>

#include <openthread/platform/crypto.h>

#include <psa/crypto.h>

#ifndef CONFIG_OPENTHREAD_CRYPTO_PLATFORM_ALLOCS_CONTEXT
#error Platform-allocated OpenThread crypto contexts are not enabled
#endif

#define CONTEXT_REUSE_ITERATIONS 4U

/* FIPS 180-4 SHA-256("abc") */
static const uint8_t sha256_input[] = { 'a', 'b', 'c' };
static const uint8_t sha256_expected[OT_CRYPTO_SHA256_HASH_SIZE] = {
	0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea, 0x41, 0x41, 0x40,
	0xde, 0x5d, 0xae, 0x22, 0x23, 0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17,
	0x7a, 0x9c, 0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad,
};

/* RFC 4231 HMAC-SHA-256 test case 1 */
static const uint8_t hmac_key[] = {
	0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
	0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
};
static const uint8_t hmac_input[] = {
	'H', 'i', ' ', 'T', 'h', 'e', 'r', 'e',
};
static const uint8_t hmac_expected[OT_CRYPTO_SHA256_HASH_SIZE] = {
	0xb0, 0x34, 0x4c, 0x61, 0xd8, 0xdb, 0x38, 0x53, 0x5c, 0xa8, 0xaf,
	0xce, 0xaf, 0x0b, 0xf1, 0x2b, 0x88, 0x1d, 0xc2, 0x00, 0xc9, 0x83,
	0x3d, 0xa7, 0x26, 0xe9, 0x37, 0x6c, 0x2e, 0x32, 0xcf, 0xf7,
};

/* AES-128 ECB, plaintext 0x05 * 16, key 0x00..0x0f */
static const uint8_t aes_key[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
};
static const uint8_t aes_plain[] = {
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
};
static const uint8_t aes_expected[] = {
	0xea, 0x5e, 0x61, 0xae, 0x81, 0x67, 0xca, 0xa0,
	0x58, 0x63, 0x88, 0xeb, 0x9a, 0x7c, 0xb7, 0x55,
};

/* RFC 5869 HKDF-SHA-256 test case 1, 42-byte OKM */
static const uint8_t hkdf_ikm[] = {
	0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
	0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
};
static const uint8_t hkdf_salt[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
	0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
};
static const uint8_t hkdf_info[] = {
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9,
};
static const uint8_t hkdf_expected[] = {
	0x3c, 0xb2, 0x5f, 0x25, 0xfa, 0xac, 0xd5, 0x7a, 0x90, 0x43, 0x4f,
	0x64, 0xd0, 0x36, 0x2f, 0x2a, 0x2d, 0x2d, 0x0a, 0x90, 0xcf, 0x1a,
	0x5a, 0x4c, 0x5d, 0xb0, 0x2d, 0x56, 0xec, 0xc4, 0xc5, 0xbf, 0x34,
	0x00, 0x72, 0x08, 0xd5, 0xb8, 0x87, 0x18, 0x58, 0x65,
};

#define ASSERT_CONTEXT_ALLOCATED(context, size)                                        \
	do {                                                                           \
		zassert_not_null((context)->mContext);                                \
		zassert_equal((context)->mContextSize, (uint16_t)(size));             \
		zassert_equal((uintptr_t)(context)->mContext % sizeof(uint64_t), 0U); \
	} while (0)

#define ASSERT_CONTEXT_CLEARED(context)                    \
	do {                                               \
		zassert_is_null((context)->mContext);     \
		zassert_equal((context)->mContextSize, 0); \
	} while (0)

static void *suite_setup(void)
{
	otPlatCryptoInit();
	zassert_equal(psa_crypto_init(), PSA_SUCCESS);

	return NULL;
}

static otCryptoKeyRef import_volatile_key(otCryptoKeyType type, otCryptoKeyAlgorithm algorithm,
					  int usage, const uint8_t *key, size_t key_len)
{
	otCryptoKeyRef key_ref = 0U;
	otError err;

	err = otPlatCryptoImportKey(&key_ref, type, algorithm, usage,
				    OT_CRYPTO_KEY_STORAGE_VOLATILE, key, key_len);
	zassert_equal(err, OT_ERROR_NONE);

	return key_ref;
}

ZTEST(openthread_crypto_psa, test_init_null_context)
{
	zassert_equal(otPlatCryptoSha256Init(NULL), OT_ERROR_INVALID_ARGS);
	zassert_equal(otPlatCryptoHmacSha256Init(NULL), OT_ERROR_INVALID_ARGS);
	zassert_equal(otPlatCryptoHkdfInit(NULL), OT_ERROR_INVALID_ARGS);
	zassert_equal(otPlatCryptoAesInit(NULL), OT_ERROR_INVALID_ARGS);
}

ZTEST(openthread_crypto_psa, test_deinit_without_context)
{
	otCryptoContext context = { 0 };

	zassert_equal(otPlatCryptoSha256Deinit(&context), OT_ERROR_INVALID_ARGS);
	zassert_equal(otPlatCryptoHmacSha256Deinit(&context), OT_ERROR_INVALID_ARGS);
	zassert_equal(otPlatCryptoHkdfDeinit(&context), OT_ERROR_INVALID_ARGS);
	zassert_equal(otPlatCryptoAesFree(&context), OT_ERROR_INVALID_ARGS);
	ASSERT_CONTEXT_CLEARED(&context);
}

ZTEST(openthread_crypto_psa, test_sha256_allocated_context)
{
	uint64_t caller_storage;
	otCryptoContext context = {
		.mContext = &caller_storage,
		.mContextSize = sizeof(caller_storage),
	};
	uint8_t digest[OT_CRYPTO_SHA256_HASH_SIZE];

	for (uint8_t i = 0U; i < CONTEXT_REUSE_ITERATIONS; i++) {
		zassert_equal(otPlatCryptoSha256Init(&context), OT_ERROR_NONE);
		zassert_not_equal(context.mContext, &caller_storage);
		ASSERT_CONTEXT_ALLOCATED(&context, sizeof(psa_hash_operation_t));

		zassert_equal(otPlatCryptoSha256Start(&context), OT_ERROR_NONE);
		zassert_equal(otPlatCryptoSha256Update(&context, sha256_input,
						      sizeof(sha256_input)),
			      OT_ERROR_NONE);
		zassert_equal(otPlatCryptoSha256Finish(&context, digest, sizeof(digest)),
			      OT_ERROR_NONE);
		zassert_mem_equal(digest, sha256_expected, sizeof(sha256_expected));

		zassert_equal(otPlatCryptoSha256Deinit(&context), OT_ERROR_NONE);
		ASSERT_CONTEXT_CLEARED(&context);

		zassert_equal(otPlatCryptoSha256Deinit(&context), OT_ERROR_INVALID_ARGS);
	}
}

ZTEST(openthread_crypto_psa, test_hmac_sha256_allocated_context)
{
	otCryptoContext context = { 0 };
	otCryptoKey key = { 0 };
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	psa_status_t status;
	uint8_t mac[OT_CRYPTO_SHA256_HASH_SIZE];

	/*
	 * psa_mac_sign_setup() requires PSA_KEY_USAGE_SIGN_MESSAGE.
	 * otPlatCryptoImportKey() maps OT_CRYPTO_KEY_USAGE_SIGN_HASH onto
	 * PSA_KEY_USAGE_SIGN_HASH, so import this HMAC key through PSA.
	 */
	psa_set_key_type(&attributes, PSA_KEY_TYPE_HMAC);
	psa_set_key_algorithm(&attributes, PSA_ALG_HMAC(PSA_ALG_SHA_256));
	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_MESSAGE);
	status = psa_import_key(&attributes, hmac_key, sizeof(hmac_key), &key_id);
	psa_reset_key_attributes(&attributes);
	zassert_equal(status, PSA_SUCCESS);

	key.mKeyRef = key_id;

	zassert_equal(otPlatCryptoHmacSha256Init(&context), OT_ERROR_NONE);
	ASSERT_CONTEXT_ALLOCATED(&context, sizeof(psa_mac_operation_t));

	zassert_equal(otPlatCryptoHmacSha256Start(&context, &key), OT_ERROR_NONE);
	zassert_equal(otPlatCryptoHmacSha256Update(&context, hmac_input, sizeof(hmac_input)),
		      OT_ERROR_NONE);
	zassert_equal(otPlatCryptoHmacSha256Finish(&context, mac, sizeof(mac)), OT_ERROR_NONE);
	zassert_mem_equal(mac, hmac_expected, sizeof(hmac_expected));

	zassert_equal(otPlatCryptoHmacSha256Deinit(&context), OT_ERROR_NONE);
	ASSERT_CONTEXT_CLEARED(&context);
	zassert_equal(otPlatCryptoHmacSha256Deinit(&context), OT_ERROR_INVALID_ARGS);

	zassert_equal(psa_destroy_key(key_id), PSA_SUCCESS);
}

ZTEST(openthread_crypto_psa, test_hkdf_allocated_context)
{
	otCryptoContext context = { 0 };
	otCryptoKey input_key = { 0 };
	otCryptoKeyRef key_ref;
	uint8_t okm[sizeof(hkdf_expected)];

	key_ref = import_volatile_key(OT_CRYPTO_KEY_TYPE_DERIVE, OT_CRYPTO_KEY_ALG_HKDF_SHA256,
				      OT_CRYPTO_KEY_USAGE_DERIVE, hkdf_ikm, sizeof(hkdf_ikm));
	input_key.mKeyRef = key_ref;

	zassert_equal(otPlatCryptoHkdfInit(&context), OT_ERROR_NONE);
	ASSERT_CONTEXT_ALLOCATED(&context, sizeof(psa_key_derivation_operation_t));

	zassert_equal(otPlatCryptoHkdfExtract(&context, hkdf_salt, sizeof(hkdf_salt), &input_key),
		      OT_ERROR_NONE);
	zassert_equal(otPlatCryptoHkdfExpand(&context, hkdf_info, sizeof(hkdf_info), okm,
					     sizeof(okm)),
		      OT_ERROR_NONE);
	zassert_mem_equal(okm, hkdf_expected, sizeof(hkdf_expected));

	zassert_equal(otPlatCryptoHkdfDeinit(&context), OT_ERROR_NONE);
	ASSERT_CONTEXT_CLEARED(&context);
	zassert_equal(otPlatCryptoHkdfDeinit(&context), OT_ERROR_INVALID_ARGS);

	zassert_equal(otPlatCryptoDestroyKey(key_ref), OT_ERROR_NONE);
}

ZTEST(openthread_crypto_psa, test_aes_allocated_context)
{
	otCryptoContext context = { 0 };
	otCryptoKey key = { 0 };
	otCryptoKeyRef key_ref;
	uint8_t cipher[sizeof(aes_expected)];

	key_ref = import_volatile_key(OT_CRYPTO_KEY_TYPE_AES, OT_CRYPTO_KEY_ALG_AES_ECB,
				      OT_CRYPTO_KEY_USAGE_ENCRYPT, aes_key, sizeof(aes_key));
	key.mKeyRef = key_ref;

	zassert_equal(otPlatCryptoAesInit(&context), OT_ERROR_NONE);
	ASSERT_CONTEXT_ALLOCATED(&context, sizeof(psa_key_id_t));

	zassert_equal(otPlatCryptoAesSetKey(&context, &key), OT_ERROR_NONE);
	zassert_equal(otPlatCryptoAesEncrypt(&context, aes_plain, cipher), OT_ERROR_NONE);
	zassert_mem_equal(cipher, aes_expected, sizeof(aes_expected));

	zassert_equal(otPlatCryptoAesFree(&context), OT_ERROR_NONE);
	ASSERT_CONTEXT_CLEARED(&context);
	zassert_equal(otPlatCryptoAesFree(&context), OT_ERROR_INVALID_ARGS);

	zassert_equal(otPlatCryptoDestroyKey(key_ref), OT_ERROR_NONE);
}

ZTEST(openthread_crypto_psa, test_concurrent_contexts)
{
	otCryptoContext sha = { 0 };
	otCryptoContext hmac = { 0 };
	otCryptoContext hkdf = { 0 };
	otCryptoContext aes = { 0 };

	zassert_equal(otPlatCryptoSha256Init(&sha), OT_ERROR_NONE);
	zassert_equal(otPlatCryptoHmacSha256Init(&hmac), OT_ERROR_NONE);
	zassert_equal(otPlatCryptoHkdfInit(&hkdf), OT_ERROR_NONE);
	zassert_equal(otPlatCryptoAesInit(&aes), OT_ERROR_NONE);

	ASSERT_CONTEXT_ALLOCATED(&sha, sizeof(psa_hash_operation_t));
	ASSERT_CONTEXT_ALLOCATED(&hmac, sizeof(psa_mac_operation_t));
	ASSERT_CONTEXT_ALLOCATED(&hkdf, sizeof(psa_key_derivation_operation_t));
	ASSERT_CONTEXT_ALLOCATED(&aes, sizeof(psa_key_id_t));

	zassert_not_equal(sha.mContext, hmac.mContext);
	zassert_not_equal(sha.mContext, hkdf.mContext);
	zassert_not_equal(sha.mContext, aes.mContext);
	zassert_not_equal(hmac.mContext, hkdf.mContext);
	zassert_not_equal(hmac.mContext, aes.mContext);
	zassert_not_equal(hkdf.mContext, aes.mContext);

	zassert_equal(otPlatCryptoSha256Deinit(&sha), OT_ERROR_NONE);
	zassert_equal(otPlatCryptoHmacSha256Deinit(&hmac), OT_ERROR_NONE);
	zassert_equal(otPlatCryptoHkdfDeinit(&hkdf), OT_ERROR_NONE);
	zassert_equal(otPlatCryptoAesFree(&aes), OT_ERROR_NONE);

	ASSERT_CONTEXT_CLEARED(&sha);
	ASSERT_CONTEXT_CLEARED(&hmac);
	ASSERT_CONTEXT_CLEARED(&hkdf);
	ASSERT_CONTEXT_CLEARED(&aes);
}

ZTEST_SUITE(openthread_crypto_psa, NULL, suite_setup, NULL, NULL, NULL);
