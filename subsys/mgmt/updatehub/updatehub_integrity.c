/*
 * Copyright (c) 2023 O.S.Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(updatehub, CONFIG_UPDATEHUB_LOG_LEVEL);

#include "updatehub_integrity.h"

int updatehub_integrity_init(updatehub_crypto_context_t *ctx)
{
	int ret;

	if (ctx == NULL) {
		LOG_DBG("Invalid integrity context");
		return -EINVAL;
	}

	*ctx = psa_hash_operation_init();
	ret = psa_hash_setup(ctx, PSA_ALG_SHA_256);
	if (ret != PSA_SUCCESS) {
		LOG_DBG("Failed to %s SHA-256 operation. (%d)", "set up", ret);
		return -EFAULT;
	}

	return 0;
}

int updatehub_integrity_update(updatehub_crypto_context_t *ctx,
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

	ret = psa_hash_update(ctx, buffer, len);
	if (ret != PSA_SUCCESS) {
		psa_hash_abort(ctx);
		LOG_DBG("Failed to %s SHA-256 operation. (%d)", "update", ret);
		return -EFAULT;
	}

	return 0;
}

int updatehub_integrity_finish(updatehub_crypto_context_t *ctx,
			       uint8_t *hash, const uint32_t size)
{
	size_t hash_len;
	int ret;

	if (ctx == NULL || hash == NULL) {
		return -EINVAL;
	}

	if (size < SHA256_BIN_DIGEST_SIZE) {
		LOG_DBG("HASH input buffer is to small to store the message digest");
		return -EINVAL;
	}

	ret = psa_hash_finish(ctx, hash, size, &hash_len);
	if (ret != PSA_SUCCESS) {
		LOG_DBG("Failed to %s SHA-256 operation. (%d)", "finish", ret);
		psa_hash_abort(ctx);
		return -EFAULT;
	}

	return 0;
}
