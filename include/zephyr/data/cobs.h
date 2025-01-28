/*
 * Copyright (c) 2024 Kelly Helmut Lord
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DATA_COBS_H_
#define ZEPHYR_INCLUDE_DATA_COBS_H_

#include <stddef.h>
#include <sys/types.h>
#include <zephyr/net_buf.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup cobs COBS (Consistent Overhead Byte Stuffing)
 * @ingroup utilities
 * @{
 *
 * @brief COBS encoding and decoding functions
 *
 * This module provides functions for encoding and decoding data using the
 * Consistent Overhead Byte Stuffing (COBS) algorithm. COBS is an encoding
 * that eliminates zeros in encoded data, making it useful for framing packets
 * in byte streams.
 */

/**
 * @brief Calculate the maximum encoded buffer size needed for COBS encoding
 *
 * The maximum size of COBS-encoded data will be:
 * - Original size + overhead byte per 254 bytes + final delimiter
 * - Worst case: input_size + input_size/254 + 1 + 1
 *
 * @param decoded_size Size of the data to be encoded
 *
 * @return Size of buffer required for encoding
 */
static inline size_t cobs_max_encoded_len(size_t decoded_size)
{
	return decoded_size + decoded_size / 254 + 1 + 1;
}

/**
 * @brief Encode data using Consistent Overhead Byte Stuffing (COBS)
 *
 * Encodes the data from the source buffer using COBS encoding and writes
 * the result to the destination buffer. The encoded data is guaranteed
 * to contain no zero bytes except for a final delimiter.
 *
 * @param src Source net_buf containing data to encode
 * @param dst Destination net_buf for encoded data
 *
 * @retval 0 Success
 * @retval -EINVAL Invalid parameters (NULL pointers)
 * @retval -ENOMEM Destination buffer too small
 */
int cobs_encode(struct net_buf *src, struct net_buf *dst);

/**
 * @brief Decode COBS-encoded data
 *
 * Decodes COBS-encoded data from the source buffer and writes the
 * decoded result to the destination buffer. The function validates
 * the COBS encoding during decoding.
 *
 * @param src Source net_buf containing COBS-encoded data
 * @param dst Destination net_buf for decoded data
 *
 * @retval 0 Success
 * @retval -EINVAL Invalid parameters (NULL pointers) or malformed COBS data
 * @retval -ENOMEM Destination buffer too small
 */
int cobs_decode(struct net_buf *src, struct net_buf *dst);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DATA_COBS_H_ */
