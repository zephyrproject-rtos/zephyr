/* Copyright (c) 2024 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#include <psa_crypto_driver_wrappers.h>
#include <zephyr/secure_storage/its/transform/aead.h>
#include <mbedtls/platform_util.h>

BUILD_ASSERT(SECURE_STORAGE_ITS_TRANSFORM_AEAD_TAG_SIZE == 16);

psa_status_t secure_storage_its_transform_aead_crypt(
		psa_key_usage_t operation, secure_storage_its_uid_t uid,
		const uint8_t nonce[static CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_NONCE_SIZE],
		size_t add_data_len, const uint8_t *add_data, size_t input_len,
		const uint8_t *input, size_t output_size, uint8_t *output, size_t *output_len)
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
	psa_set_key_bits(&key_attributes, PSA_BYTES_TO_BITS(sizeof(key)));

	/* Avoid calling psa_aead_*crypt() because that would require importing keys into
	 * PSA Crypto. This gets called from PSA Crypto for storing persistent keys so,
	 * even if using PSA_KEY_LIFETIME_VOLATILE, it would corrupt the global key store
	 * which holds all the active keys in the PSA Crypto core.
	 */
	aead_crypt = (operation == PSA_KEY_USAGE_ENCRYPT) ?
		      psa_driver_wrapper_aead_encrypt : psa_driver_wrapper_aead_decrypt;

	ret = secure_storage_its_transform_aead_get_key(uid, key);
	if (ret != PSA_SUCCESS) {
		goto exit;
	}

	ret = aead_crypt(&key_attributes, key, sizeof(key), alg, nonce,
			 CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_NONCE_SIZE, add_data,
			 add_data_len, input, input_len, output, output_size, output_len);

exit:
	mbedtls_platform_zeroize(key, sizeof(key));
	return ret;
}
