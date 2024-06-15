/*
 * Copyright (c) 2023 O.S.Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __UPDATEHUB_INTEGRITY_H__
#define __UPDATEHUB_INTEGRITY_H__

#if defined(CONFIG_PSA_CRYPTO_CLIENT)
#include <psa/crypto.h>
#else
#include <mbedtls/sha256.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define SHA256_BIN_DIGEST_SIZE	(32)
#define SHA256_HEX_DIGEST_SIZE	((SHA256_BIN_DIGEST_SIZE * 2) + 1)

#if defined(CONFIG_PSA_CRYPTO_CLIENT)
typedef psa_hash_operation_t updatehub_crypto_context_t;
#else
typedef mbedtls_sha256_context updatehub_crypto_context_t;
#endif

int updatehub_integrity_init(updatehub_crypto_context_t *ctx);
int updatehub_integrity_update(updatehub_crypto_context_t *ctx,
			       const uint8_t *buffer, const uint32_t len);
int updatehub_integrity_finish(updatehub_crypto_context_t *ctx,
			       uint8_t *hash, const uint32_t size);

#ifdef __cplusplus
}
#endif

#endif /* __UPDATEHUB_INTEGRITY_H__ */
