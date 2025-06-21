/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raspberrypi_pico_sha256

#include <zephyr/crypto/crypto.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys/byteorder.h>

#include <pico/bootrom/lock.h>
#include <pico/sha256.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sha_rpi_pico_sha256, CONFIG_CRYPTO_LOG_LEVEL);

struct rpi_pico_sha256_data {
	pico_sha256_state_t state;
};

static int rpi_pico_sha256_hash_handler(struct hash_ctx *ctx, struct hash_pkt *pkt, bool finish)
{
	struct rpi_pico_sha256_data *data = ctx->device->data;

	assert(data->state.locked);

	data->state.cache_used = 0;
	data->state.cache.word = 0;
	data->state.total_data_size = 0;

	sha256_err_not_ready_clear();
	sha256_set_bswap(true);
	sha256_start();

	pico_sha256_update(&data->state, pkt->in_buf, pkt->in_len);

	if (!finish) {
		return 0;
	}

	pico_sha256_write_padding(&data->state);
	sha256_wait_valid_blocking();

	for (uint i = 0; i < 8; i++) {
		((uint32_t *)pkt->out_buf)[i] = BSWAP_32((uint32_t)sha256_hw->sum[i]);
	}

	return 0;
}

static int rpi_pico_sha256_query_hw_caps(const struct device *dev)
{
	return (CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS);
}

static int rpi_pico_sha256_hash_begin_session(const struct device *dev, struct hash_ctx *ctx,
					      enum hash_algo algo)
{
	struct rpi_pico_sha256_data *data = dev->data;

	assert(!data->state.locked);

	if (algo != CRYPTO_HASH_ALGO_SHA256) {
		LOG_ERR("Unsupported algo");
		return -EINVAL;
	}

	if (ctx->flags & ~(rpi_pico_sha256_query_hw_caps(dev))) {
		LOG_ERR("Unsupported flag");
		return -EINVAL;
	}

	if (!bootrom_try_acquire_lock(BOOTROM_LOCK_SHA_256)) {
		return -EBUSY;
	}

	data->state.locked = true;
	ctx->hash_hndlr = rpi_pico_sha256_hash_handler;

	return 0;
}

static int rpi_pico_sha256_hash_session_free(const struct device *dev, struct hash_ctx *ctx)
{
	struct rpi_pico_sha256_data *data = dev->data;

	assert(data->state.locked);

	bootrom_release_lock(BOOTROM_LOCK_SHA_256);
	data->state.locked = false;

	return 0;
}

static DEVICE_API(crypto, rpi_pico_sha256_crypto_api) = {
	.query_hw_caps = rpi_pico_sha256_query_hw_caps,
	.hash_begin_session = rpi_pico_sha256_hash_begin_session,
	.hash_free_session = rpi_pico_sha256_hash_session_free,
};

#define RPI_PICO_SHA256_INIT(idx)                                                                  \
	static struct rpi_pico_sha256_data rpi_pico_sha256_##idx##_data;                           \
	DEVICE_DT_INST_DEFINE(idx, NULL, NULL, &rpi_pico_sha256_##idx##_data, NULL, POST_KERNEL,   \
			      CONFIG_CRYPTO_INIT_PRIORITY, &rpi_pico_sha256_crypto_api);

DT_INST_FOREACH_STATUS_OKAY(RPI_PICO_SHA256_INIT)
