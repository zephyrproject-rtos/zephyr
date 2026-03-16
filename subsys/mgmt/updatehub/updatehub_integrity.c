/*
 * Copyright (c) 2023 O.S.Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(updatehub, CONFIG_UPDATEHUB_LOG_LEVEL);

#include "updatehub_integrity.h"

int updatehub_integrity_init(psa_hash_operation_t *ctx)
{
	psa_status_t status;

	if (ctx == NULL) {
		LOG_DBG("Invalid integrity context");
		return -EINVAL;
	}

	*ctx = psa_hash_operation_init();
	status = psa_hash_setup(ctx, PSA_ALG_SHA_256);
	if (status != PSA_SUCCESS) {
		LOG_DBG("Failed to %s SHA-256 operation. (%d)", "set up", status);
		return -EFAULT;
	}

	return 0;
}

int updatehub_integrity_update(psa_hash_operation_t *ctx,
			       const uint8_t *buffer, const uint32_t len)
{
	psa_status_t status;

	if (ctx == NULL || buffer == NULL) {
		return -EINVAL;
	}

	/* bypass */
	if (len == 0) {
		return 0;
	}

	status = psa_hash_update(ctx, buffer, len);
	if (status != PSA_SUCCESS) {
		psa_hash_abort(ctx);
		LOG_DBG("Failed to %s SHA-256 operation. (%d)", "update", status);
		return -EFAULT;
	}

	return 0;
}

int updatehub_integrity_finish(psa_hash_operation_t *ctx,
			       uint8_t *hash, const uint32_t size)
{
	psa_status_t status;
	size_t hash_len;

	if (ctx == NULL || hash == NULL) {
		return -EINVAL;
	}

	if (size < SHA256_BIN_DIGEST_SIZE) {
		LOG_DBG("HASH input buffer is to small to store the message digest");
		return -EINVAL;
	}

	status = psa_hash_finish(ctx, hash, size, &hash_len);
	if (status != PSA_SUCCESS) {
		psa_hash_abort(ctx);
		LOG_DBG("Failed to %s SHA-256 operation. (%d)", "finish", status);
		return -EFAULT;
	}

	return 0;
}
