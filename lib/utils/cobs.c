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
#include <zephyr/data/cobs.h>

/* ========================================================================
 * Block-based Encoding Helpers
 * ======================================================================== */

/* Check if destination buffer has sufficient space */
static inline int validate_encode_space(const struct net_buf *src, 
					const struct net_buf *dst, 
					uint32_t flags)
{
	size_t max_encoded_size = cobs_max_encoded_len(src->len, flags);

	if (net_buf_tailroom(dst) < max_encoded_size) {
		return -ENOMEM;
	}
	
	return 0;
}

/* Handle delimiter found during encoding */
static inline void handle_delimiter(uint8_t **code_ptr, uint8_t *code, 
				    struct net_buf *dst)
{
	**code_ptr = *code;
	*code_ptr = net_buf_add(dst, 1);
	*code = 1;
}

/* Handle data byte during encoding */
static inline void handle_data_byte(uint8_t data, uint8_t **code_ptr, 
				    uint8_t *code, struct net_buf *dst,
				    const struct net_buf *src)
{
	net_buf_add_u8(dst, data);
	(*code)++;

	/* If we've reached maximum block size, start a new block */
	if (*code == 0xFF && src->len > 0) {
		handle_delimiter(code_ptr, code, dst);
	}
}

/* Add trailing delimiter if requested */
static inline void add_trailing_delimiter(struct net_buf *dst, uint32_t flags, 
					  uint8_t delimiter)
{
	if (flags & COBS_FLAG_TRAILING_DELIMITER) {
		net_buf_add_u8(dst, delimiter);
	}
}

int cobs_encode(struct net_buf *src, struct net_buf *dst, uint32_t flags)
{
	int ret = validate_encode_space(src, dst, flags);
	if (ret != 0) {
		return ret;
	}

	uint8_t delimiter = COBS_FLAG_CUSTOM_DELIMITER(flags);
	uint8_t *code_ptr = net_buf_add(dst, 1);
	uint8_t code = 1;

	/* Process all input bytes */
	while (src->len > 0) {
		uint8_t data = net_buf_pull_u8(src);
		
		if (data == delimiter) {
			handle_delimiter(&code_ptr, &code, dst);
		} else {
			handle_data_byte(data, &code_ptr, &code, dst, src);
		}
	}

	*code_ptr = code;
	add_trailing_delimiter(dst, flags, delimiter);

	return 0;
}

/* ========================================================================
 * Block-based Decoding Helpers
 * ======================================================================== */

/* Validate and remove trailing delimiter if present */
static inline int validate_trailing_delimiter(struct net_buf *src, 
					      const uint32_t flags,
					      const uint8_t delimiter)
{
	if (!(flags & COBS_FLAG_TRAILING_DELIMITER)) {
		return 0;
	}

	uint8_t end_delim = net_buf_remove_u8(src);
	if (end_delim != delimiter) {
		return -EINVAL;
	}
	
	return 0;
}

/* Validate offset byte */
static inline int validate_offset(const uint8_t offset, const uint8_t delimiter, 
				  const uint32_t flags)
{
	if (offset == delimiter && !(flags & COBS_FLAG_TRAILING_DELIMITER)) {
		return -EINVAL;
	}
	return 0;
}

/* Verify sufficient data available for block */
static inline int verify_block_data(const struct net_buf *src, uint8_t offset)
{
	if (src->len < (offset - 1)) {
		return -EINVAL;
	}
	return 0;
}

/* Copy data bytes from encoded block */
static inline int copy_block_data_bytes(struct net_buf *src, struct net_buf *dst,
					uint8_t count, const uint8_t delimiter)
{
	for (uint8_t i = 0; i < count; i++) {
		uint8_t byte = net_buf_pull_u8(src);

		if (byte == delimiter) {
			return -EINVAL;
		}
		net_buf_add_u8(dst, byte);
	}
	
	return 0;
}

/* Insert delimiter between blocks if needed */
static inline void insert_delimiter_if_needed(struct net_buf *dst, uint8_t offset,
					      const struct net_buf *src,
					      const uint8_t delimiter)
{
	/* If this wasn't a maximum offset and we have more data,
	 * there was a delimiter here in the original data
	 */
	if (offset != 0xFF && src->len > 0) {
		net_buf_add_u8(dst, delimiter);
	}
}

