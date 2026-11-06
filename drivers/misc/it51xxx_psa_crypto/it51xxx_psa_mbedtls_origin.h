/*
 * Copyright The Mbed TLS Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IT51XXX_PSA_MBEDTLS_ORIGIN_H
#define IT51XXX_PSA_MBEDTLS_ORIGIN_H
#include "mbedtls/md.h"
#include "psa/crypto.h"

/* Construct a PKCS v1.5 encoding of a hashed message
 *
 * This is used both for signature generation and verification.
 *
 * Parameters:
 * - md_alg:  Identifies the hash algorithm used to generate the given hash;
 *            MBEDTLS_MD_NONE if raw data is signed.
 * - hashlen: Length of hash in case hashlen is MBEDTLS_MD_NONE.
 * - hash:    Buffer containing the hashed message or the raw data.
 * - dst_len: Length of the encoded message.
 * - dst:     Buffer to hold the encoded message.
 *
 * Assumptions:
 * - hash has size hashlen if md_alg == MBEDTLS_MD_NONE.
 * - hash has size corresponding to md_alg if md_alg != MBEDTLS_MD_NONE.
 * - dst points to a buffer of size at least dst_len.
 *
 */
int rsa_rsassa_pkcs1_v15_encode(mbedtls_md_type_t md_alg, unsigned int hashlen,
				const unsigned char *hash, size_t dst_len, unsigned char *dst);

#endif /* IT51XXX_PSA_MBEDTLS_ORIGIN_H */
