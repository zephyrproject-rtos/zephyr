/*
 * Copyright (c) 2023 O.S.Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(updatehub, CONFIG_UPDATEHUB_LOG_LEVEL);

#include "updatehub_integrity.h"

int updatehub_integrity_init(struct updatehub_crypto_context *ctx)
{
	int ret;

	if (ctx == NULL) {
		LOG_DBG("Invalid integrity context");
		return -EINVAL;
	}

	memset(ctx, 0, sizeof(struct updatehub_crypto_context));

#if defined(CONFIG_FLASH_AREA_CHECK_INTEGRITY_MBEDTLS)
	ctx->md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
	if (ctx->md_info == NULL) {
		LOG_DBG("Message Digest not found or not enabled");
		return -ENOENT;
	}

	mbedtls_md_init(&ctx->md_ctx);
	ret = mbedtls_md_setup(&ctx->md_ctx, ctx->md_info, 0);
	if (ret == MBEDTLS_ERR_MD_BAD_INPUT_DATA) {
		LOG_DBG("Bad Message Digest selected");
		return -EFAULT;
	}
	if (ret == MBEDTLS_ERR_MD_ALLOC_FAILED) {
		LOG_DBG("Failed to allocate memory");
		return -ENOMEM;
	}

	ret = mbedtls_md_starts(&ctx->md_ctx);
	if (ret == MBEDTLS_ERR_MD_BAD_INPUT_DATA) {
		LOG_DBG("Bad Message Digest selected");
		return -EFAULT;
	}
#elif defined(CONFIG_FLASH_AREA_CHECK_INTEGRITY_TC)
	ret = tc_sha256_init(&ctx->sha256sum);
	if (ret != TC_CRYPTO_SUCCESS) {
		LOG_DBG("Invalid integrity context");
		return -EFAULT;
	}
#endif

	return 0;
}

int updatehub_integrity_update(struct updatehub_crypto_context *ctx,
			       const uint8_t *buffer, const uint32_t len)
{
	int ret;

	if (ctx == NULL || buffer == NULL) {
		return -EINVAL;
	}

	/* bypass */
	if (len == 0) {
		return 0;
	}

#if defined(CONFIG_FLASH_AREA_CHECK_INTEGRITY_MBEDTLS)
	ret = mbedtls_md_update(&ctx->md_ctx, buffer, len);
	if (ret == MBEDTLS_ERR_MD_BAD_INPUT_DATA) {
		LOG_DBG("Bad Message Digest selected");
		return -EFAULT;
	}
#elif defined(CONFIG_FLASH_AREA_CHECK_INTEGRITY_TC)
	ret = tc_sha256_update(&ctx->sha256sum, buffer, len);
	if (ret != TC_CRYPTO_SUCCESS) {
		LOG_DBG("Invalid integrity context or invalid buffer");
		return -EFAULT;
	}
#endif

	return 0;
}

int updatehub_integrity_finish(struct updatehub_crypto_context *ctx,
			       uint8_t *hash, const uint32_t size)
{
	int ret;

	if (ctx == NULL || hash == NULL) {
		return -EINVAL;
	}

#if defined(CONFIG_FLASH_AREA_CHECK_INTEGRITY_MBEDTLS)
	if (size < mbedtls_md_get_size(ctx->md_info)) {
		LOG_DBG("HASH input buffer is to small to store the message digest");
		return -EINVAL;
	}

	ret = mbedtls_md_finish(&ctx->md_ctx, hash);
	if (ret == MBEDTLS_ERR_MD_BAD_INPUT_DATA) {
		LOG_DBG("Bad Message Digest selected");
		return -EFAULT;
	}

	mbedtls_md_free(&ctx->md_ctx);
#elif defined(CONFIG_FLASH_AREA_CHECK_INTEGRITY_TC)
	ret = tc_sha256_final(hash, &ctx->sha256sum);
	if (ret != TC_CRYPTO_SUCCESS) {
		LOG_DBG("Invalid integrity context or invalid hash pointer");
		return -EFAULT;
	}
#endif

	return 0;
}
