/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/pixel/convert.h>

LOG_MODULE_REGISTER(pixel_convert, CONFIG_PIXEL_LOG_LEVEL);

int pixel_image_convert(struct pixel_image *img, uint32_t new_format)
{
	const struct pixel_operation *op = NULL;

	STRUCT_SECTION_FOREACH_ALTERNATE(pixel_convert, pixel_operation, tmp) {
		if (tmp->fourcc_in == img->fourcc && tmp->fourcc_out == new_format) {
			op = tmp;
			break;
		}
	}

	if (op == NULL) {
		LOG_ERR("Conversion operation from %s to %s not found",
			VIDEO_FOURCC_TO_STR(img->fourcc), VIDEO_FOURCC_TO_STR(new_format));
		return pixel_image_error(img, -ENOSYS);
	}

	return pixel_image_add_uncompressed(img, op);
}

void pixel_convert_op(struct pixel_operation *op)
{
	const uint8_t *line_in = pixel_operation_get_input_line(op);
	uint8_t *line_out = pixel_operation_get_output_line(op);
	void (*convert)(const uint8_t *rgb24i, uint8_t *rgb24o, uint16_t width) = op->arg;

	__ASSERT_NO_MSG(convert != NULL);

	convert(line_in, line_out, op->width);
	pixel_operation_done(op);
}

__weak void pixel_line_rgb24_to_rgb24(const uint8_t *rgb24i, uint8_t *rgb24o, uint16_t width)
{
	memcpy(rgb24o, rgb24i, width * 3);
}
PIXEL_DEFINE_CONVERT_OPERATION(pixel_line_rgb24_to_rgb24,
			       VIDEO_PIX_FMT_RGB24, VIDEO_PIX_FMT_RGB24);

__weak void pixel_line_rgb24_to_rgb332(const uint8_t *rgb24, uint8_t *rgb332, uint16_t width)
{
	for (size_t i = 0, o = 0, w = 0; w < width; w++, i += 3, o += 1) {
		rgb332[o] = 0;
		rgb332[o] |= (uint16_t)rgb24[i + 0] >> 5 << (0 + 3 + 2);
		rgb332[o] |= (uint16_t)rgb24[i + 1] >> 5 << (0 + 0 + 2);
		rgb332[o] |= (uint16_t)rgb24[i + 2] >> 6 << (0 + 0 + 0);
	}
}
PIXEL_DEFINE_CONVERT_OPERATION(pixel_line_rgb24_to_rgb332,
			       VIDEO_PIX_FMT_RGB24, VIDEO_PIX_FMT_RGB332);

__weak void pixel_line_rgb332_to_rgb24(const uint8_t *rgb332, uint8_t *rgb24, uint16_t width)
{
	for (size_t i = 0, o = 0, w = 0; w < width; w++, i += 1, o += 3) {
		rgb24[o + 0] = rgb332[i] >> (0 + 3 + 2) << 5;
		rgb24[o + 1] = rgb332[i] >> (0 + 0 + 2) << 5;
		rgb24[o + 2] = rgb332[i] >> (0 + 0 + 0) << 6;
	}
}
PIXEL_DEFINE_CONVERT_OPERATION(pixel_line_rgb332_to_rgb24,
			       VIDEO_PIX_FMT_RGB332, VIDEO_PIX_FMT_RGB24);

static inline uint16_t pixel_rgb24_to_rgb565(const uint8_t rgb24[3])
{
	uint16_t rgb565 = 0;

	rgb565 |= ((uint16_t)rgb24[0] >> 3 << (0 + 6 + 5));
	rgb565 |= ((uint16_t)rgb24[1] >> 2 << (0 + 0 + 5));
	rgb565 |= ((uint16_t)rgb24[2] >> 3 << (0 + 0 + 0));
	return rgb565;
}

static inline void pixel_rgb565_to_rgb24(uint16_t rgb565, uint8_t rgb24[3])
{
	rgb24[0] = rgb565 >> (0 + 6 + 5) << 3;
	rgb24[1] = rgb565 >> (0 + 0 + 5) << 2;
	rgb24[2] = rgb565 >> (0 + 0 + 0) << 3;
}

__weak void pixel_line_rgb24_to_rgb565be(const uint8_t *rgb24, uint8_t *rgb565be, uint16_t width)
{
	for (size_t i = 0, o = 0, w = 0; w < width; w++, i += 3, o += 2) {
		*(uint16_t *)&rgb565be[o] = sys_cpu_to_be16(pixel_rgb24_to_rgb565(&rgb24[i]));
	}
}
PIXEL_DEFINE_CONVERT_OPERATION(pixel_line_rgb24_to_rgb565be,
			       VIDEO_PIX_FMT_RGB24, VIDEO_PIX_FMT_RGB565X);

