/* Copyright (c) 2024 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#include <../library/psa_crypto_driver_wrappers.h>
#include <zephyr/secure_storage/its/transform.h>
#include <zephyr/secure_storage/its/transform/aead_get.h>
#include <zephyr/sys/__assert.h>
#include <mbedtls/platform_util.h>

static psa_status_t psa_aead_crypt(psa_key_usage_t operation, secure_storage_its_uid_t uid,
				   const uint8_t nonce
				   [static CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_NONCE_SIZE],
				   size_t add_data_len, const void *add_data, size_t input_len,
				   const void *input, size_t output_size, void *output,
				   size_t *output_len)
{
	psa_status_t ret;
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	uint8_t key[CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_KEY_SIZE];
	psa_key_type_t key_type;
	psa_algorithm_t alg;
	psa_status_t (*aead_crypt)(const psa_key_attributes_t *attributes, const uint8_t *key,
				   size_t key_size, psa_algorithm_t alg, const uint8_t *nonce,
				   size_t nonce_length, const uint8_t *add_data,
				   size_t add_data_len, const uint8_t *input, size_t input_len,
				   uint8_t *output, size_t output_size, size_t *output_len);

	secure_storage_its_transform_aead_get_scheme(&key_type, &alg);

	psa_set_key_usage_flags(&key_attributes, operation);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_type(&key_attributes, key_type);
	psa_set_key_algorithm(&key_attributes, alg);
	psa_set_key_bits(&key_attributes, sizeof(key) * 8);

	/* Avoid calling psa_aead_*crypt() because that would require importing keys into
	 * PSA Crypto. This gets called from PSA Crypto for storing persistent keys so,
	 * even if using PSA_KEY_LIFETIME_VOLATILE, it would corrupt the global key store
	 * which holds all the active keys in the PSA Crypto core.
	 */
	aead_crypt = (operation == PSA_KEY_USAGE_ENCRYPT) ?
		      psa_driver_wrapper_aead_encrypt : psa_driver_wrapper_aead_decrypt;

	ret = secure_storage_its_transform_aead_get_key(uid, key);
	if (ret != PSA_SUCCESS) {
		return ret;
	}

	ret = aead_crypt(&key_attributes, key, sizeof(key), alg, nonce,
			 CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_NONCE_SIZE, add_data,
			 add_data_len, input, input_len, output, output_size, output_len);

	mbedtls_platform_zeroize(key, sizeof(key));
	return ret;
}

enum { CIPHERTEXT_MAX_SIZE
	= PSA_AEAD_ENCRYPT_OUTPUT_MAX_SIZE(CONFIG_SECURE_STORAGE_ITS_MAX_DATA_SIZE) };

BUILD_ASSERT(CONFIG_SECURE_STORAGE_ITS_TRANSFORM_OUTPUT_OVERHEAD
	     == CIPHERTEXT_MAX_SIZE - CONFIG_SECURE_STORAGE_ITS_MAX_DATA_SIZE
		+ CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_NONCE_SIZE);

BUILD_ASSERT(SECURE_STORAGE_ALL_CREATE_FLAGS
	     <= (1 << (8 * sizeof(secure_storage_packed_create_flags_t))) - 1);

struct stored_entry {
	secure_storage_packed_create_flags_t create_flags;
	uint8_t nonce[CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_NONCE_SIZE];
	uint8_t ciphertext[CIPHERTEXT_MAX_SIZE]; /* Keep last as this is variable in size. */
} __packed;
BUILD_ASSERT(sizeof(struct stored_entry) == SECURE_STORAGE_ITS_TRANSFORM_MAX_STORED_DATA_SIZE);

/** @return The length of a `struct stored_entry` whose `ciphertext` is `len` bytes long. */
#define STORED_ENTRY_LEN(len) (sizeof(struct stored_entry) - CIPHERTEXT_MAX_SIZE + len)

struct additional_data {
	secure_storage_its_uid_t uid;
	secure_storage_packed_create_flags_t create_flags;
} __packed;

psa_status_t secure_storage_its_transform_to_store(
		secure_storage_its_uid_t uid, size_t data_len, const void *data,
		secure_storage_packed_create_flags_t create_flags,
		uint8_t stored_data[static SECURE_STORAGE_ITS_TRANSFORM_MAX_STORED_DATA_SIZE],
		size_t *stored_data_len)
{
	psa_status_t ret;
	struct stored_entry *stored_entry = (struct stored_entry *)stored_data;
	const struct additional_data add_data = {.uid = uid, .create_flags = create_flags};
	size_t ciphertext_len;

	stored_entry->create_flags = create_flags;

	ret = secure_storage_its_transform_aead_get_nonce(stored_entry->nonce);
	if (ret != PSA_SUCCESS) {
		return ret;
	}

	ret = psa_aead_crypt(PSA_KEY_USAGE_ENCRYPT, uid, stored_entry->nonce, sizeof(add_data),
			     &add_data, data_len, data, sizeof(stored_entry->ciphertext),
			     &stored_entry->ciphertext, &ciphertext_len);
	if (ret == PSA_SUCCESS) {
		*stored_data_len = STORED_ENTRY_LEN(ciphertext_len);
	}
	return ret;
}

psa_status_t secure_storage_its_transform_from_store(
		secure_storage_its_uid_t uid, size_t stored_data_len,
		const uint8_t stored_data[static SECURE_STORAGE_ITS_TRANSFORM_MAX_STORED_DATA_SIZE],
		size_t data_size, void *data, size_t *data_len,
		psa_storage_create_flags_t *create_flags)
{
	if (stored_data_len < STORED_ENTRY_LEN(0)) {
		return PSA_ERROR_STORAGE_FAILURE;
	}

	psa_status_t ret;
	struct stored_entry *stored_entry = (struct stored_entry *)stored_data;
	const struct additional_data add_data = {.uid = uid,
						 .create_flags = stored_entry->create_flags};
	const size_t ciphertext_len = stored_data_len - STORED_ENTRY_LEN(0);

	ret = psa_aead_crypt(PSA_KEY_USAGE_DECRYPT, uid, stored_entry->nonce, sizeof(add_data),
			     &add_data, ciphertext_len, stored_entry->ciphertext, data_size, data,
			     data_len);
	if (ret == PSA_SUCCESS) {
		*create_flags = stored_entry->create_flags;
	}
	return ret;
}
