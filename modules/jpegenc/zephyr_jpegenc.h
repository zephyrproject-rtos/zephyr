/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Zephyr interface to the JPEGENC software JPEG encoder
 *
 * Applications and subsystems include this header instead of the library's
 * JPEGENC.h, so the encoder can be used next to other libraries, such as the
 * JPEGDEC decoder, whose headers define the same macro names.
 */

#ifndef ZEPHYR_MODULES_JPEGENC_ZEPHYR_JPEGENC_H_
#define ZEPHYR_MODULES_JPEGENC_ZEPHYR_JPEGENC_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup zephyr_jpegenc JPEGENC software JPEG encoder
 * @ingroup third_party
 * @{
 */

/** Bytes reserved for the library's encoder state */
#define ZEPHYR_JPEGENC_STATE_SIZE 3328U

/** Bytes reserved for the library's frame encoding state */
#define ZEPHYR_JPEGENC_ENCODE_SIZE 16U

/** Smallest output buffer zephyr_jpegenc_begin() accepts, in bytes */
#define ZEPHYR_JPEGENC_MIN_OUTPUT_SIZE 1024U

/** Input pixel format */
enum zephyr_jpegenc_format {
	/** 8-bit grayscale */
	ZEPHYR_JPEGENC_GRAY8,
	/** Little-endian RGB565 */
	ZEPHYR_JPEGENC_RGB565_LE,
	/** Big-endian RGB565 */
	ZEPHYR_JPEGENC_RGB565_BE,
	/** Packed 8-bit B, G, R */
	ZEPHYR_JPEGENC_BGR888,
	/** Packed 8-bit R, G, B and an ignored byte */
	ZEPHYR_JPEGENC_RGBX8888,
	/** Packed YCbCr 4:2:2 (Y, Cb, Y, Cr), encoded with 4:2:0 subsampling */
	ZEPHYR_JPEGENC_YUYV,
	/** Planar YCbCr 4:2:0, added with zephyr_jpegenc_add_frame_yuv420p() */
	ZEPHYR_JPEGENC_YUV420P,
	/** Semi-planar YCbCr 4:2:0, added with zephyr_jpegenc_add_frame_nv12() */
	ZEPHYR_JPEGENC_NV12,
};

/** Chroma subsampling of the encoded image */
enum zephyr_jpegenc_subsampling {
	/** No chroma subsampling */
	ZEPHYR_JPEGENC_SUBSAMPLING_444,
	/** Half chroma width and height */
	ZEPHYR_JPEGENC_SUBSAMPLING_420,
};

/** Encoding quality preset */
enum zephyr_jpegenc_quality {
	/** Best quality, largest output */
	ZEPHYR_JPEGENC_QUALITY_BEST,
	/** High quality */
	ZEPHYR_JPEGENC_QUALITY_HIGH,
	/** Medium quality */
	ZEPHYR_JPEGENC_QUALITY_MEDIUM,
	/** Low quality, smallest output */
	ZEPHYR_JPEGENC_QUALITY_LOW,
};

/** Encoder context, owned by the caller */
struct zephyr_jpegenc {
	/** @cond INTERNAL_HIDDEN */
	uint64_t state[ZEPHYR_JPEGENC_STATE_SIZE / sizeof(uint64_t)];
	uint32_t encode[ZEPHYR_JPEGENC_ENCODE_SIZE / sizeof(uint32_t)];
	/** @endcond */
};

/**
 * @brief Start encoding an image into a memory buffer
 *
 * @param enc Encoder context.
 * @param out Buffer that receives the JPEG data.
 * @param out_size Size of @p out, in bytes, at least ZEPHYR_JPEGENC_MIN_OUTPUT_SIZE.
 * @param width Image width, in pixels.
 * @param height Image height, in pixels.
 * @param format Input pixel format.
 * @param subsampling Chroma subsampling of the encoded image.
 * @param quality Encoding quality preset.
 *
 * @retval 0 Encoding started.
 * @retval -EINVAL Invalid argument.
 * @retval -ENOTSUP Unsupported combination of format and subsampling.
 */
int zephyr_jpegenc_begin(struct zephyr_jpegenc *enc, uint8_t *out, size_t out_size, uint16_t width,
			 uint16_t height, enum zephyr_jpegenc_format format,
			 enum zephyr_jpegenc_subsampling subsampling,
			 enum zephyr_jpegenc_quality quality);

/**
 * @brief Encode a whole frame of packed pixels
 *
 * @param enc Encoder context started with zephyr_jpegenc_begin().
 * @param pixels First row of the frame.
 * @param pitch Distance between rows, in bytes.
 *
 * @retval 0 Frame encoded.
 * @retval -EINVAL Invalid argument, or a planar format.
 * @retval -ENOSPC The output buffer is full.
 */
int zephyr_jpegenc_add_frame(struct zephyr_jpegenc *enc, const uint8_t *pixels, size_t pitch);

/**
 * @brief Encode a whole planar YUV 4:2:0 frame
 *
 * The chroma planes are half the image width and height, rounded up.
 *
 * @param enc Encoder context started with ZEPHYR_JPEGENC_YUV420P.
 * @param y Luma plane.
 * @param y_pitch Distance between luma rows, in bytes.
 * @param u Cb plane.
 * @param u_pitch Distance between Cb rows, in bytes.
 * @param v Cr plane.
 * @param v_pitch Distance between Cr rows, in bytes.
 *
 * @retval 0 Frame encoded.
 * @retval -EINVAL Invalid argument.
 * @retval -ENOSPC The output buffer is full.
 */
int zephyr_jpegenc_add_frame_yuv420p(struct zephyr_jpegenc *enc, const uint8_t *y, size_t y_pitch,
				     const uint8_t *u, size_t u_pitch, const uint8_t *v,
				     size_t v_pitch);

/**
 * @brief Encode a whole semi-planar NV12 frame
 *
 * The interleaved CbCr plane is half the image height, rounded up.
 *
 * @param enc Encoder context started with ZEPHYR_JPEGENC_NV12.
 * @param y Luma plane.
 * @param y_pitch Distance between luma rows, in bytes.
 * @param uv Interleaved CbCr plane.
 * @param uv_pitch Distance between CbCr rows, in bytes.
 *
 * @retval 0 Frame encoded.
 * @retval -EINVAL Invalid argument.
 * @retval -ENOSPC The output buffer is full.
 */
int zephyr_jpegenc_add_frame_nv12(struct zephyr_jpegenc *enc, const uint8_t *y, size_t y_pitch,
				  const uint8_t *uv, size_t uv_pitch);

/**
 * @brief Finish the image
 *
 * @param enc Encoder context.
 * @param size Receives the size of the JPEG data, in bytes.
 *
 * @retval 0 Image finished.
 * @retval -EINVAL Encoding did not start or failed.
 */
int zephyr_jpegenc_end(struct zephyr_jpegenc *enc, size_t *size);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_MODULES_JPEGENC_ZEPHYR_JPEGENC_H_ */
