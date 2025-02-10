/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raspberrypi_pico_sha256

#include <zephyr/crypto/crypto.h>

#include <hardware/sha256.h>
#include <pico/bootrom/lock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sha_rpi_pico_sha256, CONFIG_CRYPTO_LOG_LEVEL);

/*
 * This is based on the pico_sha256 implementation in the Pico-SDK.
 */

/* We add one 0x80 byte, then 8 bytes for the size */
#define SHA256_PADDING_DATA_BYTES 9
#define SHA256_BLOCK_SIZE_BYTES   64

struct rpi_pico_sha256_data {
	bool locked;
	uint8_t cache_used;
	union {
		uint32_t word;
		uint8_t bytes[4];
	} cache;
	size_t total_data_size;
};

static void write_to_hardware(struct rpi_pico_sha256_data *state, const uint8_t *data,
			      size_t data_size_bytes)
{
	if (!state->cache_used && !(((uintptr_t)data) & 3u)) {
		/* aligned writes */
		while (data_size_bytes >= 4) {
			/* write a whole word */
			sha256_wait_ready_blocking();
			sha256_put_word(*(const uint32_t *)data);
			data += 4;
			data_size_bytes -= 4;
		}
	}

	while (data_size_bytes--) {
		state->cache.bytes[state->cache_used++] = *data++;
		if (state->cache_used == 4) {
			state->cache_used = 0;
			sha256_wait_ready_blocking();
			sha256_put_word(state->cache.word);
		}
	}
}

static inline size_t bytes_to_write(struct rpi_pico_sha256_data *state, size_t data_size_bytes)
{
	return ((state->total_data_size + (SHA256_BLOCK_SIZE_BYTES - 1)) &
		~(SHA256_BLOCK_SIZE_BYTES - 1)) -
	       state->total_data_size;
}

static void update_internal(struct rpi_pico_sha256_data *state, const uint8_t *data,
			    size_t data_size_bytes)
{
	/* must finish off the last 64 byte block first or else sha256_err_not_ready will be true */
	const size_t bytes_left = MIN(bytes_to_write(state, data_size_bytes), data_size_bytes);

	if (bytes_left > 0) {
		write_to_hardware(state, data, bytes_left);
		state->total_data_size += bytes_left;
		data_size_bytes -= bytes_left;
		data += bytes_left;
	}

	/* Write the rest of the data */
	if (data_size_bytes > 0) {
		write_to_hardware(state, data, data_size_bytes);
		state->total_data_size += data_size_bytes;
	}
}

static void add_zero_bytes(struct rpi_pico_sha256_data *state, size_t data_size_bytes)
{
	const uint8_t zero[] = {0, 0, 0, 0};

	while ((int32_t)data_size_bytes > 0) {
		update_internal(state, zero, MIN(4, data_size_bytes));
		data_size_bytes -= 4;
	}
}

static void write_padding(struct rpi_pico_sha256_data *state)
{
	/* Has to be a multiple of 64 bytes */
	uint64_t size = (state->total_data_size + SHA256_PADDING_DATA_BYTES +
			 (SHA256_BLOCK_SIZE_BYTES - 1)) &
			~(SHA256_BLOCK_SIZE_BYTES - 1);
	const size_t user_data_size = state->total_data_size;
	const size_t padding_size_bytes = size - state->total_data_size;
	const uint8_t one_bit = 0x80;

	/* append a single '1' bit */
	update_internal(state, &one_bit, 1);

	/* Zero unused padding */
	add_zero_bytes(state, padding_size_bytes - SHA256_PADDING_DATA_BYTES);

	/* Add size in bits, big endian */
	size = __builtin_bswap64(user_data_size * 8);
	update_internal(state, (uint8_t *)&size, sizeof(uint64_t)); /* last write */
}

static int rpi_pico_sha256_hash_handler(struct hash_ctx *ctx, struct hash_pkt *pkt, bool finish)
{
	struct rpi_pico_sha256_data *data = ctx->device->data;

	assert(data->locked);

	data->cache_used = 0;
	data->cache.word = 0;
	data->total_data_size = 0;

	sha256_err_not_ready_clear();
	sha256_set_bswap(true);
	sha256_start();

	update_internal(data, pkt->in_buf, pkt->in_len);

	if (!finish) {
		return 0;
	}

	write_padding(data);
	sha256_wait_valid_blocking();

	for (uint i = 0; i < 8; i++) {
		((uint32_t *)pkt->out_buf)[i] = __builtin_bswap32(sha256_hw->sum[i]);
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

	assert(!data->locked);

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

	data->locked = true;
	ctx->hash_hndlr = rpi_pico_sha256_hash_handler;

	return 0;
}

static int rpi_pico_sha256_hash_session_free(const struct device *dev, struct hash_ctx *ctx)
{
	struct rpi_pico_sha256_data *data = dev->data;

	assert(data->locked);

	bootrom_release_lock(BOOTROM_LOCK_SHA_256);
	data->locked = false;

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
