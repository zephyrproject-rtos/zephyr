/* Copyright (c) 2026 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/secure_storage/its/transform/aead.h>
#include <zephyr/sys/__assert.h>
#include <psa/crypto.h>
#include <string.h>

#define NONCE_SIZE CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_NONCE_SIZE
#define TAG_SIZE SECURE_STORAGE_ITS_TRANSFORM_AEAD_TAG_SIZE

static void xor_crypt(const uint8_t nonce[static NONCE_SIZE], size_t len,
		      const uint8_t *input, uint8_t *output)
{
	for (size_t i = 0; i != len; ++i) {
		output[i] = input[i] ^ nonce[i % NONCE_SIZE];
	}
}

static void compute_tag(size_t add_data_len, const uint8_t *add_data, size_t data_len,
			const uint8_t *data, uint8_t tag[static TAG_SIZE])
{
	memset(tag, 0, TAG_SIZE);
	for (size_t i = 0; i != add_data_len; ++i) {
		tag[i % TAG_SIZE] ^= add_data[i];
	}
	for (size_t i = 0; i != data_len; ++i) {
		tag[i % TAG_SIZE] ^= data[i];
	}
}

psa_status_t secure_storage_its_transform_aead_crypt(
		psa_key_usage_t operation, secure_storage_its_uid_t uid,
		const uint8_t nonce[static CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_NONCE_SIZE],
		size_t add_data_len, const uint8_t *add_data, size_t input_len,
		const uint8_t *input, size_t output_size, uint8_t *output, size_t *output_len)
{
	if (operation == PSA_KEY_USAGE_ENCRYPT) {
		__ASSERT_NO_MSG(output_size >= input_len + TAG_SIZE);
		xor_crypt(nonce, input_len, input, output);
		compute_tag(add_data_len, add_data, input_len, input, output + input_len);
		*output_len = input_len + TAG_SIZE;
	} else {
		uint8_t tag[TAG_SIZE];

		if (input_len < TAG_SIZE) {
			return PSA_ERROR_DATA_CORRUPT;
		}
		__ASSERT_NO_MSG(output_size >= input_len - TAG_SIZE);
		const size_t data_len = input_len - TAG_SIZE;

		xor_crypt(nonce, data_len, input, output);
		compute_tag(add_data_len, add_data, data_len, output, tag);
		if (memcmp(tag, input + data_len, TAG_SIZE) != 0) {
			return PSA_ERROR_INVALID_SIGNATURE;
		}
		*output_len = data_len;
	}
	return PSA_SUCCESS;
}