int cobs_decode(struct net_buf *src, struct net_buf *dst, uint32_t flags)
{
	uint8_t delimiter = COBS_FLAG_CUSTOM_DELIMITER(flags);

	int ret = validate_trailing_delimiter(src, flags, delimiter);
	if (ret != 0) {
		return ret;
	}

	while (src->len > 0) {
		/* Pull the COBS offset byte */
		uint8_t offset = net_buf_pull_u8(src);

		ret = validate_offset(offset, delimiter, flags);
		if (ret != 0) {
			return ret;
		}

		ret = verify_block_data(src, offset);
		if (ret != 0) {
			return ret;
		}

		/* Copy offset-1 bytes */
		ret = copy_block_data_bytes(src, dst, offset - 1, delimiter);
		if (ret != 0) {
			return ret;
		}

		insert_delimiter_if_needed(dst, offset, src, delimiter);
	}

	return 0;
}

/* ========================================================================
 * Streaming Encoder Implementation
 * ======================================================================== */

void cobs_encode_init(struct cobs_encode_state *self)
{
	if (!self) {
		return;
	}

	self->src_frag = NULL;
	self->src_offset = 0;
	self->block_code = 0;
	self->block_pos = 0;
}

void cobs_encode_reset(struct cobs_encode_state *self)
{
	if (!self) {
		return;
	}

	self->src_frag = NULL;
	self->src_offset = 0;
	self->block_code = 0;
	self->block_pos = 0;
}

/* ========================================================================
 * Source Navigation Helpers
 * ======================================================================== */

/* Skip empty fragments to find next available data */
static inline void skip_empty_fragments(struct cobs_encode_state *self)
{
	while (self->src_frag && self->src_offset >= self->src_frag->len) {
		self->src_frag = self->src_frag->frags;
		self->src_offset = 0;
	}
}

/* Check if more source data is available */
static inline bool has_source_data(const struct cobs_encode_state *self)
{
	return (self->src_frag && self->src_offset < self->src_frag->len);
}

/* Peek at byte at current position without consuming */
static inline bool peek_byte(struct cobs_encode_state *self, uint8_t *byte_out)
{
	skip_empty_fragments(self);

	if (!has_source_data(self)) {
		return false;
	}

	*byte_out = self->src_frag->data[self->src_offset];
	return true;
}

/* Advance to next byte */
static inline void advance_byte(struct cobs_encode_state *self)
{
	self->src_offset++;
	skip_empty_fragments(self);
}

/* ========================================================================
 * Block Code Scanning
 * ======================================================================== */

/* Scan single fragment for delimiter or max bytes */
static inline uint8_t scan_fragment(const uint8_t *data, const size_t len, 
				    size_t offset, uint8_t count, 
				    const uint8_t delimiter)
{
	while (offset < len && count < 0xFF) {
		if (data[offset] == delimiter) {
			return count;
		}
		offset++;
		count++;
	}
	return count;
}

/* Scan ahead from current position to find code byte */
static uint8_t scan_for_code_byte(const struct cobs_encode_state *self, 
				  const uint8_t delimiter)
{
	struct net_buf *frag = self->src_frag;
	size_t offset = self->src_offset;
	uint8_t count = 1; /* Code byte is 1 + number of data bytes */

	/* Scan up to 254 bytes or until we find delimiter */
	while (frag && count < 0xFF) {
		count = scan_fragment(frag->data, frag->len, offset, 
				      count, delimiter);
		
		if (count < 0xFF || offset >= frag->len) {
			/* Found delimiter or exhausted fragment */
			if (offset < frag->len && frag->data[offset] == delimiter) {
				return count;
			}
			/* Move to next fragment */
			frag = frag->frags;
			offset = 0;
		}
	}
	
	return count;
}

/* ========================================================================
 * Encoding Operations
 * ======================================================================== */

/* Initialize source on first call */
static inline void init_source_if_needed(struct cobs_encode_state *self, 
					 struct net_buf *src)
{
	if (!self->src_frag) {
		self->src_frag = src;
		self->src_offset = 0;
	}
}

