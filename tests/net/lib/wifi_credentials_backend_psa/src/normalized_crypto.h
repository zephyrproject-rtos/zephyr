/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PSA_CRYPTO_H
#define PSA_CRYPTO_H

#include "zephyr/types.h"
#include "psa/crypto_types.h"
#include "psa/crypto_values.h"

struct psa_client_key_attributes_s {
	uint16_t type;
	uint16_t bits;
	uint32_t lifetime;
	psa_key_id_t id;
	uint32_t usage;
	uint32_t alg;
};

struct psa_key_attributes_s {
	struct psa_client_key_attributes_s client;
};

typedef struct psa_key_attributes_s psa_key_attributes_t;

psa_status_t psa_import_key(const psa_key_attributes_t *attributes, const uint8_t *data,
			    size_t data_length, mbedtls_svc_key_id_t *key);

psa_status_t psa_export_key(mbedtls_svc_key_id_t key, uint8_t *data, size_t data_size,
			    size_t *data_length);

psa_status_t psa_destroy_key(mbedtls_svc_key_id_t key);
void psa_set_key_id(psa_key_attributes_t *attributes, mbedtls_svc_key_id_t key);
void psa_set_key_bits(psa_key_attributes_t *attributes, size_t bits);
void psa_set_key_type(psa_key_attributes_t *attributes, psa_key_type_t type);
void psa_set_key_algorithm(psa_key_attributes_t *attributes, psa_algorithm_t alg);
void psa_set_key_lifetime(psa_key_attributes_t *attributes, psa_key_lifetime_t lifetime);
psa_status_t psa_destroy_key(mbedtls_svc_key_id_t key);
void psa_set_key_usage_flags(psa_key_attributes_t *attributes, psa_key_usage_t usage_flags);

#endif
