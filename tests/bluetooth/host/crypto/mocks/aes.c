/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "mocks/aes.h"

DEFINE_FAKE_VALUE_FUNC(psa_status_t, psa_crypto_init);
DEFINE_FAKE_VALUE_FUNC(psa_status_t, psa_generate_random, uint8_t *, size_t);
DEFINE_FAKE_VALUE_FUNC(psa_status_t, psa_import_key, const psa_key_attributes_t *, const uint8_t *,
			size_t, mbedtls_svc_key_id_t *);
DEFINE_FAKE_VALUE_FUNC(psa_status_t, psa_cipher_encrypt, mbedtls_svc_key_id_t, psa_algorithm_t,
			const uint8_t *, size_t, uint8_t *, size_t, size_t *);
DEFINE_FAKE_VALUE_FUNC(psa_status_t, psa_destroy_key, mbedtls_svc_key_id_t);
