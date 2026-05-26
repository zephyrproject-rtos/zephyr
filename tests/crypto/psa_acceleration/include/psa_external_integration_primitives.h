/* Copyright Embeint Inc
 * SPDX-License-Identifier: Apache-2.0
 *
 * File must declare the following structs:
 *    struct psa_driver_external_integration_hash_operation_t
 */

#ifndef PSA_CRYPTO_DRIVER_EXTERNAL_INTEGRATION_PRIMITIVES_H
#define PSA_CRYPTO_DRIVER_EXTERNAL_INTEGRATION_PRIMITIVES_H

#include <stdint.h>

struct psa_driver_external_integration_hash_operation {
	uint32_t custom_context[3];
};

typedef struct psa_driver_external_integration_hash_operation
	psa_driver_external_integration_hash_operation_t;

#endif /* PSA_CRYPTO_DRIVER_EXTERNAL_INTEGRATION_PRIMITIVES_H */
