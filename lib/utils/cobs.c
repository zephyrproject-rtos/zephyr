/*
 * Copyright (c) 2024 Kelly Helmut Lord
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <zephyr/data/cobs.h>

int cobs_encode(struct net_buf *src, struct net_buf *dst, uint32_t flags)
{
	uint8_t delimiter = COBS_FLAG_CUSTOM_DELIMITER(flags);

	/* Calculate required space for worst case */
	size_t max_encoded_size = cobs_max_encoded_len(src->len, flags);

	/* Check if destination has enough space */
	if (net_buf_tailroom(dst) < max_encoded_size) {
		return -ENOMEM;
	}

	uint8_t *code_ptr = net_buf_add(dst, 1);
	uint8_t code = 1;

	/* Process all input bytes */
	uint8_t data = 0;

	while (src->len > 0) {
		data = net_buf_pull_u8(src);
		if (data == delimiter) {
			/* Delimiter found - write current code and start new block */
			*code_ptr = code;
			code_ptr = net_buf_add(dst, 1);
			code = 1;
		} else {
			/* Add non-zero byte to output */
			net_buf_add_u8(dst, data);
			code++;

			/* If we've reached maximum block size, start a new block */
			if (code == 0xFF && (src->len - 1 >= 0)) {
				*code_ptr = code;
				code_ptr = net_buf_add(dst, 1);
				code = 1;
			}
		}
	}

	*code_ptr = code;

	if (flags & COBS_FLAG_TRAILING_DELIMITER) {
		/* Add final delimiter */
		net_buf_add_u8(dst, delimiter);
	}

	return 0;
}

int cobs_decode(struct net_buf *src, struct net_buf *dst, uint32_t flags)
{
	uint8_t delimiter = COBS_FLAG_CUSTOM_DELIMITER(flags);

	if (flags & COBS_FLAG_TRAILING_DELIMITER) {
		uint8_t end_delim = net_buf_remove_u8(src);

		if (end_delim != delimiter) {
			return -EINVAL;
		}
	}

	while (src->len > 0) {
		/* Pull the COBS offset byte */
		uint8_t offset = net_buf_pull_u8(src);

		if (offset == delimiter && !(flags & COBS_FLAG_TRAILING_DELIMITER)) {
			return -EINVAL;
		}

		/* Verify we have enough data */
		if (src->len < (offset - 1)) {
			return -EINVAL;
		}

		/* Copy offset-1 bytes */
		for (uint8_t i = 0; i < offset - 1; i++) {
			uint8_t byte = net_buf_pull_u8(src);

			if (byte == delimiter) {
				return -EINVAL;
			}
			net_buf_add_u8(dst, byte);
		}

		/* If this wasn't a maximum offset and we have more data,
		 * there was a delimiter here in the original data
		 */
		if (offset != 0xFF && src->len > 0) {
			net_buf_add_u8(dst, delimiter);
		}
	}

	return 0;
}