/* Check if we can write to destination */
static inline bool can_write(size_t written, size_t capacity)
{
	return written < capacity;
}

/* Start a new COBS block */
static inline int start_new_block(struct cobs_encode_state *self, uint8_t *dst,
				  size_t *written, size_t capacity, 
				  const uint8_t delimiter)
{
	if (!can_write(*written, capacity)) {
		return -ENOMEM;
	}

	self->block_code = scan_for_code_byte(self, delimiter);
	dst[(*written)++] = self->block_code;
	
	return 0;
}

/* Process a single data byte in current block */
static inline bool process_data_byte(struct cobs_encode_state *self, uint8_t byte,
				     uint8_t *dst, size_t *written, 
				     const uint8_t delimiter)
{
	if (byte == delimiter) {
		/* Found the delimiter that ends this block */
		advance_byte(self);
		self->block_pos = 0;
		return false; /* Block complete */
	}
	
	dst[(*written)++] = byte;
	advance_byte(self);
	self->block_pos++;
	
	return true; /* Continue processing */
}

/* Copy data bytes for current block */
static inline void copy_block_data(struct cobs_encode_state *self, uint8_t *dst,
				   size_t *written, size_t capacity, 
				   const uint8_t delimiter)
{
	uint8_t byte;
	
	while (peek_byte(self, &byte) && 
	       self->block_pos < self->block_code - 1 && 
	       can_write(*written, capacity)) {
		
		if (!process_data_byte(self, byte, dst, written, delimiter)) {
			break;
		}
	}
}

/* Finalize current block if complete */
static inline void finalize_block_if_complete(struct cobs_encode_state *self)
{
	uint8_t byte;
	
	if (self->block_pos == self->block_code - 1) {
		/* Block complete, skip delimiter if present */
		if (peek_byte(self, &byte) && byte == COBS_DEFAULT_DELIMITER) {
			advance_byte(self);
		}
		self->block_pos = 0;
	}
}

int cobs_encode_stream(struct cobs_encode_state *self, struct net_buf *src,
		       uint8_t *dst, size_t *dst_len)
{
	if (!self || !src || !dst || !dst_len) {
		return -EINVAL;
	}

	const uint8_t delimiter = COBS_DEFAULT_DELIMITER;
	const size_t capacity = *dst_len;
	size_t written = 0;

	init_source_if_needed(self, src);

	uint8_t byte;

	/* Process source data and encode into destination */
	while (peek_byte(self, &byte) && can_write(written, capacity)) {
		/* Starting a new block? */
		if (self->block_pos == 0) {
			int ret = start_new_block(self, dst, &written, 
						  capacity, delimiter);
			if (ret != 0) {
				break;
			}
		}

		/* Copy data bytes for this block */
		copy_block_data(self, dst, &written, capacity, delimiter);

		/* Check if block is complete */
		finalize_block_if_complete(self);
	}

	*dst_len = written;
	return 0;
}

int cobs_encode_finalize(struct cobs_encode_state *self, uint8_t *dst, size_t *dst_len)
{
	if (!self || !dst || !dst_len) {
		return -EINVAL;
	}

	/* With the streaming design where we scan ahead and pull from source,
	 * by the time the source is empty, all blocks are complete.
	 * Just reset state for next use.
	 */
	self->block_code = 0;
	self->block_pos = 0;
	*dst_len = 0;
	
	return 0;
}

/* ========================================================================
 * Streaming Decoder Implementation
 * ======================================================================== */

void cobs_decode_init(struct cobs_decode_state *self)
{
	if (!self) {
		return;
	}

	/* Start in ready-to-decode state. COBS frames always begin with a code byte
	 * (never a delimiter), so we can start decoding immediately. If we encounter
	 * invalid data or are mid-stream, the decoder will detect errors and resync.
	 */
	self->bytes_left = 0;
	self->need_delimiter = false;
	self->frame_complete = false;
}

void cobs_decode_reset(struct cobs_decode_state *self)
{
	if (!self) {
		return;
	}

	self->bytes_left = 0;
	self->need_delimiter = false;
	self->frame_complete = false;
}

