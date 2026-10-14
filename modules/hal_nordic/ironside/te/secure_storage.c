/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/secure_storage/its/transform/aead.h>
#include <psa/crypto.h>
#include <ironside_client/psa/crypto_extra.h>

void secure_storage_its_transform_aead_get_derivation_key(psa_key_id_t *key_id,
							  psa_algorithm_t *alg)
{
	*key_id = IRONSIDE_CLIENT_MEXT_ID;
	*alg = PSA_ALG_SP800_108_COUNTER_CMAC;
}
