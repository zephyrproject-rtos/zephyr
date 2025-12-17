/*
 * Copyright (c) 2024 Kelly Helmut Lord
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <zephyr/data/cobs.h>

static inline uint8_t cobs_code_base(uint8_t delimiter)
{
	return (delimiter == 0x01) ? 2 : 1;
}

static inline uint8_t cobs_finish_code(uint8_t delimiter)
{
	if (delimiter <= 0x01) {
		return 0xFF;
	}
	return delimiter - 1;
}

int cobs_encode(struct net_buf *src, struct net_buf *dst, uint32_t flags)
{
	uint8_t delimiter = COBS_FLAG_CUSTOM_DELIMITER(flags);
	uint8_t code_base = cobs_code_base(delimiter);
	uint8_t finish_code = cobs_finish_code(delimiter);
	size_t max_encoded_size = cobs_max_encoded_len(src->len, flags);

	if (net_buf_tailroom(dst) < max_encoded_size) {
		return -ENOMEM;
	}

	uint8_t *code_ptr = net_buf_add(dst, 1);
	uint8_t code = code_base;
	uint8_t data;

	while (src->len > 0) {
		data = net_buf_pull_u8(src);
		if (data == delimiter) {
			*code_ptr = code;
			code_ptr = net_buf_add(dst, 1);
			code = code_base;
		} else {
			net_buf_add_u8(dst, data);
			code++;
			if (code == finish_code && src->len > 0) {
				*code_ptr = finish_code;
				code_ptr = net_buf_add(dst, 1);
				code = code_base;
			}
		}
	}

	*code_ptr = code;

	if (flags & COBS_FLAG_TRAILING_DELIMITER) {
		net_buf_add_u8(dst, delimiter);
	}

	return 0;
}

int cobs_decode(struct net_buf *src, struct net_buf *dst, uint32_t flags)
{
	uint8_t delimiter = COBS_FLAG_CUSTOM_DELIMITER(flags);
	uint8_t code_base = cobs_code_base(delimiter);
	uint8_t finish_code = cobs_finish_code(delimiter);

	if (flags & COBS_FLAG_TRAILING_DELIMITER) {
		if (net_buf_remove_u8(src) != delimiter) {
			return -EINVAL;
		}
	}

	while (src->len > 0) {
		uint8_t offset = net_buf_pull_u8(src);

		if (offset == delimiter || offset < code_base) {
			return -EINVAL;
		}

		uint8_t bytes_to_read = offset - code_base;

		if (src->len < bytes_to_read) {
			return -EINVAL;
		}

		for (uint8_t i = 0; i < bytes_to_read; i++) {
			uint8_t byte = net_buf_pull_u8(src);

			if (byte == delimiter) {
				return -EINVAL;
			}
			net_buf_add_u8(dst, byte);
		}

		if (offset != finish_code && src->len > 0) {
			net_buf_add_u8(dst, delimiter);
		}
	}

	return 0;
}
