/* Copyright (c) 2026 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/secure_storage/its/transform/aead_get.h>
#include <zephyr/kernel.h>
#include <psa/crypto.h>

psa_status_t secure_storage_its_transform_aead_get_nonce(
		uint8_t nonce[static CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_NONCE_SIZE])
{
	psa_status_t ret = PSA_SUCCESS;
	static uint8_t s_nonce[CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_NONCE_SIZE];
	static bool s_nonce_initialized;
	static K_MUTEX_DEFINE(s_nonce_mutex);

	k_mutex_lock(&s_nonce_mutex, K_FOREVER);

	if (!s_nonce_initialized) {
		ret = psa_generate_random(s_nonce, sizeof(s_nonce));
		if (ret != PSA_SUCCESS) {
			goto exit;
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

exit:
	k_mutex_unlock(&s_nonce_mutex);
	return ret;
}