__weak void pixel_line_rgb24_to_rgb565le(const uint8_t *rgb24, uint8_t *rgb565le, uint16_t width)
{
	for (size_t i = 0, o = 0, w = 0; w < width; w++, i += 3, o += 2) {
		*(uint16_t *)&rgb565le[o] = sys_cpu_to_le16(pixel_rgb24_to_rgb565(&rgb24[i]));
	}
}
PIXEL_DEFINE_CONVERT_OPERATION(pixel_line_rgb24_to_rgb565le,
			       VIDEO_PIX_FMT_RGB24, VIDEO_PIX_FMT_RGB565);

__weak void pixel_line_rgb565be_to_rgb24(const uint8_t *rgb565be, uint8_t *rgb24, uint16_t width)
{
	for (size_t i = 0, o = 0, w = 0; w < width; w++, i += 2, o += 3) {
		pixel_rgb565_to_rgb24(sys_be16_to_cpu(*(uint16_t *)&rgb565be[i]), &rgb24[o]);
	}
}
PIXEL_DEFINE_CONVERT_OPERATION(pixel_line_rgb565be_to_rgb24,
			       VIDEO_PIX_FMT_RGB565X, VIDEO_PIX_FMT_RGB24);

__weak void pixel_line_rgb565le_to_rgb24(const uint8_t *rgb565le, uint8_t *rgb24, uint16_t width)
{
	for (size_t i = 0, o = 0, w = 0; w < width; w++, i += 2, o += 3) {
		pixel_rgb565_to_rgb24(sys_le16_to_cpu(*(uint16_t *)&rgb565le[i]), &rgb24[o]);
	}
}
PIXEL_DEFINE_CONVERT_OPERATION(pixel_line_rgb565le_to_rgb24,
			       VIDEO_PIX_FMT_RGB565, VIDEO_PIX_FMT_RGB24);

#define Q21(val) ((int32_t)((val) * (1 << 21)))

static inline uint8_t pixel_rgb24_to_y8_bt709(const uint8_t rgb24[3])
{
	int16_t r = rgb24[0], g = rgb24[1], b = rgb24[2];

	return CLAMP(((Q21(+0.1826) * r + Q21(+0.6142) * g + Q21(+0.0620) * b) >> 21) + 16,
		     0x00, 0xff);
}

uint8_t pixel_rgb24_get_luma_bt709(const uint8_t rgb24[3])
{
	return pixel_rgb24_to_y8_bt709(rgb24);
}

static inline uint8_t pixel_rgb24_to_u8_bt709(const uint8_t rgb24[3])
{
	int16_t r = rgb24[0], g = rgb24[1], b = rgb24[2];

	return CLAMP(((Q21(-0.1006) * r + Q21(-0.3386) * g + Q21(+0.4392) * b) >> 21) + 128,
		     0x00, 0xff);
}

static inline uint8_t pixel_rgb24_to_v8_bt709(const uint8_t rgb24[3])
{
	int16_t r = rgb24[0], g = rgb24[1], b = rgb24[2];

	return CLAMP(((Q21(+0.4392) * r + Q21(-0.3989) * g + Q21(-0.0403) * b) >> 21) + 128,
		     0x00, 0xff);
}

static inline void pixel_yuv24_to_rgb24_bt709(const uint8_t y, uint8_t u, uint8_t v,
					      uint8_t rgb24[3])
{
	int32_t yy = (int32_t)y - 16, uu = (int32_t)u - 128, vv = (int32_t)v - 128;

	/* Y range [16:235], U/V range [16:240], RGB range[0:255] (full range) */
	rgb24[0] = CLAMP((Q21(+1.1644) * yy + Q21(+0.0000) * uu + Q21(+1.7928) * vv) >> 21,
			 0x00, 0xff);
	rgb24[1] = CLAMP((Q21(+1.1644) * yy + Q21(-0.2133) * uu + Q21(-0.5330) * vv) >> 21,
			 0x00, 0xff);
	rgb24[2] = CLAMP((Q21(+1.1644) * yy + Q21(+2.1124) * uu + Q21(+0.0000) * vv) >> 21,
			 0x00, 0xff);
}

#undef Q21

__weak void pixel_line_yuv24_to_rgb24_bt709(const uint8_t *yuv24, uint8_t *rgb24, uint16_t width)
{
	for (size_t i = 0, o = 0, w = 0; w < width; w++, i += 3, o += 3) {
		pixel_yuv24_to_rgb24_bt709(yuv24[i + 0], yuv24[i + 1], yuv24[i + 2], &rgb24[o]);
	}
}
PIXEL_DEFINE_CONVERT_OPERATION(pixel_line_yuv24_to_rgb24_bt709,
			       VIDEO_PIX_FMT_YUV24, VIDEO_PIX_FMT_RGB24);

void pixel_line_rgb24_to_yuv24_bt709(const uint8_t *rgb24, uint8_t *yuv24, uint16_t width)
{
	for (size_t i = 0, o = 0, w = 0; w < width; w++, i += 3, o += 3) {
		yuv24[o + 0] = pixel_rgb24_to_y8_bt709(&rgb24[i]);
		yuv24[o + 1] = pixel_rgb24_to_u8_bt709(&rgb24[i]);
		yuv24[o + 2] = pixel_rgb24_to_v8_bt709(&rgb24[i]);
	}
}
PIXEL_DEFINE_CONVERT_OPERATION(pixel_line_rgb24_to_yuv24_bt709,
			       VIDEO_PIX_FMT_RGB24, VIDEO_PIX_FMT_YUV24);

