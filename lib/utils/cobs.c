/*
 * Copyright (c) 2024 Kelly Helmut Lord
 * Copyright (c) 2025 Martin Schröder
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/data/cobs.h>

/* Block-based Encoding */

int cobs_encode(struct net_buf *src, struct net_buf *dst, uint32_t flags)
{
	uint8_t delimiter = COBS_FLAG_CUSTOM_DELIMITER(flags);
	size_t max_encoded_size = cobs_max_encoded_len(src->len, flags);
	if (net_buf_tailroom(dst) < max_encoded_size) {
		return -ENOMEM;
	}

	/* Empty input encodes as {0x01} */
	if (src->len == 0) {
		net_buf_add_u8(dst, 0x01);
		if ((flags & COBS_FLAG_TRAILING_DELIMITER) != 0U) {
			net_buf_add_u8(dst, delimiter);
		}
		return 0;
	}

	/* Use streaming encoder */
	struct cobs_encode_state state = {0};

	cobs_encode_init(&state);

	uint8_t *encode_buf = net_buf_tail(dst);
	size_t encode_capacity = net_buf_tailroom(dst);
	size_t encoded_len = encode_capacity;

	int ret = cobs_encode_stream(&state, src, encode_buf, &encoded_len, delimiter);

	if (ret != 0) {
		return ret;
	}

	/* Finalize encoding */
	size_t finalize_capacity = encode_capacity - encoded_len;
	size_t finalized_len = finalize_capacity;

	ret = cobs_encode_finalize(&state, encode_buf + encoded_len,
				   &finalized_len, delimiter);
	if (ret != 0) {
		return ret;
	}

	net_buf_add(dst, encoded_len + finalized_len);
	net_buf_pull(src, src->len);

	if ((flags & COBS_FLAG_TRAILING_DELIMITER) != 0U) {
		net_buf_add_u8(dst, delimiter);
	}

	return 0;
}

/* Block-based Decoding */

int cobs_decode(struct net_buf *src, struct net_buf *dst, uint32_t flags)
{
	uint8_t delimiter = COBS_FLAG_CUSTOM_DELIMITER(flags);

	/* Handle trailing delimiter if present */
	if ((flags & COBS_FLAG_TRAILING_DELIMITER) != 0U) {
		if (src->len == 0) {
			return -EINVAL;
		}
		uint8_t end_delim = net_buf_remove_u8(src);
		if (end_delim != delimiter) {
			return -EINVAL;
		}
	}

	/* Add synthetic delimiter for streaming decoder */
	struct cobs_decode_state state = {0};

	cobs_decode_init(&state);

	uint8_t temp_buf[src->len + 1];

	memcpy(temp_buf, src->data, src->len);
	temp_buf[src->len] = delimiter;

	int ret = cobs_decode_stream(&state, temp_buf, src->len + 1, dst, delimiter);

	if (ret < 0) {
		return ret;
	}

	if (state.frame_complete == false || ret < (ssize_t)src->len) {
		return -EINVAL;
	}

	net_buf_pull(src, src->len);
	return 0;
}

/* Streaming Encoder */

void cobs_encode_init(struct cobs_encode_state *self)
{
	__ASSERT(self != NULL, "self must not be NULL");

	self->src_frag = NULL;
	self->src_offset = 0;
	self->block_code = 0;
	self->block_pos = 0;
}

/* Skip empty fragments and peek at current byte */
static inline bool peek_byte(struct cobs_encode_state *self, uint8_t *byte_out)
{
	while ((self->src_frag != NULL) && (self->src_offset >= self->src_frag->len)) {
		self->src_frag = self->src_frag->frags;
		self->src_offset = 0;
	}

	if (self->src_frag == NULL || self->src_offset >= self->src_frag->len) {
		return false;
	}

	*byte_out = self->src_frag->data[self->src_offset];
	return true;
}

static inline void advance_byte(struct cobs_encode_state *self)
{
	self->src_offset++;
}

/* Scan ahead to find code byte value */
static uint8_t scan_for_code_byte(const struct cobs_encode_state *self,
				  const uint8_t delimiter)
{
	struct net_buf *frag = self->src_frag;
	size_t offset = self->src_offset;
	uint8_t count = 1;

	while ((frag != NULL) && (count < 0xFF)) {
		while (offset < frag->len && count < 0xFF) {
			if (frag->data[offset] == delimiter) {
				return count;
			}
			offset++;
			count++;
		}

		if (offset >= frag->len) {
			frag = frag->frags;
			offset = 0;
		}
	}

	return count;
}

