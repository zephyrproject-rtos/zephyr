/*
 * Copyright (c) 2024 Kelly Helmut Lord
 * Copyright (c) 2025 Martin Schröder
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DATA_COBS_H_
#define ZEPHYR_INCLUDE_DATA_COBS_H_

#include <stddef.h>
#include <sys/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/net_buf.h>

#ifdef __cplusplus
extern "C" {
#endif

#define COBS_DEFAULT_DELIMITER 0x00

/**
 * @brief Flag indicating that encode and decode should include an explicit trailing delimiter
 */
#define COBS_FLAG_TRAILING_DELIMITER BIT(8)

/**
 * @brief Macro for embedding a custom delimiter into flags
 *
 * 8 LSB of "flags" is used for the delimiter
 * Example usage:
 * cobs_encode(src_buf, dst_buf, COBS_FLAG_TRAILING_DELIMITER | COBS_FLAG_CUSTOM_DELIMITER(0x7F));
 *
 * @param x Custom delimiter byte value
 */
#define COBS_FLAG_CUSTOM_DELIMITER(x) ((x) & 0xff)

/**
 * @defgroup cobs COBS (Consistent Overhead Byte Stuffing)
 * @ingroup utilities
 * @{
 *
 * @brief COBS encoding and decoding functions with custom delimiter support
 *
 * Provides functions for COBS encoding/decoding with configurable delimiters.
 * The implementation handles both standard zero-delimited COBS and custom
 * delimiter variants.
 */

/**
 * @brief Calculate maximum encoded buffer size
 *
 * @param decoded_size  Size of input data to be encoded
 * @param flags         COBS_FLAG_TRAILING_DELIMITER to include termination byte in calculation
 *
 * @return Required buffer size for worst-case encoding scenario
 */
static inline size_t cobs_max_encoded_len(size_t decoded_size, uint32_t flags)
{
	if (flags & COBS_FLAG_TRAILING_DELIMITER) {
		return decoded_size + decoded_size / 254 + 1 + 1;
	} else {
		return decoded_size + decoded_size / 254 + 1;
	}
}

/**
 * @brief Standard COBS encoding
 *
 * @param src        Source buffer to encode
 * @param dst        Destination buffer for encoded data
 * @param flags      Encoding flags (COBS_FLAG_TRAILING_DELIMITER, COBS_FLAG_CUSTOM_DELIMITER)
 *
 * @retval 0        Success
 * @retval -ENOMEM  Insufficient destination space
 * @retval -EINVAL  Invalid COBS structure or parameters
 */

int cobs_encode(struct net_buf *src, struct net_buf *dst, uint32_t flags);

/**
 * @brief Standard COBS decoding
 *
 * @param src        Source buffer to decode
 * @param dst        Destination buffer for decoded data
 * @param flags      Decoding flags (COBS_FLAG_TRAILING_DELIMITER, COBS_FLAG_CUSTOM_DELIMITER)
 *
 * @retval 0        Success
 * @retval -ENOMEM  Insufficient destination space
 * @retval -EINVAL  Invalid COBS structure or parameters
 */
int cobs_decode(struct net_buf *src, struct net_buf *dst, uint32_t flags);

/**
 * @brief COBS streaming encoder state machine context
 *
 * Encodes from a source buffer across multiple output buffer calls.
 * Non-destructive: tracks read position without modifying source.
 */
struct cobs_encode_state {
	/** Current source fragment being read */
	struct net_buf *src_frag;
	/** Offset within current fragment */
	size_t src_offset;
	/** Current block's code byte (0 = need to compute) */
	uint8_t block_code;
	/** Position within current block (0-254) */
	uint8_t block_pos;
};

/**
 * @brief COBS streaming decoder state machine context
 */
struct cobs_decode_state {
	/** Bytes remaining in current block (0-254) */
	uint8_t bytes_left;
	/** Whether to insert delimiter after block */
	bool need_delimiter;
	/** Set to true when frame delimiter found */
	bool frame_complete;
};

/**
 * @brief Initialize a COBS streaming encoder context
 *
 * @param self        Pointer to the COBS encoder state machine context (must not be NULL)
 */
void cobs_encode_init(struct cobs_encode_state *self);

/**
 * @brief Initialize a COBS streaming decoder context
 *
 * @param self        Pointer to the COBS decoder state machine context (must not be NULL)
 */
void cobs_decode_init(struct cobs_decode_state *self);

/**
 * @brief Encode data using a COBS streaming state machine
 *
 * Consumes as many bytes from src as possible, encoding them into dst.
 * When the function returns, src contains remaining unencoded data and
 * dst_len contains the number of bytes written to dst.
 *
 * @param self        Pointer to the COBS encoder state machine context
 * @param src         Source buffer to encode (data is pulled from it)
 * @param dst         Destination buffer for encoded data
 * @param dst_len     On input: capacity of dst; On output: bytes written
 * @param delimiter   Delimiter byte value (typically COBS_DEFAULT_DELIMITER)
 *
 * @retval 0        Success
 * @retval -EINVAL  Invalid parameters or delimiter conflicts with overhead byte
 */
int cobs_encode_stream(struct cobs_encode_state *self, struct net_buf *src,
			uint8_t *dst, size_t *dst_len, uint8_t delimiter);

/**
 * @brief Finalize COBS encoding and flush remaining data
 *
 * Writes any buffered data and final code byte to dst.
 *
 * @param self        Pointer to the COBS encoder state machine context
 * @param dst         Destination buffer for encoded data
 * @param dst_len     On input: capacity of dst; On output: bytes written
 * @param delimiter   Delimiter byte value (typically COBS_DEFAULT_DELIMITER)
 *
 * @retval 0        Success (encoder is reset)
 * @retval -ENOMEM  Insufficient destination space
 * @retval -EINVAL  Invalid parameters
 */
int cobs_encode_finalize(struct cobs_encode_state *self, uint8_t *dst, size_t *dst_len,
			 uint8_t delimiter);

/**
 * @brief Decode data using a COBS streaming state machine
 *
 * Decodes input fragment by treating it as continuation of stream.
 * Returns number of bytes processed from src on success, or negative error.
 * Stops processing when frame delimiter is found.
 *
 * @param self        Pointer to the COBS decoder state machine context
 * @param src         Source buffer fragment to decode
 * @param src_len     Length of source data
 * @param dst         Destination net_buf for decoded data
 * @param delimiter   Delimiter byte value (typically COBS_DEFAULT_DELIMITER)
 *
 * @retval >0       Number of bytes processed (frame may be complete)
 * @retval -ENOMEM  Insufficient destination space
 * @retval -EINVAL  Invalid COBS structure or unexpected delimiter
 */
int cobs_decode_stream(struct cobs_decode_state *self, const uint8_t *src,
			size_t src_len, struct net_buf *dst, uint8_t delimiter);

/** @} */
#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DATA_COBS_H_ */
