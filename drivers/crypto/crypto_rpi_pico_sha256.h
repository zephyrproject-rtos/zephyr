/*
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * This file is forked from
 * https://github.com/raspberrypi/pico-sdk/blob/2.1.0/src/rp2_common/pico_sha256/sha256.c
 */

#ifndef ZEPHYR_DRIVERS_CRYPTO_CRYPTO_RPI_PICO_SHA256_H_
#define ZEPHYR_DRIVERS_CRYPTO_CRYPTO_RPI_PICO_SHA256_H_

#include <hardware/sha256.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys/byteorder.h>

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
	return ROUND_UP(state->total_data_size, SHA256_BLOCK_SIZE_BYTES) - state->total_data_size;
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

	uint64_t size = ROUND_UP(state->total_data_size + SHA256_PADDING_DATA_BYTES,
				 SHA256_BLOCK_SIZE_BYTES);
	const size_t user_data_size = state->total_data_size;
	const size_t padding_zeros = size - state->total_data_size - SHA256_PADDING_DATA_BYTES;
	const uint8_t one_bit = 0x80;

	/* append a single '1' bit */
	update_internal(state, &one_bit, 1);

	/* Zero unused padding */
	if (padding_zeros > 0) {
		add_zero_bytes(state, padding_zeros);
	}

	/* Add size in bits, big endian */
	size = BSWAP_64((uint64_t)(user_data_size * 8));
	update_internal(state, (uint8_t *)&size, sizeof(uint64_t)); /* last write */
}

#endif /* ZEPHYR_DRIVERS_CRYPTO_CRYPTO_RPI_PICO_SHA256_H_ */
