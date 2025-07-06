/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fff.h>
#include <psa/crypto.h>

/* List of fakes used by this unit tester */
#define AES_FFF_FAKES_LIST(FAKE)               \
		FAKE(psa_crypto_init)          \
		FAKE(psa_generate_random)      \
		FAKE(psa_import_key)           \
		FAKE(psa_cipher_encrypt)       \
		FAKE(psa_destroy_key)

DECLARE_FAKE_VALUE_FUNC(psa_status_t, psa_crypto_init);
DECLARE_FAKE_VALUE_FUNC(psa_status_t, psa_generate_random, uint8_t *, size_t);
DECLARE_FAKE_VALUE_FUNC(psa_status_t, psa_import_key, const psa_key_attributes_t *, const uint8_t *,
			size_t, mbedtls_svc_key_id_t *);
DECLARE_FAKE_VALUE_FUNC(psa_status_t, psa_cipher_encrypt, mbedtls_svc_key_id_t, psa_algorithm_t,
			const uint8_t *, size_t, uint8_t *, size_t, size_t *);
DECLARE_FAKE_VALUE_FUNC(psa_status_t, psa_destroy_key, mbedtls_svc_key_id_t);
