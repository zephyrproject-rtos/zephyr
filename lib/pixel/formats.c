/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/pixel/formats.h>

LOG_MODULE_REGISTER(pixel_formats, CONFIG_PIXEL_LOG_LEVEL);

/* RGB332 <-> RGB24 */

__weak void pixel_rgb24line_to_rgb332line(const uint8_t *rgb24, uint8_t *rgb332, uint16_t width)
{
	for (size_t i = 0, o = 0, w = 0; w < width; w++, i += 3, o += 1) {
		rgb332[o] = 0;
		rgb332[o] |= (uint16_t)rgb24[i + 0] >> 5 << (0 + 3 + 2);
		rgb332[o] |= (uint16_t)rgb24[i + 1] >> 5 << (0 + 0 + 2);
		rgb332[o] |= (uint16_t)rgb24[i + 2] >> 6 << (0 + 0 + 0);
	}
}

void pixel_rgb24stream_to_rgb332stream(struct pixel_stream *strm)
{
	pixel_line_to_stream(strm, pixel_rgb24line_to_rgb332line);
}

__weak void pixel_rgb332line_to_rgb24line(const uint8_t *rgb332, uint8_t *rgb24, uint16_t width)
{
	for (size_t i = 0, o = 0, w = 0; w < width; w++, i += 1, o += 3) {
		rgb24[o + 0] = rgb332[i] >> (0 + 3 + 2) << 5;
		rgb24[o + 1] = rgb332[i] >> (0 + 0 + 2) << 5;
		rgb24[o + 2] = rgb332[i] >> (0 + 0 + 0) << 6;
	}
}

void pixel_rgb332stream_to_rgb24stream(struct pixel_stream *strm)
{
	pixel_line_to_stream(strm, pixel_rgb332line_to_rgb24line);
}

/* RGB565 <-> RGB24 */

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

__weak void pixel_rgb24line_to_rgb565beline(const uint8_t *rgb24, uint8_t *rgb565be, uint16_t width)
{
	for (size_t i = 0, o = 0, w = 0; w < width; w++, i += 3, o += 2) {
		*(uint16_t *)&rgb565be[o] = sys_cpu_to_be16(pixel_rgb24_to_rgb565(&rgb24[i]));
	}
}

void pixel_rgb24stream_to_rgb565bestream(struct pixel_stream *strm)
{
	pixel_line_to_stream(strm, pixel_rgb24line_to_rgb565beline);
}

__weak void pixel_rgb24line_to_rgb565leline(const uint8_t *rgb24, uint8_t *rgb565le, uint16_t width)
{
	for (size_t i = 0, o = 0, w = 0; w < width; w++, i += 3, o += 2) {
		*(uint16_t *)&rgb565le[o] = sys_cpu_to_le16(pixel_rgb24_to_rgb565(&rgb24[i]));
	}
}

void pixel_rgb24stream_to_rgb565lestream(struct pixel_stream *strm)
{
	pixel_line_to_stream(strm, pixel_rgb24line_to_rgb565leline);
}

__weak void pixel_rgb565beline_to_rgb24line(const uint8_t *rgb565be, uint8_t *rgb24, uint16_t width)
{
	for (size_t i = 0, o = 0, w = 0; w < width; w++, i += 2, o += 3) {
		pixel_rgb565_to_rgb24(sys_be16_to_cpu(*(uint16_t *)&rgb565be[i]), &rgb24[o]);
	}
}

void pixel_rgb565bestream_to_rgb24stream(struct pixel_stream *strm)
{
	pixel_line_to_stream(strm, pixel_rgb565beline_to_rgb24line);
}

__weak void pixel_rgb565leline_to_rgb24line(const uint8_t *rgb565le, uint8_t *rgb24, uint16_t width)
{
	for (size_t i = 0, o = 0, w = 0; w < width; w++, i += 2, o += 3) {
		pixel_rgb565_to_rgb24(sys_le16_to_cpu(*(uint16_t *)&rgb565le[i]), &rgb24[o]);
	}
}

void pixel_rgb565lestream_to_rgb24stream(struct pixel_stream *strm)
{
	pixel_line_to_stream(strm, pixel_rgb565leline_to_rgb24line);
}

/* YUV support */

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

/* YUV24 <-> RGB24 */

__weak void pixel_yuv24line_to_rgb24line_bt709(const uint8_t *yuv24, uint8_t *rgb24, uint16_t width)
{
	for (size_t i = 0, o = 0, w = 0; w < width; w++, i += 3, o += 3) {
		pixel_yuv24_to_rgb24_bt709(yuv24[i + 0], yuv24[i + 1], yuv24[i + 2], &rgb24[o]);
	}
}

void pixel_yuv24stream_to_rgb24stream_bt709(struct pixel_stream *strm)
{
	pixel_line_to_stream(strm, pixel_yuv24line_to_rgb24line_bt709);
}

void pixel_rgb24line_to_yuv24line_bt709(const uint8_t *rgb24, uint8_t *yuv24, uint16_t width)
{
	for (size_t i = 0, o = 0, w = 0; w < width; w++, i += 3, o += 3) {
		yuv24[o + 0] = pixel_rgb24_to_y8_bt709(&rgb24[i]);
		yuv24[o + 1] = pixel_rgb24_to_u8_bt709(&rgb24[i]);
		yuv24[o + 2] = pixel_rgb24_to_v8_bt709(&rgb24[i]);
	}
}

void pixel_rgb24stream_to_yuv24stream_bt709(struct pixel_stream *strm)
{
	pixel_line_to_stream(strm, pixel_rgb24line_to_yuv24line_bt709);
}

/* YUYV <-> YUV24 */

__weak void pixel_yuv24line_to_yuyvline(const uint8_t *yuv24, uint8_t *yuyv, uint16_t width)
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

void pixel_yuv24stream_to_yuyvstream(struct pixel_stream *strm)
{
	pixel_line_to_stream(strm, pixel_yuv24line_to_yuyvline);
}

__weak void pixel_yuyvline_to_yuv24line(const uint8_t *yuyv, uint8_t *yuv24, uint16_t width)
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

void pixel_yuyvstream_to_yuv24stream(struct pixel_stream *strm)
{
	pixel_line_to_stream(strm, pixel_yuyvline_to_yuv24line);
}

/* YUYV <-> RGB24 */

__weak void pixel_rgb24line_to_yuyvline_bt709(const uint8_t *rgb24, uint8_t *yuyv, uint16_t width)
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

void pixel_rgb24stream_to_yuyvstream_bt709(struct pixel_stream *strm)
{
	pixel_line_to_stream(strm, pixel_rgb24line_to_yuyvline_bt709);
}

__weak void pixel_yuyvline_to_rgb24line_bt709(const uint8_t *yuyv, uint8_t *rgb24, uint16_t width)
{
	for (size_t i = 0, o = 0, w = 0; w + 2 <= width; w += 2, i += 4, o += 6) {
		/* Pixel 0 */
		pixel_yuv24_to_rgb24_bt709(yuyv[i + 0], yuyv[i + 1], yuyv[i + 3], &rgb24[o + 0]);
		/* Pixel 1 */
		pixel_yuv24_to_rgb24_bt709(yuyv[i + 2], yuyv[i + 1], yuyv[i + 3], &rgb24[o + 3]);
	}
}

void pixel_yuyvstream_to_rgb24stream_bt709(struct pixel_stream *strm)
{
	pixel_line_to_stream(strm, pixel_yuyvline_to_rgb24line_bt709);
}
