/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_bee_sha256

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/crypto/crypto.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <errno.h>

#if defined(CONFIG_SOC_SERIES_RTL87X2G)
#include <crypto_engine_nsc.h>
#elif defined(CONFIG_SOC_SERIES_RTL8752H)
#include <rtl876x_hw_sha256.h>
#endif

LOG_MODULE_REGISTER(crypto_bee_sha, CONFIG_CRYPTO_LOG_LEVEL);

struct bee_sha256_data {
	struct k_mutex lock;
};

static int bee_sha256_hash_handler(struct hash_ctx *ctx, struct hash_pkt *pkt, bool finish)
{
	struct bee_sha256_data *data = ctx->device->data;
	uint32_t result[8];
	uint8_t empty = 0U;
	uint8_t *input;
	bool ret;

	if (!finish) {
		LOG_ERR("Multipart SHA-256 is not supported");
		return -ENOTSUP;
	}

	if (!pkt || !pkt->out_buf || (!pkt->in_buf && pkt->in_len != 0U)) {
		return -EINVAL;
	}

	input = pkt->in_len == 0U ? &empty : (uint8_t *)pkt->in_buf;

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = hw_sha256(input, pkt->in_len, result, HW_SHA256_CPU_MODE);
	k_mutex_unlock(&data->lock);

	if (!ret) {
		return -EIO;
	}

	memcpy(pkt->out_buf, result, sizeof(result));

	return 0;
}

static int bee_sha256_query_hw_caps(const struct device *dev)
{
	ARG_UNUSED(dev);

	return CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS;
}

static int bee_sha256_hash_begin_session(const struct device *dev, struct hash_ctx *ctx,
					 enum hash_algo algo)
{
	if (algo != CRYPTO_HASH_ALGO_SHA256) {
		LOG_ERR("Unsupported hash algorithm %d", algo);
		return -ENOTSUP;
	}

	if ((ctx->flags & ~(CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS)) != 0U) {
		LOG_ERR("Unsupported hash flags 0x%x", ctx->flags);
		return -EINVAL;
	}

	ctx->hash_hndlr = bee_sha256_hash_handler;
	ctx->device = dev;
	ctx->drv_sessn_state = NULL;
	ctx->started = false;

	return 0;
}

static int bee_sha256_hash_free_session(const struct device *dev, struct hash_ctx *ctx)
{
	ARG_UNUSED(dev);

	ctx->hash_hndlr = NULL;
	ctx->drv_sessn_state = NULL;
	ctx->started = false;

	return 0;
}

static int bee_sha256_init(const struct device *dev)
{
	struct bee_sha256_data *data = dev->data;

	k_mutex_init(&data->lock);

	return 0;
}

static DEVICE_API(crypto, bee_sha256_crypto_api) = {
	.query_hw_caps = bee_sha256_query_hw_caps,
	.hash_begin_session = bee_sha256_hash_begin_session,
	.hash_free_session = bee_sha256_hash_free_session,
	.hash_async_callback_set = NULL,
};

#define BEE_SHA256_INIT(inst)                                                                      \
	static struct bee_sha256_data bee_sha256_data_##inst;                                      \
	DEVICE_DT_INST_DEFINE(inst, bee_sha256_init, NULL, &bee_sha256_data_##inst, NULL,          \
			      POST_KERNEL, CONFIG_CRYPTO_INIT_PRIORITY, &bee_sha256_crypto_api);

DT_INST_FOREACH_STATUS_OKAY(BEE_SHA256_INIT)