/* ========================================================================
 * Decoding Operations
 * ======================================================================== */

/* Check if destination has space */
static inline int check_dst_space(const struct net_buf *dst)
{
	if (net_buf_tailroom(dst) < 1) {
		return -ENOMEM;
	}
	return 0;
}

/* Insert pending delimiter before next block */
static inline int insert_pending_delimiter(struct cobs_decode_state *self,
					   const uint8_t *src, size_t *processed,
					   struct net_buf *dst, 
					   const uint8_t delimiter)
{
	/* Peek at next byte */
	if (src[*processed] == delimiter) {
		/* Frame ends - don't insert delimiter */
		(*processed)++;
		self->need_delimiter = false;
		self->frame_complete = true;
		return 1; /* Signal frame complete */
	}
	
	/* Insert delimiter before next block */
	int ret = check_dst_space(dst);
	if (ret != 0) {
		return ret;
	}
	
	net_buf_add_u8(dst, delimiter);
	self->need_delimiter = false;
	
	return 0;
}

/* Read and validate code byte */
static inline int read_code_byte(struct cobs_decode_state *self, 
				 const uint8_t *src, size_t *processed,
				 const uint8_t delimiter)
{
	uint8_t code = src[(*processed)++];

	/* Check for frame delimiter */
	if (code == delimiter) {
		self->frame_complete = true;
		return 1; /* Signal frame complete */
	}

	/* Validate code byte */
	if (code == 0) {
		return -EINVAL;
	}

	/* Set up block processing */
	self->bytes_left = code - 1;
	self->need_delimiter = (code != 0xFF);
	
	return 0;
}

/* Process code byte (insert delimiter if needed, read new code) */
static inline int process_code_byte(struct cobs_decode_state *self,
				    const uint8_t *src, size_t *processed,
				    struct net_buf *dst, 
				    const uint8_t delimiter)
{
	/* Insert pending delimiter from previous block before reading code */
	if (self->need_delimiter) {
		int ret = insert_pending_delimiter(self, src, processed, 
						    dst, delimiter);
		if (ret != 0) {
			return ret;
		}
	}

	return read_code_byte(self, src, processed, delimiter);
}

/* Copy a single data byte from block */
static inline int copy_data_byte(struct cobs_decode_state *self,
				 const uint8_t *src, size_t *processed,
				 struct net_buf *dst, 
				 const uint8_t delimiter)
{
	uint8_t byte = src[(*processed)++];

	/* Unexpected delimiter in data */
	if (byte == delimiter) {
		return -EINVAL;
	}

	int ret = check_dst_space(dst);
	if (ret != 0) {
		return ret;
	}

	net_buf_add_u8(dst, byte);
	self->bytes_left--;
	
	return 0;
}

/* Copy data bytes from current block */
static inline int copy_block_bytes(struct cobs_decode_state *self,
				   const uint8_t *src, size_t *processed,
				   const size_t src_len, struct net_buf *dst,
				   const uint8_t delimiter)
{
	while (self->bytes_left > 0 && *processed < src_len) {
		int ret = copy_data_byte(self, src, processed, dst, delimiter);
		if (ret != 0) {
			return ret;
		}
	}
	
	return 0;
}

int cobs_decode_stream(struct cobs_decode_state *self, const uint8_t *src,
		       size_t src_len, struct net_buf *dst)
{
	if (!self || !src || !dst) {
		return -EINVAL;
	}

	const uint8_t delimiter = COBS_DEFAULT_DELIMITER;
	size_t processed = 0;

	/* Clear frame_complete flag at start of each call */
	self->frame_complete = false;

	while (processed < src_len) {
		/* Read new code byte if needed */
		if (self->bytes_left == 0) {
			int ret = process_code_byte(self, src, &processed, 
						    dst, delimiter);
			if (ret == 1) {
				/* Frame complete */
				return (ssize_t)processed;
			}
			if (ret < 0) {
				return ret;
			}
		}

		/* Copy data bytes from block */
		int ret = copy_block_bytes(self, src, &processed, src_len, 
					   dst, delimiter);
		if (ret != 0) {
			return ret;
		}
	}

	return (ssize_t)processed;
}
