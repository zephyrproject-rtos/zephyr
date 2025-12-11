/*
 * Copyright (c) 2023 O.S.Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __UPDATEHUB_INTEGRITY_H__
#define __UPDATEHUB_INTEGRITY_H__

#include <psa/crypto.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SHA256_BIN_DIGEST_SIZE	PSA_HASH_LENGTH(PSA_ALG_SHA_256)
#define SHA256_HEX_DIGEST_SIZE	((SHA256_BIN_DIGEST_SIZE * 2) + 1)

int updatehub_integrity_init(psa_hash_operation_t *ctx);
int updatehub_integrity_update(psa_hash_operation_t *ctx,
			       const uint8_t *buffer, const uint32_t len);
int updatehub_integrity_finish(psa_hash_operation_t *ctx,
			       uint8_t *hash, const uint32_t size);

#ifdef __cplusplus
}
#endif

#endif /* __UPDATEHUB_INTEGRITY_H__ */