__weak void pixel_line_yuv24_to_yuyv(const uint8_t *yuv24, uint8_t *yuyv, uint16_t width)
{
	for (size_t i = 0, o = 0, w = 0; w + 2 <= width; w += 2, i += 6, o += 4) {
		/* Pixel 0 */
		yuyv[o + 0] = yuv24[i + 0];
		yuyv[o + 1] = yuv24[i + 1];
		/* Pixel 1 */
		yuyv[o + 2] = yuv24[i + 3];
		yuyv[o + 3] = yuv24[i + 5];
	}
}
PIXEL_DEFINE_CONVERT_OPERATION(pixel_line_yuv24_to_yuyv,
			       VIDEO_PIX_FMT_YUV24, VIDEO_PIX_FMT_YUYV);

__weak void pixel_line_yuyv_to_yuv24(const uint8_t *yuyv, uint8_t *yuv24, uint16_t width)
{
	for (size_t i = 0, o = 0, w = 0; w + 2 <= width; w += 2, i += 4, o += 6) {
		/* Pixel 0 */
		yuv24[o + 0] = yuyv[i + 0];
		yuv24[o + 1] = yuyv[i + 1];
		yuv24[o + 2] = yuyv[i + 3];
		/* Pixel 1 */
		yuv24[o + 3] = yuyv[i + 2];
		yuv24[o + 4] = yuyv[i + 1];
		yuv24[o + 5] = yuyv[i + 3];
	}
}
PIXEL_DEFINE_CONVERT_OPERATION(pixel_line_yuyv_to_yuv24,
			       VIDEO_PIX_FMT_YUYV, VIDEO_PIX_FMT_YUV24);

__weak void pixel_line_rgb24_to_yuyv_bt709(const uint8_t *rgb24, uint8_t *yuyv, uint16_t width)
{
	for (size_t i = 0, o = 0, w = 0; w + 2 <= width; w += 2, i += 6, o += 4) {
		/* Pixel 0 */
		yuyv[o + 0] = pixel_rgb24_to_y8_bt709(&rgb24[i + 0]);
		yuyv[o + 1] = pixel_rgb24_to_u8_bt709(&rgb24[i + 0]);
		/* Pixel 1 */
		yuyv[o + 2] = pixel_rgb24_to_y8_bt709(&rgb24[i + 3]);
		yuyv[o + 3] = pixel_rgb24_to_v8_bt709(&rgb24[i + 3]);
	}
}
PIXEL_DEFINE_CONVERT_OPERATION(pixel_line_rgb24_to_yuyv_bt709,
			       VIDEO_PIX_FMT_RGB24, VIDEO_PIX_FMT_YUYV);

__weak void pixel_line_yuyv_to_rgb24_bt709(const uint8_t *yuyv, uint8_t *rgb24, uint16_t width)
{
	for (size_t i = 0, o = 0, w = 0; w + 2 <= width; w += 2, i += 4, o += 6) {
		/* Pixel 0 */
		pixel_yuv24_to_rgb24_bt709(yuyv[i + 0], yuyv[i + 1], yuyv[i + 3], &rgb24[o + 0]);
		/* Pixel 1 */
		pixel_yuv24_to_rgb24_bt709(yuyv[i + 2], yuyv[i + 1], yuyv[i + 3], &rgb24[o + 3]);
	}
}
PIXEL_DEFINE_CONVERT_OPERATION(pixel_line_yuyv_to_rgb24_bt709,
			       VIDEO_PIX_FMT_YUYV, VIDEO_PIX_FMT_RGB24);

__weak void pixel_line_y8_to_rgb24_bt709(const uint8_t *y8, uint8_t *rgb24, uint16_t width)
{
	for (size_t i = 0, o = 0, w = 0; w < width; w++, i += 1, o += 3) {
		pixel_yuv24_to_rgb24_bt709(y8[i], UINT8_MAX / 2, UINT8_MAX / 2, &rgb24[o]);
	}
}
PIXEL_DEFINE_CONVERT_OPERATION(pixel_line_y8_to_rgb24_bt709,
			       VIDEO_PIX_FMT_GREY, VIDEO_PIX_FMT_RGB24);

__weak void pixel_line_rgb24_to_y8_bt709(const uint8_t *rgb24, uint8_t *y8, uint16_t width)
{
	for (size_t i = 0, o = 0, w = 0; w < width; w++, i += 3, o += 1) {
		y8[o] = pixel_rgb24_to_y8_bt709(&rgb24[i]);
	}
}
PIXEL_DEFINE_CONVERT_OPERATION(pixel_line_rgb24_to_y8_bt709,
			       VIDEO_PIX_FMT_RGB24, VIDEO_PIX_FMT_GREY);
