/* Copyright (c) 2026 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/secure_storage/its/transform/aead.h>
#include <psa/crypto.h>
#include <string.h>

psa_status_t secure_storage_its_transform_aead_get_nonce(
		uint8_t nonce[static CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_NONCE_SIZE])
{
	psa_status_t ret;
	static uint8_t s_nonce[CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_NONCE_SIZE];
	static bool s_nonce_initialized;

	if (!s_nonce_initialized) {
		ret = psa_generate_random(s_nonce, sizeof(s_nonce));
		if (ret != PSA_SUCCESS) {
			return ret;
		}
		s_nonce_initialized = true;
	} else {
		for (unsigned int i = 0; i != sizeof(s_nonce); ++i) {
			++s_nonce[i];
			if (s_nonce[i] != 0) {
				break;
			}
		}
	}
	memcpy(nonce, &s_nonce, sizeof(s_nonce));

	return PSA_SUCCESS;
}
