/*
 * Copyright (c) 2024 Kelly Helmut Lord
 * Copyright (c) 2026 Basalte bv
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


/**
 * @name COBS Encoder/Decoder Flags
 * @anchor COBS_FLAGS
 *
 * @{
 */

/**
 * @brief Default COBS delimiter value
 *
 * The standard COBS delimiter is zero (0x00). This is the delimiter value
 * used when COBS_FLAG_CUSTOM_DELIMITER is not specified.
 */
#define COBS_DEFAULT_DELIMITER 0x00

/**
 * @brief Flag indicating that encode or decode should include a trailing delimiter
 *
 * When set, the encoder will append a delimiter byte after the encoded data,
 * and the decoder will accept a delimiter byte at the end of the encoded data.
 */
#define COBS_FLAG_TRAILING_DELIMITER BIT(8)

/**
 * @brief Macro for setting a custom delimiter in flags
 *
 * The 8 LSB of "flags" is used for the delimiter value. When a custom delimiter is
 * configured, the implementation applies an XOR operation with the delimiter value
 * on the encoded data after encoding and before decoding. This allows COBS to work
 * with delimiters other than zero.
 *
 * @param x Custom delimiter value (0-255)
 *
 * @return Delimiter value masked to 8 bits
 *
 * Example usage:
 * @code{.c}
 * cobs_encode(src_buf, dst_buf, COBS_FLAG_TRAILING_DELIMITER | COBS_FLAG_CUSTOM_DELIMITER(0x7F));
 * @endcode
 */
#define COBS_FLAG_CUSTOM_DELIMITER(x) ((x) & 0xff)

/**
 * @}
 */

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
 * @param flags         Encoding flags @ref COBS_FLAGS
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
 * @brief COBS encoding
 *
 * Encodes data from source buffer to destination buffer using COBS encoding.
 *
 * @param src        Source buffer to encode
 * @param dst        Destination buffer for encoded data
 * @param flags      Encoding flags @ref COBS_FLAGS
 *
 * @retval 0        Success
 * @retval -ENOMEM  Insufficient destination space
 * @retval -EINVAL  Invalid COBS structure or parameters
 */

int cobs_encode(struct net_buf *src, struct net_buf *dst, uint32_t flags);

/**
 * @brief COBS decoding
 *
 * Decodes COBS-encoded data from source buffer to destination buffer.
 *
 * @param src        Source buffer to decode
 * @param dst        Destination buffer for decoded data
 * @param flags      Decoding flags @ref COBS_FLAGS
 *
 * @retval 0        Success
 * @retval -ENOMEM  Insufficient destination space
 * @retval -EINVAL  Invalid COBS structure or parameters
 */
int cobs_decode(struct net_buf *src, struct net_buf *dst, uint32_t flags);

/**
 * @brief Callback function type for streaming COBS encoder/decoder
 *
 * This callback is invoked by the streaming encoder/decoder to output
 * processed data chunks.
 * A decoder that allows trailing delimiters and encounters one will invoke
 * this callback with a NULL pointer and zero length indicating a completed frame.
 *
 * When this callback function returns a negative error value, the encoder or decoder
 * stream is aborted and the error will be propagated.
 *
 * @param buf        Buffer containing processed data
 * @param len        Length of data in buffer
 * @param user_data  User-provided context pointer
 *
 * @return 0 on success, negative errno code on failure
 */
typedef int (*cobs_stream_cb)(const uint8_t *buf, size_t len, void *user_data);

/**
 * @brief COBS streaming encoder state
 *
 * This structure maintains the state for incremental COBS encoding.
 * It should be initialized with cobs_encoder_init() before use.
 */
struct cobs_encoder {
	/** @cond INTERNAL_HIDDEN */
	/** Callback function to output encoded data */
	cobs_stream_cb cb;
	/** User data pointer passed to callback */
	void *cb_user_data;

	/** Internal buffer for partial encoding */
	uint8_t fragment[255];
	/** Encoding flags @ref COBS_FLAGS */
	uint32_t flags;
	/** @endcond */
};

