/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The only translation unit that includes the library: its header and
 * implementation stay private to this file.
 */
#include <JPEGENC.h>
#include <jpegenc.inl>

#include <errno.h>
#include <limits.h>

#include <zephyr/toolchain.h>

#include "zephyr_jpegenc.h"

BUILD_ASSERT(sizeof(JPEGE_IMAGE) <= sizeof(((struct zephyr_jpegenc *)0)->state),
	     "ZEPHYR_JPEGENC_STATE_SIZE is too small for JPEGE_IMAGE");
BUILD_ASSERT(__alignof__(JPEGE_IMAGE) <= __alignof__(uint64_t),
	     "JPEGE_IMAGE needs more than 8-byte alignment");
BUILD_ASSERT(sizeof(JPEGENCODE) <= sizeof(((struct zephyr_jpegenc *)0)->encode),
	     "ZEPHYR_JPEGENC_ENCODE_SIZE is too small for JPEGENCODE");
BUILD_ASSERT(__alignof__(JPEGENCODE) <= __alignof__(uint32_t),
	     "JPEGENCODE needs more than 4-byte alignment");

/* Room the library keeps free at the end of the output buffer */
#define OUTPUT_HIGH_WATER 512U

static JPEGE_IMAGE *image(struct zephyr_jpegenc *enc)
{
	return (JPEGE_IMAGE *)enc->state;
}

static JPEGENCODE *encode(struct zephyr_jpegenc *enc)
{
	return (JPEGENCODE *)enc->encode;
}

static int error_to_errno(int error)
{
	switch (error) {
	case JPEGE_SUCCESS:
		return 0;
	case JPEGE_UNSUPPORTED_FEATURE:
		return -ENOTSUP;
	case JPEGE_NO_BUFFER:
		return -ENOSPC;
	case JPEGE_MEM_ERROR:
		return -ENOMEM;
	case JPEGE_INVALID_PARAMETER:
	case JPEGE_ENCODE_ERROR:
	case JPEGE_INVALID_FILE:
	default:
		return -EINVAL;
	}
}

static int pixel_type(enum zephyr_jpegenc_format format)
{
	switch (format) {
	case ZEPHYR_JPEGENC_GRAY8:
		return JPEGE_PIXEL_GRAYSCALE;
	case ZEPHYR_JPEGENC_RGB565_LE:
		return JPEGE_PIXEL_RGB565_LE;
	case ZEPHYR_JPEGENC_RGB565_BE:
		return JPEGE_PIXEL_RGB565_BE;
	case ZEPHYR_JPEGENC_BGR888:
		return JPEGE_PIXEL_RGB888;
	case ZEPHYR_JPEGENC_RGBX8888:
		return JPEGE_PIXEL_ARGB8888;
	case ZEPHYR_JPEGENC_YUYV:
		return JPEGE_PIXEL_YUV422;
	case ZEPHYR_JPEGENC_YUV420P:
		return JPEGE_PIXEL_YUV420P;
	case ZEPHYR_JPEGENC_NV12:
		return JPEGE_PIXEL_NV12;
	default:
		return -1;
	}
}

int zephyr_jpegenc_begin(struct zephyr_jpegenc *enc, uint8_t *out, size_t out_size, uint16_t width,
			 uint16_t height, enum zephyr_jpegenc_format format,
			 enum zephyr_jpegenc_subsampling subsampling,
			 enum zephyr_jpegenc_quality quality)
{
	JPEGE_IMAGE *jpeg;
	int type = pixel_type(format);

	if ((enc == NULL) || (out == NULL) || (out_size < ZEPHYR_JPEGENC_MIN_OUTPUT_SIZE) ||
	    (out_size > (size_t)INT_MAX) || (width == 0U) || (height == 0U) || (type < 0) ||
	    ((subsampling != ZEPHYR_JPEGENC_SUBSAMPLING_444) &&
	     (subsampling != ZEPHYR_JPEGENC_SUBSAMPLING_420)) ||
	    (quality < ZEPHYR_JPEGENC_QUALITY_BEST) || (quality > ZEPHYR_JPEGENC_QUALITY_LOW)) {
		return -EINVAL;
	}
	/* The YUV inputs are already 4:2:0, and the YUYV loader only produces 4:2:0 */
	if (((format == ZEPHYR_JPEGENC_YUYV) || (format == ZEPHYR_JPEGENC_YUV420P) ||
	     (format == ZEPHYR_JPEGENC_NV12)) &&
	    (subsampling != ZEPHYR_JPEGENC_SUBSAMPLING_420)) {
		return -ENOTSUP;
	}

	jpeg = image(enc);
	memset(jpeg, 0, sizeof(*jpeg));
	memset(encode(enc), 0, sizeof(JPEGENCODE));
	jpeg->pOutput = out;
	jpeg->iBufferSize = (int)out_size;
	jpeg->pHighWater = &out[out_size - OUTPUT_HIGH_WATER];

	return error_to_errno(JPEGEncodeBegin(jpeg, encode(enc), width, height, (uint8_t)type,
					      (subsampling == ZEPHYR_JPEGENC_SUBSAMPLING_420)
						      ? JPEGE_SUBSAMPLE_420
						      : JPEGE_SUBSAMPLE_444,
					      (uint8_t)quality));
}

int zephyr_jpegenc_add_frame(struct zephyr_jpegenc *enc, const uint8_t *pixels, size_t pitch)
{
	if ((pixels == NULL) || (pitch == 0U) || (pitch > (size_t)INT_MAX)) {
		return -EINVAL;
	}

	return error_to_errno(JPEGAddFrame(image(enc), encode(enc), (uint8_t *)pixels, (int)pitch));
}

int zephyr_jpegenc_add_frame_yuv420p(struct zephyr_jpegenc *enc, const uint8_t *y, size_t y_pitch,
				     const uint8_t *u, size_t u_pitch, const uint8_t *v,
				     size_t v_pitch)
{
	if ((y_pitch > (size_t)INT_MAX) || (u_pitch > (size_t)INT_MAX) ||
	    (v_pitch > (size_t)INT_MAX)) {
		return -EINVAL;
	}

	return error_to_errno(JPEGAddFrameYUV420P(image(enc), encode(enc), y, (int)y_pitch, u,
						  (int)u_pitch, v, (int)v_pitch));
}

int zephyr_jpegenc_add_frame_nv12(struct zephyr_jpegenc *enc, const uint8_t *y, size_t y_pitch,
				  const uint8_t *uv, size_t uv_pitch)
{
	if ((y_pitch > (size_t)INT_MAX) || (uv_pitch > (size_t)INT_MAX)) {
		return -EINVAL;
	}

	return error_to_errno(
		JPEGAddFrameNV12(image(enc), encode(enc), y, (int)y_pitch, uv, (int)uv_pitch));
}

int zephyr_jpegenc_end(struct zephyr_jpegenc *enc, size_t *size)
{
	int ret = JPEGEncodeEnd(image(enc));

	if (ret <= 0) {
		return -EINVAL;
	}
	*size = (size_t)ret;

	return 0;
}
