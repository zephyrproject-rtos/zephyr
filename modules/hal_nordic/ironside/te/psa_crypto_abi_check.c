/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* The IronSide TE client's psa_crypto.c is compiled against its own PSA headers
 * and everything else against TF-PSA-Crypto's, so the two meet only at the ABI.
 * This holds TF-PSA-Crypto's view of each object passed across to the layout
 * the client publishes, in the configuration the rest of the image is built
 * with. The client headers that applications include beside <psa/crypto.h>
 * are included too, so that a clash shows here first.
 */

#include <stddef.h>

#include <zephyr/toolchain.h>

#include <psa/crypto.h>

#include <ironside_client/psa/crypto_abi.h>
#include <ironside_client/psa/crypto_extra.h>
#include <ironside_client/psa/initial_attestation.h>

#if !defined(MBEDTLS_PSA_CRYPTO_CLIENT) || defined(MBEDTLS_PSA_CRYPTO_C)
#error "IronSide TE needs TF-PSA-Crypto as a pure PSA Crypto client"
#endif

BUILD_ASSERT(_Generic((mbedtls_svc_key_id_t)0, psa_key_id_t : 1, default : 0),
	     "the entry points take mbedtls_svc_key_id_t where the client has psa_key_id_t");

#define ASSERT_SCALAR(type, size) \
	BUILD_ASSERT(sizeof(type) == (size), #type " width")

ASSERT_SCALAR(psa_status_t, IRONSIDE_CLIENT_PSA_ABI_STATUS_SIZE);
ASSERT_SCALAR(psa_key_id_t, IRONSIDE_CLIENT_PSA_ABI_KEY_ID_SIZE);
ASSERT_SCALAR(psa_key_type_t, IRONSIDE_CLIENT_PSA_ABI_KEY_TYPE_SIZE);
ASSERT_SCALAR(psa_key_lifetime_t, IRONSIDE_CLIENT_PSA_ABI_KEY_LIFETIME_SIZE);
ASSERT_SCALAR(psa_key_usage_t, IRONSIDE_CLIENT_PSA_ABI_KEY_USAGE_SIZE);
ASSERT_SCALAR(psa_algorithm_t, IRONSIDE_CLIENT_PSA_ABI_ALGORITHM_SIZE);
ASSERT_SCALAR(psa_key_derivation_step_t, IRONSIDE_CLIENT_PSA_ABI_DERIVATION_STEP_SIZE);

#define ASSERT_KEY_ATTRIBUTE(member, offset, size)                          \
	BUILD_ASSERT(offsetof(psa_key_attributes_t, member) == (offset) &&  \
		     sizeof(((psa_key_attributes_t *)0)->member) == (size), \
		     "key attributes " #member " layout")

BUILD_ASSERT(sizeof(psa_key_attributes_t) == IRONSIDE_CLIENT_PSA_ABI_KEY_ATTRIBUTES_SIZE,
	     "key attributes size");
BUILD_ASSERT(_Alignof(psa_key_attributes_t) == IRONSIDE_CLIENT_PSA_ABI_KEY_ATTRIBUTES_ALIGN,
	     "key attributes alignment");
ASSERT_KEY_ATTRIBUTE(MBEDTLS_PRIVATE(type),
		     IRONSIDE_CLIENT_PSA_ABI_KEY_ATTRIBUTES_TYPE_OFFSET,
		     IRONSIDE_CLIENT_PSA_ABI_KEY_TYPE_SIZE);
ASSERT_KEY_ATTRIBUTE(MBEDTLS_PRIVATE(bits),
		     IRONSIDE_CLIENT_PSA_ABI_KEY_ATTRIBUTES_BITS_OFFSET,
		     IRONSIDE_CLIENT_PSA_ABI_KEY_BITS_SIZE);
ASSERT_KEY_ATTRIBUTE(MBEDTLS_PRIVATE(lifetime),
		     IRONSIDE_CLIENT_PSA_ABI_KEY_ATTRIBUTES_LIFETIME_OFFSET,
		     IRONSIDE_CLIENT_PSA_ABI_KEY_LIFETIME_SIZE);
ASSERT_KEY_ATTRIBUTE(MBEDTLS_PRIVATE(policy).MBEDTLS_PRIVATE(usage),
		     IRONSIDE_CLIENT_PSA_ABI_KEY_ATTRIBUTES_USAGE_OFFSET,
		     IRONSIDE_CLIENT_PSA_ABI_KEY_USAGE_SIZE);
ASSERT_KEY_ATTRIBUTE(MBEDTLS_PRIVATE(policy).MBEDTLS_PRIVATE(alg),
		     IRONSIDE_CLIENT_PSA_ABI_KEY_ATTRIBUTES_ALG_OFFSET,
		     IRONSIDE_CLIENT_PSA_ABI_ALGORITHM_SIZE);
ASSERT_KEY_ATTRIBUTE(MBEDTLS_PRIVATE(policy).MBEDTLS_PRIVATE(alg2),
		     IRONSIDE_CLIENT_PSA_ABI_KEY_ATTRIBUTES_ALG2_OFFSET,
		     IRONSIDE_CLIENT_PSA_ABI_ALGORITHM_SIZE);
ASSERT_KEY_ATTRIBUTE(MBEDTLS_PRIVATE(id), IRONSIDE_CLIENT_PSA_ABI_KEY_ATTRIBUTES_ID_OFFSET,
		     IRONSIDE_CLIENT_PSA_ABI_KEY_ID_SIZE);

#define ASSERT_PAKE_CIPHER_SUITE(member, offset)                             \
	BUILD_ASSERT(offsetof(psa_pake_cipher_suite_t, member) == (offset), \
		     "PAKE cipher suite " #member " offset")

BUILD_ASSERT(sizeof(psa_pake_cipher_suite_t) == IRONSIDE_CLIENT_PSA_ABI_PAKE_CIPHER_SUITE_SIZE,
	     "PAKE cipher suite size");
BUILD_ASSERT(_Alignof(psa_pake_cipher_suite_t) == IRONSIDE_CLIENT_PSA_ABI_PAKE_CIPHER_SUITE_ALIGN,
	     "PAKE cipher suite alignment");
ASSERT_PAKE_CIPHER_SUITE(algorithm, IRONSIDE_CLIENT_PSA_ABI_PAKE_CIPHER_SUITE_ALGORITHM_OFFSET);
ASSERT_PAKE_CIPHER_SUITE(type, IRONSIDE_CLIENT_PSA_ABI_PAKE_CIPHER_SUITE_TYPE_OFFSET);
ASSERT_PAKE_CIPHER_SUITE(family, IRONSIDE_CLIENT_PSA_ABI_PAKE_CIPHER_SUITE_FAMILY_OFFSET);
ASSERT_PAKE_CIPHER_SUITE(bits, IRONSIDE_CLIENT_PSA_ABI_PAKE_CIPHER_SUITE_BITS_OFFSET);
ASSERT_PAKE_CIPHER_SUITE(key_confirmation,
			 IRONSIDE_CLIENT_PSA_ABI_PAKE_CIPHER_SUITE_KEY_CONFIRMATION_OFFSET);

#define ASSERT_OPERATION_LAYOUT(type)                                           \
	BUILD_ASSERT(sizeof(type) == IRONSIDE_CLIENT_PSA_ABI_OPERATION_SIZE &&  \
		     _Alignof(type) == IRONSIDE_CLIENT_PSA_ABI_OPERATION_ALIGN, \
		     #type " layout")

ASSERT_OPERATION_LAYOUT(psa_hash_operation_t);
ASSERT_OPERATION_LAYOUT(psa_mac_operation_t);
ASSERT_OPERATION_LAYOUT(psa_cipher_operation_t);
ASSERT_OPERATION_LAYOUT(psa_aead_operation_t);
ASSERT_OPERATION_LAYOUT(psa_key_derivation_operation_t);
ASSERT_OPERATION_LAYOUT(psa_pake_operation_t);
