/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raspberrypi_pico_sha256

#include <zephyr/crypto/crypto.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys/byteorder.h>

#include <pico/bootrom/lock.h>
#include <pico/sha256.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(crypto_rpi_pico_sha256, CONFIG_CRYPTO_LOG_LEVEL);

struct crypto_rpi_pico_sha256_data {
	pico_sha256_state_t state;
	struct k_spinlock lock;
};

static int crypto_rpi_pico_sha256_hash_handler(struct hash_ctx *ctx, struct hash_pkt *pkt,
					       bool finish)
{
	struct crypto_rpi_pico_sha256_data *data = ctx->device->data;

	if (!data->state.locked) {
		LOG_ERR("Invalid lock status: unlocked");
		return -EINVAL;
	}

	data->state.cache_used = 0;
	data->state.cache.word = 0;
	data->state.total_data_size = 0;

	sha256_err_not_ready_clear();
	sha256_set_bswap(true);
	sha256_start();

	pico_sha256_update(&data->state, pkt->in_buf, pkt->in_len);

	if (!finish) {
		LOG_ERR("Multipart hashing not supported yet");
		return -ENOTSUP;
	}

	pico_sha256_write_padding(&data->state);
	sha256_wait_valid_blocking();

	for (uint i = 0; i < 8; i++) {
		((uint32_t *)pkt->out_buf)[i] = BSWAP_32((uint32_t)sha256_hw->sum[i]);
	}

	return 0;
}

static int crypto_rpi_pico_sha256_query_hw_caps(const struct device *dev)
{
	return CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS;
}

static int crypto_rpi_pico_sha256_hash_begin_session(const struct device *dev, struct hash_ctx *ctx,
						     enum hash_algo algo)
{
	struct crypto_rpi_pico_sha256_data *data = dev->data;
	k_spinlock_key_t key;
	int ret;

	if (data->state.locked) {
		LOG_ERR("Invalid lock status: locked");
		return -EINVAL;
	}

	if (algo != CRYPTO_HASH_ALGO_SHA256) {
		LOG_ERR("Unsupported algo: %d", algo);
		return -EINVAL;
	}

	if (ctx->flags & ~(crypto_rpi_pico_sha256_query_hw_caps(dev))) {
		LOG_ERR("Unsupported flag %x", ctx->flags);
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	ret = bootrom_try_acquire_lock(BOOTROM_LOCK_SHA_256);
	if (!ret) {
		LOG_ERR("bootrom_try_acquire_lock failed");
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	data->state.locked = true;

	k_spin_unlock(&data->lock, key);

	ctx->hash_hndlr = crypto_rpi_pico_sha256_hash_handler;

	return 0;
}

static int crypto_rpi_pico_sha256_hash_session_free(const struct device *dev, struct hash_ctx *ctx)
{
	struct crypto_rpi_pico_sha256_data *data = dev->data;
	k_spinlock_key_t key;

	if (!data->state.locked) {
		LOG_ERR("Invalid lock status: unlocked");
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	bootrom_release_lock(BOOTROM_LOCK_SHA_256);
	data->state.locked = false;
	k_spin_unlock(&data->lock, key);

	return 0;
}

static DEVICE_API(crypto, crypto_rpi_pico_sha256_crypto_api) = {
	.query_hw_caps = crypto_rpi_pico_sha256_query_hw_caps,
	.hash_begin_session = crypto_rpi_pico_sha256_hash_begin_session,
	.hash_free_session = crypto_rpi_pico_sha256_hash_session_free,
};

#define CRYPTO_RPI_PICO_SHA256_INIT(idx)                                                           \
	static struct crypto_rpi_pico_sha256_data crypto_rpi_pico_sha256_##idx##_data;             \
	DEVICE_DT_INST_DEFINE(idx, NULL, NULL, &crypto_rpi_pico_sha256_##idx##_data, NULL,         \
			      POST_KERNEL, CONFIG_CRYPTO_INIT_PRIORITY,                            \
			      &crypto_rpi_pico_sha256_crypto_api);

DT_INST_FOREACH_STATUS_OKAY(CRYPTO_RPI_PICO_SHA256_INIT)
