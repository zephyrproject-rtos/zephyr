/* Copyright (c) 2026 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/secure_storage/its/transform/aead_get.h>
#include <psa/crypto.h>

#if defined(CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_SCHEME_AES_GCM)
#define PSA_KEY_TYPE PSA_KEY_TYPE_AES
#define PSA_ALG PSA_ALG_GCM
#elif defined(CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_SCHEME_CHACHA20_POLY1305)
#define PSA_KEY_TYPE PSA_KEY_TYPE_CHACHA20
#define PSA_ALG PSA_ALG_CHACHA20_POLY1305
#endif

void secure_storage_its_transform_aead_get_scheme(psa_key_type_t *key_type, psa_algorithm_t *alg)
{
	*key_type = PSA_KEY_TYPE;
	*alg = PSA_ALG;
}