/**
 * @brief COBS streaming decoder state
 *
 * This structure maintains the state for incremental COBS decoding.
 * It should be initialized with cobs_decoder_init() before use.
 */
struct cobs_decoder {
	/** @cond INTERNAL_HIDDEN */
	/** Callback function to output decoded data */
	cobs_stream_cb cb;
	/** User data pointer passed to callback */
	void *cb_user_data;

	/** Current COBS code byte being processed */
	uint8_t code;
	/** Position within current code block */
	uint8_t code_index;
	/** Decoding flags @ref COBS_FLAGS */
	uint32_t flags;
	/** @endcond */
};

/**
 * @brief Initialize COBS streaming encoder
 *
 * Initializes a COBS encoder for streaming operation. The encoder will call
 * the provided callback function to output encoded data chunks as they become
 * available.
 *
 * @param enc        Pointer to encoder structure to initialize
 * @param cb         Callback function for output data
 * @param user_data  User data pointer passed to callback
 * @param flags      Encoding flags @ref COBS_FLAGS
 *
 * @return 0 on success, negative errno code on failure
 */
int cobs_encoder_init(struct cobs_encoder *enc, cobs_stream_cb cb, void *user_data, uint32_t flags);

/**
 * @brief Finalize COBS streaming encoder
 *
 * Flushes any remaining data and optionally writes trailing delimiter if
 * COBS_FLAG_TRAILING_DELIMITER was set during initialization.
 *
 * The encoder state will be reset.
 *
 * @param enc Pointer to encoder structure
 *
 * @return 0 on success, negative errno code on failure
 */
int cobs_encoder_close(struct cobs_encoder *enc);

/**
 * @brief Write data to COBS streaming encoder
 *
 * Encodes the provided data and outputs encoded chunks via the registered
 * callback function. This function can be called multiple times to encode
 * data incrementally.
 *
 * In case an error is returned, the encoder state will be reset.
 *
 * @param enc Pointer to encoder structure
 * @param buf Buffer containing data to encode
 * @param len Length of data in buffer
 *
 * @return Number of bytes used from @p buf on success, negative errno code on failure
 */
int cobs_encoder_write(struct cobs_encoder *enc, const uint8_t *buf, size_t len);

/**
 * @brief Initialize COBS streaming decoder
 *
 * Initializes a COBS decoder for streaming operation. The decoder will call
 * the provided callback function to output decoded data chunks as they become
 * available.
 *
 * @param dec        Pointer to decoder structure to initialize
 * @param cb         Callback function for output data
 * @param user_data  User data pointer passed to callback
 * @param flags      Decoding flags @ref COBS_FLAGS
 *
 * @return 0 on success, negative errno code on failure
 */
int cobs_decoder_init(struct cobs_decoder *dec, cobs_stream_cb cb, void *user_data, uint32_t flags);

/**
 * @brief Finalize COBS streaming decoder
 *
 * Completes the decoding process and verifies that the stream ended properly.
 * Should be called after all data has been written to the decoder.
 *
 * The decoder state will be reset.
 *
 * @param dec Pointer to decoder structure
 *
 * @retval 0        Success
 * @retval -EINVAL  More data was expected before closing
 */
int cobs_decoder_close(struct cobs_decoder *dec);

/**
 * @brief Write data to COBS streaming decoder
 *
 * Decodes the provided encoded data and outputs decoded chunks via the
 * registered callback function. This function can be called multiple times
 * to decode data incrementally.
 *
 * In case an error is returned, the decoder state will be reset.
 *
 * @note If a delimiter is encountered, and the @ref COBS_FLAG_TRAILING_DELIMITER flag
 *       is set, the registered callback function will be called with a NULL pointer
 *       indicating a frame end.
 *
 * @param dec Pointer to decoder structure
 * @param buf Buffer containing encoded data
 * @param len Length of data in buffer
 *
 * @return Number of bytes used from @p buf on success, negative errno code on failure
 */
int cobs_decoder_write(struct cobs_decoder *dec, const uint8_t *buf, size_t len);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DATA_COBS_H_ */
