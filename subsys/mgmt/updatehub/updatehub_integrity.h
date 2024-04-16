/*
 * Copyright (c) 2023 O.S.Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __UPDATEHUB_INTEGRITY_H__
#define __UPDATEHUB_INTEGRITY_H__

#if defined(CONFIG_FLASH_AREA_CHECK_INTEGRITY_MBEDTLS)
#include <mbedtls/md.h>
#elif defined(CONFIG_FLASH_AREA_CHECK_INTEGRITY_TC)
#include <tinycrypt/sha256.h>
#include <tinycrypt/constants.h>
#else
#error "Integrity check method not defined"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define SHA256_BIN_DIGEST_SIZE	(32)
#define SHA256_HEX_DIGEST_SIZE	((SHA256_BIN_DIGEST_SIZE * 2) + 1)

struct updatehub_crypto_context {
#if defined(CONFIG_FLASH_AREA_CHECK_INTEGRITY_MBEDTLS)
	mbedtls_md_context_t md_ctx;
	const mbedtls_md_info_t *md_info;
#elif defined(CONFIG_FLASH_AREA_CHECK_INTEGRITY_TC)
	struct tc_sha256_state_struct sha256sum;
#endif
};

int updatehub_integrity_init(struct updatehub_crypto_context *ctx);
int updatehub_integrity_update(struct updatehub_crypto_context *ctx,
			       const uint8_t *buffer, const uint32_t len);
int updatehub_integrity_finish(struct updatehub_crypto_context *ctx,
			       uint8_t *hash, const uint32_t size);

#ifdef __cplusplus
}
#endif

#endif /* __UPDATEHUB_INTEGRITY_H__ */