int cobs_encode_stream(struct cobs_encode_state *self, struct net_buf *src,
		       uint8_t *dst, size_t *dst_len, uint8_t delimiter)
{
	if (self == NULL || src == NULL || dst == NULL || dst_len == NULL) {
		return -EINVAL;
	}

	const size_t capacity = *dst_len;
	size_t written = 0;
	bool last_block_ended_with_delimiter = false;

	/* Initialize source on first call */
	if (self->src_frag == NULL) {
		self->src_frag = src;
		self->src_offset = 0;
	}

	uint8_t byte;

	while ((peek_byte(self, &byte) != false) && (written < capacity)) {
		/* Start new block if needed */
		if (self->block_pos == 0) {
			if (written >= capacity) {
				break;
			}
			self->block_code = scan_for_code_byte(self, delimiter);
			dst[written++] = self->block_code;
		}

		/* Copy data bytes for this block */
		while ((peek_byte(self, &byte) != false) &&
		       (self->block_pos < self->block_code - 1) &&
		       (written < capacity)) {

			if (byte == delimiter) {
				advance_byte(self);
				self->block_pos = 0;
				break;
			}

			dst[written++] = byte;
			advance_byte(self);
			self->block_pos++;
		}

		/* Check if block is complete */
		if (self->block_pos == self->block_code - 1) {
			last_block_ended_with_delimiter = false;

			if ((self->block_code != 0xFF) && (peek_byte(self, &byte) != false) &&
			    (byte == delimiter)) {
				advance_byte(self);
				last_block_ended_with_delimiter = true;
			}
			self->block_pos = 0;
		}
	}

	/* Store whether we need final code byte in finalize */
	self->src_frag = (last_block_ended_with_delimiter != false) ? src : NULL;

	*dst_len = written;
	return 0;
}

int cobs_encode_finalize(struct cobs_encode_state *self, uint8_t *dst, size_t *dst_len,
			 uint8_t delimiter)
{
	if (self == NULL || dst == NULL || dst_len == NULL) {
		return -EINVAL;
	}

	size_t written = 0;
	size_t capacity = *dst_len;

	/* Write final code byte if last block ended with delimiter */
	if (self->src_frag != NULL) {
		if (capacity < 1) {
			return -ENOMEM;
		}
		dst[written++] = 0x01;
	}

	/* Reset state */
	self->block_code = 0;
	self->block_pos = 0;
	self->src_frag = NULL;
	self->src_offset = 0;

	*dst_len = written;
	return 0;
}

/* Streaming Decoder */

void cobs_decode_init(struct cobs_decode_state *self)
{
	__ASSERT(self != NULL, "self must not be NULL");

	self->bytes_left = 0;
	self->need_delimiter = false;
	self->frame_complete = false;
}

static inline int check_dst_space(const struct net_buf *dst)
{
	return (net_buf_tailroom(dst) < 1) ? -ENOMEM : 0;
}

/* Process code byte and insert delimiter if needed */
static inline int process_code_byte(struct cobs_decode_state *self,
				    const uint8_t *src, size_t *processed,
				    struct net_buf *dst,
				    const uint8_t delimiter)
{
	/* Insert pending delimiter before reading new code */
	if (self->need_delimiter != false) {
		if (src[*processed] == delimiter) {
			(*processed)++;
			self->need_delimiter = false;
			self->frame_complete = true;
			return 1;
		}

		int ret = check_dst_space(dst);

		if (ret != 0) {
			return ret;
		}

		net_buf_add_u8(dst, delimiter);
		self->need_delimiter = false;
	}

	/* Read code byte */
	uint8_t code = src[(*processed)++];

	if (code == delimiter) {
		self->frame_complete = true;
		return 1;
	}

	if (code == 0) {
		return -EINVAL;
	}

	self->bytes_left = code - 1;
	self->need_delimiter = (code != 0xFF);

	return 0;
}

int cobs_decode_stream(struct cobs_decode_state *self, const uint8_t *src,
		       size_t src_len, struct net_buf *dst, uint8_t delimiter)
{
	if (self == NULL || src == NULL || dst == NULL) {
		return -EINVAL;
	}

	size_t processed = 0;
	self->frame_complete = false;

	while (processed < src_len) {
		/* Read new code byte if needed */
		if (self->bytes_left == 0) {
			int ret = process_code_byte(self, src, &processed, dst, delimiter);
			if (ret == 1) {
				return (ssize_t)processed;
			}
			if (ret < 0) {
				return ret;
			}
		}

		/* Copy data bytes from block */
		while ((self->bytes_left > 0) && (processed < src_len)) {
			uint8_t byte = src[processed++];

			if (byte == delimiter) {
				return -EINVAL;
			}

			int ret = check_dst_space(dst);
			if (ret != 0) {
				return ret;
			}

			net_buf_add_u8(dst, byte);
			self->bytes_left--;
		}
	}

	return (ssize_t)processed;
}
