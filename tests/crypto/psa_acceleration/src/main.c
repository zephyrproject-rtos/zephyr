/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Test the integration of cryptographic accelerators external to the tf-psa-crypto
 * repository. Algorithms implemented for the tests must not overlap with the algorithms
 * tested in the `mbedtls_psa` test, as we run those in parallel to validate the external
 * integration doesn't break builtin functionality.
 */

#include <zephyr/ztest.h>

#include <psa/crypto.h>

#include <tf-psa-crypto/drivers/external_integration.h>

static const uint8_t test_key[] = {1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
				   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32};
static const uint8_t test_input[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

static int count_init;
static int count_free;
static int count_key_agreement;

psa_status_t psa_driver_external_integration_init(void)
{
	count_init += 1;
	return PSA_SUCCESS;
}

void psa_driver_external_integration_free(void)
{
	count_free += 1;
}

psa_status_t psa_driver_external_integration_sign_message(
	const psa_key_attributes_t *attributes, const uint8_t *key_buffer, size_t key_buffer_size,
	psa_algorithm_t alg, const uint8_t *input, size_t input_length, uint8_t *signature,
	size_t signature_size, size_t *signature_length)
{
	if (alg != PSA_ALG_ECDSA(PSA_ALG_SHA_256)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	zassert_not_null(attributes);
	zassert_not_null(key_buffer);
	zassert_true(key_buffer_size > 0);
	zassert_true(signature_size > 0);
	zassert_equal(sizeof(test_input), input_length);
	zassert_mem_equal(test_input, input, sizeof(test_input));

	/* Mark the signature so calling code can validate */
	memset(signature, 0xAA, signature_size);
	*signature_length = signature_size;

	return PSA_SUCCESS;
}

psa_status_t psa_driver_external_integration_verify_message(
	const psa_key_attributes_t *attributes, const uint8_t *key_buffer, size_t key_buffer_size,
	psa_algorithm_t alg, const uint8_t *input, size_t input_length, const uint8_t *signature,
	size_t signature_length)
{
	if (alg != PSA_ALG_ECDSA(PSA_ALG_SHA_256)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	zassert_not_null(attributes);
	zassert_not_null(key_buffer);
	zassert_true(key_buffer_size > 0);
	zassert_true(signature_length > 0);
	zassert_equal(sizeof(test_input), input_length);
	zassert_mem_equal(test_input, input, sizeof(test_input));

	for (int i = 0; i < signature_length; i++) {
		if (signature[i] != 0xAA) {
			return PSA_ERROR_INVALID_SIGNATURE;
		}
	}
	return PSA_SUCCESS;
}

psa_status_t psa_driver_external_integration_sign_hash(const psa_key_attributes_t *attributes,
						       const uint8_t *key_buffer,
						       size_t key_buffer_size, psa_algorithm_t alg,
						       const uint8_t *hash, size_t hash_length,
						       uint8_t *signature, size_t signature_size,
						       size_t *signature_length)
{
	if (alg != PSA_ALG_ECDSA(PSA_ALG_SHA_256)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	zassert_not_null(attributes);
	zassert_not_null(key_buffer);
	zassert_true(key_buffer_size > 0);
	zassert_true(signature_size > 0);
	zassert_equal(sizeof(test_input), hash_length);
	zassert_mem_equal(test_input, hash, sizeof(test_input));

	/* Mark the signature so calling code can validate */
	memset(signature, 0xAB, signature_size);
	*signature_length = signature_size;

	return PSA_SUCCESS;
}

psa_status_t psa_driver_external_integration_verify_hash(
	const psa_key_attributes_t *attributes, const uint8_t *key_buffer, size_t key_buffer_size,
	psa_algorithm_t alg, const uint8_t *hash, size_t hash_length, const uint8_t *signature,
	size_t signature_length)
{
	if (alg != PSA_ALG_ECDSA(PSA_ALG_SHA_256)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	zassert_not_null(attributes);
	zassert_not_null(key_buffer);
	zassert_true(key_buffer_size > 0);
	zassert_true(signature_length > 0);
	zassert_equal(sizeof(test_input), hash_length);
	zassert_mem_equal(test_input, hash, sizeof(test_input));

	for (int i = 0; i < signature_length; i++) {
		if (signature[i] != 0xAB) {
			return PSA_ERROR_INVALID_SIGNATURE;
		}
	}
	return PSA_SUCCESS;
}

psa_status_t psa_driver_external_integration_cipher_encrypt(
	const psa_key_attributes_t *attributes, const uint8_t *key_buffer, size_t key_buffer_size,
	psa_algorithm_t alg, const uint8_t *iv, size_t iv_length, const uint8_t *input,
	size_t input_length, uint8_t *output, size_t output_size, size_t *output_length)
{
	if (alg != PSA_ALG_CBC_PKCS7) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	size_t expected_output_len =
		PSA_CIPHER_ENCRYPT_OUTPUT_SIZE(PSA_KEY_TYPE_AES, PSA_ALG_CBC_PKCS7, input_length);

	zassert_not_null(attributes);
	zassert_not_null(key_buffer);
	zassert_not_null(iv);
	zassert_true(key_buffer_size > 0);
	zassert_true(output_size > 0);
	zassert_true(iv_length > 0);
	zassert_true(output_size >= expected_output_len);

	zassert_equal(sizeof(test_input), input_length);
	zassert_mem_equal(test_input, input, sizeof(test_input));

	/* Mark the output so calling code can validate */
	memset(output, 0xAC, expected_output_len);
	*output_length = expected_output_len;
	return PSA_SUCCESS;
}

psa_status_t psa_driver_external_integration_cipher_decrypt(
	const psa_key_attributes_t *attributes, const uint8_t *key_buffer, size_t key_buffer_size,
	psa_algorithm_t alg, const uint8_t *input, size_t input_length, uint8_t *output,
	size_t output_size, size_t *output_length)
{
	if (alg != PSA_ALG_CBC_PKCS7) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	const size_t iv_size = PSA_CIPHER_IV_LENGTH(PSA_KEY_TYPE_AES, PSA_ALG_CBC_PKCS7);
	size_t expected_output_len = input_length - iv_size;

	zassert_not_null(attributes);
	zassert_not_null(key_buffer);
	zassert_true(key_buffer_size > 0);
	zassert_true(input_length > 0);
	zassert_true(output_size >= expected_output_len);

	/* Mark the output so calling code can validate */
	memset(output, 0xAD, expected_output_len);
	*output_length = expected_output_len;

	return PSA_SUCCESS;
}

psa_status_t psa_driver_external_integration_hash_compute(psa_algorithm_t alg, const uint8_t *input,
							  size_t input_length, uint8_t *hash,
							  size_t hash_size, size_t *hash_length)
{
	if (alg != PSA_ALG_MD5) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	/* Mark the hash so calling code can validate */
	memset(hash, 0xAE, hash_size);
	*hash_length = hash_size;

	return PSA_SUCCESS;
}

psa_status_t
psa_driver_external_integration_hash_setup(psa_driver_external_integration_hash_operation_t *ctx,
					   psa_algorithm_t alg)
{
	if (alg != PSA_ALG_MD5) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	ctx->custom_context[0] = 0x01;
	ctx->custom_context[1] = 0x01;
	ctx->custom_context[2] = 0x01;

	return PSA_SUCCESS;
}

psa_status_t
psa_driver_external_integration_hash_update(psa_driver_external_integration_hash_operation_t *ctx,
					    const uint8_t *input, size_t input_length)
{
	ctx->custom_context[1] = 0x02;

	zassert_equal(sizeof(test_input), input_length);
	zassert_mem_equal(test_input, input, sizeof(test_input));

	return PSA_SUCCESS;
}

psa_status_t
psa_driver_external_integration_hash_finish(psa_driver_external_integration_hash_operation_t *ctx,
					    uint8_t *hash, size_t hash_size, size_t *hash_length)
{
	ctx->custom_context[2] = 0x03;

	/* Mark the hash so calling code can validate */
	memset(hash, 0xC0, hash_size);
	*hash_length = hash_size;

	return PSA_SUCCESS;
}

psa_status_t psa_driver_external_integration_aead_encrypt(
	const psa_key_attributes_t *attributes, const uint8_t *key_buffer, size_t key_buffer_size,
	psa_algorithm_t alg, const uint8_t *nonce, size_t nonce_length,
	const uint8_t *additional_data, size_t additional_data_length, const uint8_t *plaintext,
	size_t plaintext_length, uint8_t *ciphertext, size_t ciphertext_size,
	size_t *ciphertext_length)
{
	if (alg != PSA_ALG_GCM) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	size_t expected_output_len =
		PSA_AEAD_ENCRYPT_OUTPUT_SIZE(PSA_KEY_TYPE_AES, PSA_ALG_GCM, plaintext_length);

	zassert_not_null(attributes);
	zassert_not_null(key_buffer);
	zassert_not_null(nonce);
	zassert_not_null(additional_data);
	zassert_true(key_buffer_size > 0);
	zassert_true(nonce_length > 0);
	zassert_true(ciphertext_size >= expected_output_len);

	zassert_equal(sizeof(test_input), plaintext_length);
	zassert_mem_equal(test_input, plaintext, sizeof(test_input));

	/* Mark the output so calling code can validate */
	memset(ciphertext, 0xAF, expected_output_len);
	*ciphertext_length = expected_output_len;
	return PSA_SUCCESS;
}

psa_status_t psa_driver_external_integration_aead_decrypt(
	const psa_key_attributes_t *attributes, const uint8_t *key_buffer, size_t key_buffer_size,
	psa_algorithm_t alg, const uint8_t *nonce, size_t nonce_length,
	const uint8_t *additional_data, size_t additional_data_length, const uint8_t *ciphertext,
	size_t ciphertext_length, uint8_t *plaintext, size_t plaintext_size,
	size_t *plaintext_length)
{
	if (alg != PSA_ALG_GCM) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	size_t tag_len = PSA_AEAD_TAG_LENGTH(PSA_KEY_TYPE_AES, 8 * sizeof(test_key), PSA_ALG_GCM);
	size_t expected_output_len = ciphertext_length - tag_len;

	zassert_not_null(attributes);
	zassert_not_null(key_buffer);
	zassert_not_null(nonce);
	zassert_not_null(additional_data);
	zassert_true(key_buffer_size > 0);
	zassert_true(plaintext_size > 0);
	zassert_true(nonce_length > 0);
	zassert_true(plaintext_size >= expected_output_len);
	zassert_equal(sizeof(test_input) + tag_len, ciphertext_length);

	/* Mark the output so calling code can validate */
	memset(plaintext, 0xB0, expected_output_len);
	*plaintext_length = expected_output_len;
	return PSA_SUCCESS;
}

psa_status_t psa_driver_external_integration_mac_compute(const psa_key_attributes_t *attributes,
							 const uint8_t *key_buffer,
							 size_t key_buffer_size,
							 psa_algorithm_t alg, const uint8_t *input,
							 size_t input_length, uint8_t *mac,
							 size_t mac_size, size_t *mac_length)
{
	if (alg != PSA_ALG_HMAC(PSA_ALG_SHA_1)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	zassert_not_null(attributes);
	zassert_not_null(key_buffer);
	zassert_true(key_buffer_size > 0);

	/* Mark the hash so calling code can validate */
	memset(mac, 0xB1, mac_size);
	*mac_length = mac_size;

	return PSA_SUCCESS;
}

psa_status_t psa_driver_external_integration_asymmetric_encrypt(
	const psa_key_attributes_t *attributes, const uint8_t *key_buffer, size_t key_buffer_size,
	psa_algorithm_t alg, const uint8_t *input, size_t input_length, const uint8_t *salt,
	size_t salt_length, uint8_t *output, size_t output_size, size_t *output_length)
{
	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t psa_driver_external_integration_asymmetric_decrypt(
	const psa_key_attributes_t *attributes, const uint8_t *key_buffer, size_t key_buffer_size,
	psa_algorithm_t alg, const uint8_t *input, size_t input_length, const uint8_t *salt,
	size_t salt_length, uint8_t *output, size_t output_size, size_t *output_length)
{
	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t psa_driver_external_integration_key_agreement(
	const psa_key_attributes_t *attributes, const uint8_t *key_buffer, size_t key_buffer_size,
	psa_algorithm_t alg, const uint8_t *peer_key, size_t peer_key_length,
	uint8_t *shared_secret, size_t shared_secret_size, size_t *shared_secret_length)
{
	if (alg != PSA_ALG_ECDH) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	count_key_agreement += 1;

	memset(shared_secret, 0x2F, shared_secret_size);
	*shared_secret_length = shared_secret_size;
	return PSA_SUCCESS;
}

ZTEST_USER(test_psa_acceleration, test_init_calls)
{
	/* Initialised by default through `psa_crypto_init` */
	zassert_equal(1, count_init);
	zassert_equal(0, count_free);

	/* Free resources */
	mbedtls_psa_crypto_free();
	zassert_equal(1, count_init);
	zassert_equal(1, count_free);

	/* Re-initialise for additional tests */
	psa_crypto_init();
	zassert_equal(2, count_init);
	zassert_equal(1, count_free);
}

ZTEST_USER(test_psa_acceleration, test_sign_verify_message)
{
	psa_key_attributes_t key_attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_algorithm_t alg = PSA_ALG_ECDSA(PSA_ALG_SHA_256);
	uint8_t signature[PSA_SIGNATURE_MAX_SIZE] = {0};
	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	psa_status_t status;
	size_t out_len;

	psa_set_key_type(&key_attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_algorithm(&key_attr, alg);
	psa_set_key_usage_flags(&key_attr,
				PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_VERIFY_MESSAGE);

	status = psa_import_key(&key_attr, test_key, sizeof(test_key), &key_id);
	zassert_equal(status, PSA_SUCCESS);

	status = psa_sign_message(key_id, alg, test_input, sizeof(test_input), signature,
				  sizeof(signature), &out_len);
	zassert_equal(status, PSA_SUCCESS);
	zassert_equal(sizeof(signature), out_len);
	for (int i = 0; i < sizeof(signature); i++) {
		zassert_equal(0xAA, signature[i]);
	}

	status = psa_verify_message(key_id, alg, test_input, sizeof(test_input), signature,
				    sizeof(signature));
	zassert_equal(status, PSA_SUCCESS);

	status = psa_destroy_key(key_id);
	zassert_equal(status, PSA_SUCCESS);
}

ZTEST_USER(test_psa_acceleration, test_sign_verify_hash)
{
	psa_key_attributes_t key_attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_algorithm_t alg = PSA_ALG_ECDSA(PSA_ALG_SHA_256);
	uint8_t signature[PSA_SIGNATURE_MAX_SIZE] = {0};
	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	psa_status_t status;
	size_t out_len;

	psa_set_key_type(&key_attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_algorithm(&key_attr, alg);
	psa_set_key_usage_flags(&key_attr, PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_VERIFY_HASH);

	status = psa_import_key(&key_attr, test_key, sizeof(test_key), &key_id);
	zassert_equal(status, PSA_SUCCESS);

	status = psa_sign_hash(key_id, alg, test_input, sizeof(test_input), signature,
			       sizeof(signature), &out_len);
	zassert_equal(status, PSA_SUCCESS);
	zassert_equal(sizeof(signature), out_len);
	for (int i = 0; i < sizeof(signature); i++) {
		zassert_equal(0xAB, signature[i]);
	}

	status = psa_verify_hash(key_id, alg, test_input, sizeof(test_input), signature,
				 sizeof(signature));
	zassert_equal(status, PSA_SUCCESS);

	status = psa_destroy_key(key_id);
	zassert_equal(status, PSA_SUCCESS);
}

#define AES_ENCRYPTED_OUTPUT_SIZE(input)                                                           \
	PSA_CIPHER_IV_LENGTH(PSA_KEY_TYPE_AES, PSA_ALG_CBC_PKCS7) +                                \
		PSA_CIPHER_ENCRYPT_OUTPUT_SIZE(PSA_KEY_TYPE_AES, PSA_ALG_CBC_PKCS7, sizeof(input))

ZTEST_USER(test_psa_acceleration, test_cipher_encrypt_decrypt)
{
	const size_t iv_size = PSA_CIPHER_IV_LENGTH(PSA_KEY_TYPE_AES, PSA_ALG_CBC_PKCS7);
	const size_t cipher_size = PSA_CIPHER_ENCRYPT_OUTPUT_SIZE(
		PSA_KEY_TYPE_AES, PSA_ALG_CBC_PKCS7, sizeof(test_input));
	psa_key_attributes_t key_attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_algorithm_t alg = PSA_ALG_CBC_PKCS7;
	uint8_t cipher[iv_size + cipher_size];
	uint8_t plaintext[cipher_size];
	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	psa_status_t status;
	size_t out_len;

	psa_set_key_type(&key_attr, PSA_KEY_TYPE_AES);
	psa_set_key_algorithm(&key_attr, alg);
	psa_set_key_usage_flags(&key_attr, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);

	status = psa_import_key(&key_attr, test_key, sizeof(test_key), &key_id);
	zassert_equal(status, PSA_SUCCESS);

	status = psa_cipher_encrypt(key_id, alg, test_input, sizeof(test_input), cipher,
				    sizeof(cipher), &out_len);
	zassert_equal(status, PSA_SUCCESS);
	zassert_equal(sizeof(cipher), out_len);
	for (int i = 0; i < cipher_size; i++) {
		zassert_equal(0xAC, cipher[iv_size + i]);
	}

	status = psa_cipher_decrypt(key_id, alg, cipher, sizeof(cipher), plaintext,
				    sizeof(plaintext), &out_len);
	zassert_equal(status, PSA_SUCCESS);
	zassert_equal(sizeof(plaintext), out_len);
	for (int i = 0; i < out_len; i++) {
		zassert_equal(0xAD, plaintext[i]);
	}

	status = psa_destroy_key(key_id);
	zassert_equal(status, PSA_SUCCESS);
}

ZTEST_USER(test_psa_acceleration, test_hash)
{
	uint8_t out_buf[PSA_HASH_LENGTH(PSA_ALG_MD5)] = {0};
	psa_status_t status;
	size_t out_len;

	status = psa_hash_compute(PSA_ALG_MD5, test_input, sizeof(test_input), out_buf,
				  sizeof(out_buf), &out_len);
	zassert_equal(status, PSA_SUCCESS);
	zassert_equal(sizeof(out_buf), out_len);
	for (int i = 0; i < out_len; i++) {
		zassert_equal(0xAE, out_buf[i]);
	}
}

ZTEST_USER(test_psa_acceleration, test_hash_multi)
{
	uint8_t out_buf[PSA_HASH_LENGTH(PSA_ALG_MD5)] = {0};
	psa_hash_operation_t operation = {0};
	psa_status_t status;
	size_t out_len;

	status = psa_hash_setup(&operation, PSA_ALG_MD5);
	zassert_equal(status, PSA_SUCCESS);

	status = psa_hash_update(&operation, test_input, sizeof(test_input));
	zassert_equal(status, PSA_SUCCESS);

	status = psa_hash_finish(&operation, out_buf, sizeof(out_buf), &out_len);
	zassert_equal(status, PSA_SUCCESS);
	zassert_equal(sizeof(out_buf), out_len);
	for (int i = 0; i < out_len; i++) {
		zassert_equal(0xC0, out_buf[i]);
	}

	/* Accessing the private context here is actually advantageous, as it proves the variable
	 * instantiation actually contains the appropriate context field.
	 */
	zassert_equal(0x01, operation.private_ctx.external_integration_ctx.custom_context[0]);
	zassert_equal(0x02, operation.private_ctx.external_integration_ctx.custom_context[1]);
	zassert_equal(0x03, operation.private_ctx.external_integration_ctx.custom_context[2]);
}

ZTEST_USER(test_psa_acceleration, test_aead_encrypt_decrypt)
{
	const size_t nonce_size = PSA_AEAD_NONCE_LENGTH(PSA_KEY_TYPE_AES, PSA_ALG_GCM);
	const size_t cipher_size =
		PSA_AEAD_ENCRYPT_OUTPUT_SIZE(PSA_KEY_TYPE_AES, PSA_ALG_GCM, sizeof(test_input));
	psa_key_attributes_t key_attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_algorithm_t alg = PSA_ALG_GCM;
	uint8_t cipher[cipher_size];
	uint8_t plaintext[sizeof(test_input)];
	uint8_t associated_data[8] = {0};
	uint8_t nonce[nonce_size];
	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	psa_status_t status;
	size_t out_len;

	memset(nonce, 0x00, nonce_size);

	psa_set_key_type(&key_attr, PSA_KEY_TYPE_AES);
	psa_set_key_algorithm(&key_attr, alg);
	psa_set_key_usage_flags(&key_attr, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);

	status = psa_import_key(&key_attr, test_key, sizeof(test_key), &key_id);
	zassert_equal(status, PSA_SUCCESS);

	status = psa_aead_encrypt(key_id, alg, nonce, sizeof(nonce), associated_data,
				  sizeof(associated_data), test_input, sizeof(test_input), cipher,
				  sizeof(cipher), &out_len);
	zassert_equal(status, PSA_SUCCESS);
	zassert_equal(sizeof(cipher), out_len);
	for (int i = 0; i < out_len; i++) {
		zassert_equal(0xAF, cipher[i]);
	}

	status = psa_aead_decrypt(key_id, alg, nonce, sizeof(nonce), associated_data,
				  sizeof(associated_data), cipher, sizeof(cipher), plaintext,
				  sizeof(plaintext), &out_len);
	zassert_equal(status, PSA_SUCCESS);
	zassert_equal(sizeof(plaintext), out_len);
	for (int i = 0; i < out_len; i++) {
		zassert_equal(0xB0, plaintext[i]);
	}

	status = psa_destroy_key(key_id);
	zassert_equal(status, PSA_SUCCESS);
}

ZTEST_USER(test_psa_acceleration, test_mac)
{
	size_t mac_length = PSA_MAC_LENGTH(PSA_KEY_TYPE_HMAC, 8 * sizeof(test_key),
					   PSA_ALG_HMAC(PSA_ALG_SHA_1));
	psa_key_attributes_t key_attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_algorithm_t alg = PSA_ALG_HMAC(PSA_ALG_SHA_1);
	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	uint8_t mac_buf[mac_length];
	psa_status_t status;
	size_t out_len;

	psa_set_key_type(&key_attr, PSA_KEY_TYPE_HMAC);
	psa_set_key_algorithm(&key_attr, alg);
	psa_set_key_usage_flags(&key_attr, PSA_KEY_USAGE_SIGN_MESSAGE);

	status = psa_import_key(&key_attr, test_key, sizeof(test_key), &key_id);
	zassert_equal(status, PSA_SUCCESS);

	status = psa_mac_compute(key_id, alg, test_input, sizeof(test_input), mac_buf,
				 sizeof(mac_buf), &out_len);
	zassert_equal(status, PSA_SUCCESS);
	zassert_equal(sizeof(mac_buf), out_len);
	for (int i = 0; i < out_len; i++) {
		zassert_equal(0xB1, mac_buf[i]);
	}

	status = psa_destroy_key(key_id);
	zassert_equal(status, PSA_SUCCESS);
}

ZTEST_USER(test_psa_acceleration, test_asymmetric_encrypt_decrypt)
{
	/* Asymmetric encryption is primarily RSA, which requires non-trivial keys */
	ztest_test_skip();
}

ZTEST_USER(test_psa_acceleration, test_key_agreement)
{
	psa_key_attributes_t key_attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_attributes_t derived_key_attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_algorithm_t alg = PSA_ALG_ECDH;
	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	psa_key_id_t derived_key_id = PSA_KEY_ID_NULL;
	uint8_t peer_key[32] = {0};
	psa_status_t status;

	psa_set_key_type(&key_attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_algorithm(&key_attr, alg);
	psa_set_key_usage_flags(&key_attr, PSA_KEY_USAGE_DERIVE);

	psa_set_key_type(&derived_key_attr, PSA_KEY_TYPE_DERIVE);
	psa_set_key_algorithm(&derived_key_attr, alg);
	psa_set_key_usage_flags(&derived_key_attr, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);

	status = psa_import_key(&key_attr, test_key, sizeof(test_key), &key_id);
	zassert_equal(status, PSA_SUCCESS);

	status = psa_key_agreement(key_id, peer_key, sizeof(peer_key), alg, &derived_key_attr,
				   &derived_key_id);
	zassert_equal(status, PSA_SUCCESS);
	zassert_not_equal(PSA_KEY_ID_NULL, derived_key_id);

	zassert_equal(1, count_key_agreement);

	status = psa_destroy_key(derived_key_id);
	zassert_equal(status, PSA_SUCCESS);

	status = psa_destroy_key(key_id);
	zassert_equal(status, PSA_SUCCESS);
}

ZTEST_SUITE(test_psa_acceleration, NULL, NULL, NULL, NULL, NULL);
