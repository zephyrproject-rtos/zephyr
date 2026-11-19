/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The only translation unit that includes the library: its header and
 * implementation stay private to this file.
 */
#include <JPEGDEC.h>
#include <jpeg.inl>

#include <errno.h>
#include <limits.h>

#include <zephyr/toolchain.h>

#include "zephyr_jpegdec.h"

BUILD_ASSERT(sizeof(JPEGIMAGE) <= sizeof(((struct zephyr_jpegdec *)0)->state),
	     "ZEPHYR_JPEGDEC_STATE_SIZE is too small for JPEGIMAGE");
BUILD_ASSERT(__alignof__(JPEGIMAGE) <= __alignof__(uint64_t),
	     "JPEGIMAGE needs more than 8-byte alignment");

static JPEGIMAGE *image(struct zephyr_jpegdec *dec)
{
	return (JPEGIMAGE *)dec->state;
}

static int error_to_errno(int error)
{
	switch (error) {
	case JPEG_SUCCESS:
		return 0;
	case JPEG_INVALID_PARAMETER:
		return -EINVAL;
	case JPEG_UNSUPPORTED_FEATURE:
		return -ENOTSUP;
	case JPEG_ERROR_MEMORY:
		return -ENOMEM;
	case JPEG_DECODE_ERROR:
	case JPEG_INVALID_FILE:
	default:
		return -EBADMSG;
	}
}

static int draw_block(JPEGDRAW *draw)
{
	struct zephyr_jpegdec *dec = draw->pUser;
	const struct zephyr_jpegdec_block block = {
		.x = draw->x,
		.y = draw->y,
		.width = draw->iWidthUsed,
		.height = draw->iHeight,
		.pitch = draw->iWidth,
		.pixels = draw->pPixels,
	};

	/* The library continues on a nonzero return */
	dec->draw_error = dec->draw(&block, dec->user_data);

	return (dec->draw_error == 0) ? 1 : 0;
}

int zephyr_jpegdec_open(struct zephyr_jpegdec *dec, const uint8_t *data, size_t size)
{
	if ((dec == NULL) || (data == NULL) || (size == 0U) || (size > (size_t)INT_MAX)) {
		return -EINVAL;
	}

	if (JPEG_openRAM(image(dec), (uint8_t *)data, (int)size, draw_block) == 0) {
		int ret = error_to_errno(JPEG_getLastError(image(dec)));

		return (ret != 0) ? ret : -EBADMSG;
	}

	return 0;
}

void zephyr_jpegdec_get_size(struct zephyr_jpegdec *dec, uint16_t *width, uint16_t *height)
{
	*width = (uint16_t)JPEG_getWidth(image(dec));
	*height = (uint16_t)JPEG_getHeight(image(dec));
}

int zephyr_jpegdec_decode_rgb565(struct zephyr_jpegdec *dec, bool big_endian,
				 enum zephyr_jpegdec_scale scale, zephyr_jpegdec_draw_t draw,
				 void *user_data)
{
	JPEGIMAGE *jpeg = image(dec);
	int options;
	int ret;

	switch (scale) {
	case ZEPHYR_JPEGDEC_SCALE_1:
		options = 0;
		break;
	case ZEPHYR_JPEGDEC_SCALE_1_2:
		options = JPEG_SCALE_HALF;
		break;
	case ZEPHYR_JPEGDEC_SCALE_1_4:
		options = JPEG_SCALE_QUARTER;
		break;
	case ZEPHYR_JPEGDEC_SCALE_1_8:
		options = JPEG_SCALE_EIGHTH;
		break;
	default:
		return -EINVAL;
	}
	if (draw == NULL) {
		return -EINVAL;
	}

	dec->draw = draw;
	dec->user_data = user_data;
	dec->draw_error = 0;
	jpeg->pUser = dec;
	JPEG_setPixelType(jpeg, big_endian ? RGB565_BIG_ENDIAN : RGB565_LITTLE_ENDIAN);

	ret = (JPEG_decode(jpeg, 0, 0, options) != 0) ? 0 : error_to_errno(JPEG_getLastError(jpeg));

	/* The library reports success when the callback stops it */
	if (dec->draw_error != 0) {
		return (dec->draw_error < 0) ? dec->draw_error : -ECANCELED;
	}

	return ret;
}

int zephyr_jpegdec_decode_yuv420p(struct zephyr_jpegdec *dec, uint8_t *y, size_t y_pitch,
				  uint8_t *u, size_t u_pitch, uint8_t *v, size_t v_pitch)
{
	JPEGIMAGE *jpeg = image(dec);

	if ((u == NULL) || (v == NULL) || (y_pitch > (size_t)INT_MAX) ||
	    (u_pitch > (size_t)INT_MAX) || (v_pitch > (size_t)INT_MAX)) {
		return -EINVAL;
	}

	if (JPEG_decodeYUV420P(jpeg, y, (int)y_pitch, u, (int)u_pitch, v, (int)v_pitch) != 0) {
		return 0;
	}

	return error_to_errno(JPEG_getLastError(jpeg));
}

int zephyr_jpegdec_decode_nv12(struct zephyr_jpegdec *dec, uint8_t *y, size_t y_pitch, uint8_t *uv,
			       size_t uv_pitch)
{
	JPEGIMAGE *jpeg = image(dec);

	if ((uv == NULL) || (y_pitch > (size_t)INT_MAX) || (uv_pitch > (size_t)INT_MAX)) {
		return -EINVAL;
	}

	if (JPEG_decodeNV12(jpeg, y, (int)y_pitch, uv, (int)uv_pitch) != 0) {
		return 0;
	}

	return error_to_errno(JPEG_getLastError(jpeg));
}

void zephyr_jpegdec_close(struct zephyr_jpegdec *dec)
{
	JPEG_close(image(dec));
}
