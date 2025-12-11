/*
 * Copyright (c) 2024 Kelly Helmut Lord
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
 * Flag indicating that encode and decode should include an implicit end delimiter
 */
#define COBS_FLAG_TRAILING_DELIMITER BIT(8)

/**
 * Macro for extracting delimiter from flags. 8 LSB of "flags" is used for the delimiter
 * Example usage:
 * cobs_encode(src_buf, dst_buf, COBS_FLAG_TRAILING_DELIMITER | COBS_FLAG_CUSTOM_DELIMITER(0x7F));
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
 * @param src        Source buffer to decode
 * @param dst        Destination buffer for decoded data
 * @param flags      Decoding flags (reserved)
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
 * @param flags      Decoding flags (reserved)
 *
 * @retval 0        Success
 * @retval -ENOMEM  Insufficient destination space
 * @retval -EINVAL  Invalid COBS structure or parameters
 */
int cobs_decode(struct net_buf *src, struct net_buf *dst, uint32_t flags);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DATA_COBS_H_ */
