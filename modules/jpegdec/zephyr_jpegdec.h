/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Zephyr interface to the JPEGDEC software JPEG decoder
 *
 * Applications and subsystems include this header instead of the library's
 * JPEGDEC.h, so the decoder can be used next to other libraries, such as the
 * JPEGENC encoder, whose headers define the same macro names.
 */

#ifndef ZEPHYR_MODULES_JPEGDEC_ZEPHYR_JPEGDEC_H_
#define ZEPHYR_MODULES_JPEGDEC_ZEPHYR_JPEGDEC_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup zephyr_jpegdec JPEGDEC software JPEG decoder
 * @ingroup third_party
 * @{
 */

/** Bytes reserved for the library's decoder state */
#define ZEPHYR_JPEGDEC_STATE_SIZE 18432U

/** Output scale for RGB565 decoding */
enum zephyr_jpegdec_scale {
	/** Full size */
	ZEPHYR_JPEGDEC_SCALE_1 = 1,
	/** Half width and height */
	ZEPHYR_JPEGDEC_SCALE_1_2 = 2,
	/** Quarter width and height */
	ZEPHYR_JPEGDEC_SCALE_1_4 = 4,
	/** Eighth width and height */
	ZEPHYR_JPEGDEC_SCALE_1_8 = 8,
};

/** A block of decoded RGB565 pixels */
struct zephyr_jpegdec_block {
	/** Left edge of the block in the output, in pixels */
	int32_t x;
	/** Top edge of the block in the output, in pixels */
	int32_t y;
	/** Number of valid pixels in each row */
	int32_t width;
	/** Number of rows */
	int32_t height;
	/** Distance between the starts of two rows, in pixels */
	int32_t pitch;
	/** Pixels, in the byte order requested from zephyr_jpegdec_decode_rgb565() */
	const uint16_t *pixels;
};

/**
 * @brief Receive a block of decoded pixels
 *
 * @param block Decoded block, valid only during the call.
 * @param user_data Pointer passed to zephyr_jpegdec_decode_rgb565().
 *
 * @retval 0 Continue decoding.
 * @retval <0 Stop decoding.
 */
typedef int (*zephyr_jpegdec_draw_t)(const struct zephyr_jpegdec_block *block, void *user_data);

/** Decoder context, owned by the caller */
struct zephyr_jpegdec {
	/** @cond INTERNAL_HIDDEN */
	uint64_t state[ZEPHYR_JPEGDEC_STATE_SIZE / sizeof(uint64_t)];
	zephyr_jpegdec_draw_t draw;
	void *user_data;
	int draw_error;
	/** @endcond */
};

/**
 * @brief Parse the headers of a JPEG image held in memory
 *
 * @param dec Decoder context.
 * @param data JPEG data. It must stay valid until zephyr_jpegdec_close().
 * @param size Size of @p data, in bytes.
 *
 * @retval 0 Headers parsed.
 * @retval -EINVAL Invalid argument.
 * @retval -ENOTSUP The image uses a feature the decoder does not support.
 * @retval -EBADMSG The data is not a valid JPEG image.
 */
int zephyr_jpegdec_open(struct zephyr_jpegdec *dec, const uint8_t *data, size_t size);

/**
 * @brief Get the dimensions of the opened image
 *
 * @param dec Decoder context opened with zephyr_jpegdec_open().
 * @param width Receives the image width, in pixels.
 * @param height Receives the image height, in pixels.
 */
void zephyr_jpegdec_get_size(struct zephyr_jpegdec *dec, uint16_t *width, uint16_t *height);

/**
 * @brief Decode the opened image to RGB565, one block at a time
 *
 * @param dec Decoder context opened with zephyr_jpegdec_open().
 * @param big_endian Deliver pixels in big-endian instead of little-endian byte order.
 * @param scale Output scale.
 * @param draw Callback that receives each decoded block.
 * @param user_data Pointer passed to @p draw.
 *
 * @retval 0 Image decoded.
 * @retval -EINVAL Invalid argument.
 * @retval -ENOTSUP The image uses a feature the decoder does not support.
 * @retval -EBADMSG The image data is corrupt.
 * @retval <0 Other negative values: the error @p draw returned to stop decoding.
 */
int zephyr_jpegdec_decode_rgb565(struct zephyr_jpegdec *dec, bool big_endian,
				 enum zephyr_jpegdec_scale scale, zephyr_jpegdec_draw_t draw,
				 void *user_data);

/**
 * @brief Decode the opened image to planar YUV 4:2:0
 *
 * The chroma planes are half the image width and height, rounded up.
 *
 * @param dec Decoder context opened with zephyr_jpegdec_open().
 * @param y Luma plane.
 * @param y_pitch Distance between luma rows, in bytes.
 * @param u Cb plane.
 * @param u_pitch Distance between Cb rows, in bytes.
 * @param v Cr plane.
 * @param v_pitch Distance between Cr rows, in bytes.
 *
 * @retval 0 Image decoded.
 * @retval -EINVAL Invalid argument.
 * @retval -ENOTSUP The image uses a feature the decoder does not support.
 * @retval -EBADMSG The image data is corrupt.
 */
int zephyr_jpegdec_decode_yuv420p(struct zephyr_jpegdec *dec, uint8_t *y, size_t y_pitch,
				  uint8_t *u, size_t u_pitch, uint8_t *v, size_t v_pitch);

/**
 * @brief Decode the opened image to semi-planar NV12
 *
 * The interleaved CbCr plane is half the image height, rounded up.
 *
 * @param dec Decoder context opened with zephyr_jpegdec_open().
 * @param y Luma plane.
 * @param y_pitch Distance between luma rows, in bytes.
 * @param uv Interleaved CbCr plane.
 * @param uv_pitch Distance between CbCr rows, in bytes.
 *
 * @retval 0 Image decoded.
 * @retval -EINVAL Invalid argument.
 * @retval -ENOTSUP The image uses a feature the decoder does not support.
 * @retval -EBADMSG The image data is corrupt.
 */
int zephyr_jpegdec_decode_nv12(struct zephyr_jpegdec *dec, uint8_t *y, size_t y_pitch, uint8_t *uv,
			       size_t uv_pitch);

/**
 * @brief Release the opened image
 *
 * @param dec Decoder context opened with zephyr_jpegdec_open().
 */
void zephyr_jpegdec_close(struct zephyr_jpegdec *dec);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_MODULES_JPEGDEC_ZEPHYR_JPEGDEC_H_ */
