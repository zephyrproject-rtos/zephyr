/*
 * Copyright (c) 2024 Kelly Helmut Lord
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/net_buf.h>

#include <zephyr/data/cobs.h>

/**
 * @brief COBS encode data from a net_buf into another net_buf
 *
 * @param src Source net_buf containing data to encode
 * @param dst Destination net_buf to store encoded data
 * @return int 0 on success, negative errno on failure
 */
int cobs_encode(struct net_buf *src, struct net_buf *dst)
{
	/* Calculate required space for worst case */
	size_t max_encoded_size = src->len + (src->len / 254) + 2;

	/* Check if destination has enough space */
	if (net_buf_tailroom(dst) < max_encoded_size) {
		return -ENOSPC;
	}

	uint8_t *code_ptr = net_buf_add(dst, 1);
	uint8_t code = 1;

	/* Process all input bytes */
	for (size_t i = 0; i < src->len; i++) {
		if (src->data[i] == 0) {
			/* Zero found - write current code and start new block */
			*code_ptr = code;
			code_ptr = net_buf_add(dst, 1);
			code = 1;
		} else {
			/* Add non-zero byte to output */
			net_buf_add_u8(dst, src->data[i]);
			code++;

			/* If we've reached maximum block size, start a new block */
			if (code == 0xFF && i != src->len - 1) {
				*code_ptr = code;
				code_ptr = net_buf_add(dst, 1);
				code = 1;
			}
		}
	}

	*code_ptr = code;

	/* Add final zero delimiter */
	net_buf_add_u8(dst, 0);

	return 0;
}

/**
 * @brief COBS decode data from a net_buf into another net_buf
 *
 * @param src Source net_buf containing COBS encoded data
 * @param dst Destination net_buf to store decoded data
 * @return int 0 on success, negative errno on failure
 */
int cobs_decode(struct net_buf *src, struct net_buf *dst)
{
	uint8_t *src_data = src->data;
	size_t src_len = src->len;
	size_t i = 0;

	while (i < src_len - 1) {
		uint8_t code = src_data[i++];

		if (code == 0) {
			return -EINVAL;
		}

		if (net_buf_tailroom(dst) < (code - 1)) {
			return -ENOMEM;
		}

		/* Copy data bytes */
		for (uint8_t j = 1; j < code; j++) {
			if (i >= src_len - 1) {
				return -EINVAL;
			}
			net_buf_add_u8(dst, src_data[i++]);
		}

		if (code < 0xFF && i < src_len - 1) {
			/* Add implicit zero */
			if (net_buf_tailroom(dst) < 1) {
				return -ENOMEM;
			}
			net_buf_add_u8(dst, 0);
		}
	}

	return 0;
}
