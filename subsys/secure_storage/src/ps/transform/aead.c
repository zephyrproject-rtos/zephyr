/* Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/secure_storage/ps/transform.h>
#include <zephyr/secure_storage/ps/transform/aead.h>
#include <zephyr/sys/__assert.h>
#include <psa/crypto.h>

BUILD_ASSERT(CONFIG_SECURE_STORAGE_PS_TRANSFORM_OUTPUT_OVERHEAD
	     > CONFIG_SECURE_STORAGE_PS_TRANSFORM_AEAD_NONCE_SIZE);

enum { CIPHERTEXT_MAX_SIZE = CONFIG_SECURE_STORAGE_PS_MAX_DATA_SIZE
			     + SECURE_STORAGE_PS_TRANSFORM_AEAD_TAG_SIZE };

struct stored_entry {
	uint8_t nonce[CONFIG_SECURE_STORAGE_PS_TRANSFORM_AEAD_NONCE_SIZE];
	uint8_t ciphertext[CIPHERTEXT_MAX_SIZE]; /* Keep last as this is variable in size. */
} __packed;
BUILD_ASSERT(sizeof(struct stored_entry) == SECURE_STORAGE_PS_TRANSFORM_MAX_STORED_DATA_SIZE);

/** @return The length of a `struct stored_entry` whose `ciphertext` is `len` bytes long. */
#define STORED_ENTRY_LEN(len) (sizeof(struct stored_entry) - CIPHERTEXT_MAX_SIZE + len)

struct additional_data {
	psa_storage_uid_t uid;
	uint8_t replay_protection[CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_SIZE];
} __packed;

psa_status_t secure_storage_ps_transform_to_store(
		psa_storage_uid_t uid, size_t data_len, const void *data,
		const uint8_t
		replay_protection[static CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_SIZE],
		uint8_t stored_data[static SECURE_STORAGE_PS_TRANSFORM_MAX_STORED_DATA_SIZE],
		size_t *stored_data_len)
{
	psa_status_t ret;
	struct stored_entry *stored_entry = (struct stored_entry *)stored_data;
	struct additional_data add_data;
	size_t ciphertext_len;

	add_data.uid = uid;
	memcpy(add_data.replay_protection, replay_protection,
	       CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_SIZE);

	ret = secure_storage_ps_transform_aead_get_nonce(stored_entry->nonce);
	if (ret != PSA_SUCCESS) {
		return ret;
	}

	ret = secure_storage_ps_transform_aead_crypt(
			PSA_KEY_USAGE_ENCRYPT, uid, stored_entry->nonce, sizeof(add_data),
			(const uint8_t *)&add_data, data_len, data,
			sizeof(stored_entry->ciphertext), stored_entry->ciphertext,
			&ciphertext_len);
	if (ret == PSA_SUCCESS) {
		__ASSERT_NO_MSG(ciphertext_len == data_len
						  + SECURE_STORAGE_PS_TRANSFORM_AEAD_TAG_SIZE);
		*stored_data_len = STORED_ENTRY_LEN(ciphertext_len);
	}
	return ret;
}

psa_status_t secure_storage_ps_transform_from_store(
		psa_storage_uid_t uid, size_t stored_data_len,
		const uint8_t stored_data[static SECURE_STORAGE_PS_TRANSFORM_MAX_STORED_DATA_SIZE],
		const uint8_t
		replay_protection[static CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_SIZE],
		size_t data_size, void *data, size_t *data_len)
{
	if (stored_data_len < STORED_ENTRY_LEN(0) + SECURE_STORAGE_PS_TRANSFORM_AEAD_TAG_SIZE) {
		return PSA_ERROR_DATA_CORRUPT;
	}

	psa_status_t ret;
	struct stored_entry *stored_entry = (struct stored_entry *)stored_data;
	struct additional_data add_data;
	const size_t ciphertext_len = stored_data_len - STORED_ENTRY_LEN(0);

	add_data.uid = uid;
	memcpy(add_data.replay_protection, replay_protection,
	       CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_SIZE);

	ret = secure_storage_ps_transform_aead_crypt(
			PSA_KEY_USAGE_DECRYPT, uid, stored_entry->nonce, sizeof(add_data),
			(const uint8_t *)&add_data, ciphertext_len, stored_entry->ciphertext,
			data_size, data, data_len);
	if (ret == PSA_SUCCESS) {
		__ASSERT_NO_MSG(*data_len == ciphertext_len
					     - SECURE_STORAGE_PS_TRANSFORM_AEAD_TAG_SIZE);
	}
	return ret;
}
