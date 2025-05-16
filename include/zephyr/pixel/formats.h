/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PIXEL_FORMATS_H
#define ZEPHYR_INCLUDE_PIXEL_FORMATS_H

#include <stdlib.h>
#include <stdint.h>

#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/pixel/stream.h>

/**
 * Pixel stream definitions.
 * @{
 */

void pixel_rgb24stream_to_rgb565lestream(struct pixel_stream *strm);

#define PIXEL_STREAM_RGB24_TO_RGB565LE(name, width, height)                                        \
	PIXEL_STREAM_DEFINE(name, pixel_rgb24stream_to_rgb565lestream,                             \
			      (width), (height), (width) * 3, 1 * (width) * 3)

void pixel_rgb24stream_to_rgb565bestream(struct pixel_stream *strm);

#define PIXEL_STREAM_RGB24_TO_RGB565BE(name, width, height)                                        \
	PIXEL_STREAM_DEFINE(name, pixel_rgb24stream_to_rgb565bestream,                             \
			      (width), (height), (width) * 3, 1 * (width) * 3)

void pixel_rgb24stream_to_rgb565lestream(struct pixel_stream *strm);

#define PIXEL_STREAM_RGB24_TO_RGB565LE(name, width, height)                                        \
	PIXEL_STREAM_DEFINE(name, pixel_rgb24stream_to_rgb565lestream,                             \
			      (width), (height), (width) * 3, 1 * (width) * 3)

void pixel_rgb24stream_to_rgb565bestream(struct pixel_stream *strm);

#define PIXEL_STREAM_RGB24_TO_RGB565BE(name, width, height)                                        \
	PIXEL_STREAM_DEFINE(name, pixel_rgb24stream_to_rgb565bestream,                             \
			      (width), (height), (width) * 3, 1 * (width) * 3)

void pixel_rgb565lestream_to_rgb24stream(struct pixel_stream *strm);

#define PIXEL_STREAM_RGB565LE_TO_RGB24(name, width, height)                                        \
	PIXEL_STREAM_DEFINE(name, pixel_rgb565lestream_to_rgb24stream,                             \
			      (width), (height), (width) * 2, 1 * (width) * 2)

void pixel_rgb565bestream_to_rgb24stream(struct pixel_stream *strm);

#define PIXEL_STREAM_RGB565BE_TO_RGB24(name, width, height)                                        \
	PIXEL_STREAM_DEFINE(name, pixel_rgb565bestream_to_rgb24stream,                             \
			      (width), (height), (width) * 2, 1 * (width) * 2)

void pixel_yuyvstream_to_rgb24stream_bt601(struct pixel_stream *strm);

#define PIXEL_STREAM_YUYV_TO_RGB24_BT601(name, width, height)                                      \
	PIXEL_STREAM_DEFINE(name, pixel_yuyvstream_to_rgb24stream_bt601,                           \
			      (width), (height), (width) * 2, 1 * (width) * 2)

void pixel_yuyvstream_to_rgb24stream_bt709(struct pixel_stream *strm);

#define PIXEL_STREAM_YUYV_TO_RGB24_BT709(name, width, height)                                      \
	PIXEL_STREAM_DEFINE(name, pixel_yuyvstream_to_rgb24stream_bt709,                           \
			      (width), (height), (width) * 2, 1 * (width) * 2)

void pixel_rgb24stream_to_yuyvstream_bt601(struct pixel_stream *strm);

#define PIXEL_STREAM_RGB24_TO_YUYV_BT601(name, width, height)                                      \
	PIXEL_STREAM_DEFINE(name, pixel_rgb24stream_to_yuyvstream_bt601,                           \
			      (width), (height), (width) * 3, 1 * (width) * 3)

void pixel_rgb24stream_to_yuyvstream_bt709(struct pixel_stream *strm);

#define PIXEL_STREAM_RGB24_TO_YUYV_BT709(name, width, height)                                      \
	PIXEL_STREAM_DEFINE(name, pixel_rgb24stream_to_yuyvstream_bt709,                           \
			      (width), (height), (width) * 3, 1 * (width) * 3)

/** @} */

/**
 * @brief RGB565 little-endian to RGB24 conversion.
 *
 * @param rgb565le Buffer of the input row in RGB565 little-endian format (2 bytes per pixel).
 * @param rgb24 Buffer of the output row in RGB24 format (3 bytes per pixel).
 * @param width Width of the lines in number of pixels.
 */
void pixel_rgb565leline_to_rgb24line(const uint8_t *rgb565le, uint8_t *rgb24, uint16_t width);

/**
 * @brief RGB565 big-endian to RGB24 conversion.
 *
 * @param rgb565be Buffer of the input row in RGB565 big-endian format (2 bytes per pixel).
 * @param rgb24 Buffer of the output row in RGB24 format (3 bytes per pixel).
 * @param width Width of the lines in number of pixels.
 */
void pixel_rgb565beline_to_rgb24line(const uint8_t *rgb565be, uint8_t *rgb24, uint16_t width);

/**
 * @brief RGB24 to RGB565 little-endian conversion.
 *
 * @param rgb24 Buffer of the input row in RGB24 format (3 bytes per pixel).
 * @param rgb565le Buffer of the output row in RGB565 little-endian format (2 bytes per pixel).
 * @param width Width of the lines in number of pixels.
 */
void pixel_rgb24line_to_rgb565leline(const uint8_t *rgb24, uint8_t *rgb565le, uint16_t width);

/**
 * @brief RGB24 to RGB565 big-endian conversion.
 *
 * @param rgb24 Buffer of the input row in RGB24 format (3 bytes per pixel).
 * @param rgb565be Buffer of the output row in RGB565 big-endian format (2 bytes per pixel).
 * @param width Width of the lines in number of pixels.
 */
void pixel_rgb24line_to_rgb565beline(const uint8_t *rgb24, uint8_t *rgb565be, uint16_t width);

/**
 * @brief YUYV to RGB24 conversion.
 *
 * @param yuyv Buffer of the input row in YUYV format (2 bytes per pixel).
 * @param rgb24 Buffer of the output row in RGB24 format (3 bytes per pixel).
 * @param width Width of the lines in number of pixels.
 * @{
 */
/** BT.601 variant */
void pixel_yuyvline_to_rgb24line_bt601(const uint8_t *yuyv, uint8_t *rgb24, uint16_t width);
/** BT.709 variant */
void pixel_yuyvline_to_rgb24line_bt709(const uint8_t *yuyv, uint8_t *rgb24, uint16_t width);
/** @} */

/**
 * @brief RGB24 to YUYV conversion.
 *
 * @param rgb24 Buffer of the input row in RGB24 format (3 bytes per pixel).
 * @param yuyv Buffer of the output row in YUYV format (2 bytes per pixel).
 * @param width Width of the lines in number of pixels.
 * @{
 */
/** BT.601 variant */
void pixel_rgb24line_to_yuyvline_bt601(const uint8_t *rgb24, uint8_t *yuyv, uint16_t width);
/** BT.709 variant */
void pixel_rgb24line_to_yuyvline_bt709(const uint8_t *rgb24, uint8_t *yuyv, uint16_t width);
/** @} */

/* These constants are meant to be inlined by the compiler */

#define Q21(val) ((int32_t)((val) * (1 << 21)))

static const int32_t pixel_yuv_to_r_bt709[] = {Q21(+1.0000), Q21(+0.0000), Q21(+1.5748)};
static const int32_t pixel_yuv_to_g_bt709[] = {Q21(+1.0000), Q21(-0.1873), Q21(-0.4681)};
static const int32_t pixel_yuv_to_b_bt709[] = {Q21(+1.0000), Q21(+1.8556), Q21(+0.0000)};

static const int32_t pixel_yuv_to_r_bt601[] = {Q21(+1.0000), Q21(+0.0000), Q21(+1.4020)};
static const int32_t pixel_yuv_to_g_bt601[] = {Q21(+1.0000), Q21(-0.3441), Q21(-0.7141)};
static const int32_t pixel_yuv_to_b_bt601[] = {Q21(+1.0000), Q21(+1.7720), Q21(+0.0000)};

static const int32_t pixel_rgb_to_y_bt709[] = {Q21(+0.2126), Q21(+0.7152), Q21(+0.0722), 0};
static const int32_t pixel_rgb_to_u_bt709[] = {Q21(-0.1146), Q21(-0.3854), Q21(+0.5000), 128};
static const int32_t pixel_rgb_to_v_bt709[] = {Q21(+0.5000), Q21(-0.4542), Q21(-0.0468), 128};

static const int32_t pixel_rgb_to_y_bt601[] = {Q21(+0.2570), Q21(+0.5040), Q21(+0.0980), 16};
static const int32_t pixel_rgb_to_u_bt601[] = {Q21(-0.1480), Q21(-0.2910), Q21(+0.4390), 128};
static const int32_t pixel_rgb_to_v_bt601[] = {Q21(+0.4390), Q21(-0.3680), Q21(-0.0710), 128};

static const int32_t pixel_rgb_to_y_jpeg[] = {Q21(+0.2990), Q21(+0.5870), Q21(+0.1140), 0};
static const int32_t pixel_rgb_to_u_jpeg[] = {Q21(-0.1690), Q21(-0.3310), Q21(+0.5000), 128};
static const int32_t pixel_rgb_to_v_jpeg[] = {Q21(+0.5000), Q21(-0.4190), Q21(-0.0810), 128};

#undef Q21

static inline uint8_t pixel_yuv24_to_r8g8b8(uint8_t y, uint8_t u, uint8_t v, const int32_t coeff[3])
{
	int32_t y21 = coeff[0] * y;
	int32_t u21 = coeff[1] * ((int32_t)u - 128);
	int32_t v21 = coeff[2] * ((int32_t)v - 128);
	int32_t x16 = (y21 + u21 + v21) >> 21;

	return CLAMP(x16, 0, 0xff);
}

static inline uint8_t pixel_rgb24_to_y8u8v8(const uint8_t rgb24[3], const int32_t coeff[4])
{
	int16_t x16 = (coeff[0] * rgb24[0] + coeff[1] * rgb24[1] + coeff[2] * rgb24[2]) >> 21;

	return CLAMP(x16 + coeff[3], 0, 0xff);
}

/**
 * @brief Convert a pixel from YUV24 format to RGB24 using the BT.601 profile.
 *
 * @param y The 8-bit luma channel value (aka Y).
 * @param u The 8-bit chroma channel value (aka V or Cb).
 * @param v The 8-bit chroma channel value (aka V or Cr).
 * @param rgb24 The pixel in RGB24 format: {R, G, B}
 */
static inline void pixel_yuv24_to_rgb24_bt601(uint8_t y, uint8_t u, uint8_t v, uint8_t rgb24[3])
{
	rgb24[0] = pixel_yuv24_to_r8g8b8(y, u, v, pixel_yuv_to_r_bt601);
	rgb24[1] = pixel_yuv24_to_r8g8b8(y, u, v, pixel_yuv_to_g_bt601);
	rgb24[2] = pixel_yuv24_to_r8g8b8(y, u, v, pixel_yuv_to_b_bt601);
}

/**
 * @brief Convert a pixel from YUV24 format to RGB24 using the BT.709 profile.
 *
 * @param y The 8-bit luma channel value (aka Y).
 * @param u The 8-bit chroma channel value (aka V or Cb).
 * @param v The 8-bit chroma channel value (aka V or Cr).
 * @param rgb24 The pixel in RGB24 format: {R, G, B}
 */
static inline void pixel_yuv24_to_rgb24_bt709(uint8_t y, uint8_t u, uint8_t v, uint8_t rgb24[3])
{
	rgb24[0] = pixel_yuv24_to_r8g8b8(y, u, v, pixel_yuv_to_r_bt709);
	rgb24[1] = pixel_yuv24_to_r8g8b8(y, u, v, pixel_yuv_to_g_bt709);
	rgb24[2] = pixel_yuv24_to_r8g8b8(y, u, v, pixel_yuv_to_b_bt709);
}

/**
 * @brief Convert two YUYV pixels to two RGB24 pixels using the BT.601 profile.
 *
 * @param yuyv The 2 pixels in YUYV format: {Y, U, Y, V}
 * @param rgb24x2 The 2 pixels in RGB24 format: {R, G, B, R, G, B}
 */
static inline void pixel_yuyv_to_rgb24x2_bt601(const uint8_t yuyv[4], uint8_t rgb24x2[6])
{
	pixel_yuv24_to_rgb24_bt601(yuyv[0], yuyv[1], yuyv[3], &rgb24x2[0]);
	pixel_yuv24_to_rgb24_bt601(yuyv[2], yuyv[1], yuyv[3], &rgb24x2[3]);
}

/**
 * @brief Convert two YUYV pixels to two RGB24 pixels.
 *
 * @param yuyv The 2 pixels in YUYV format: {Y, U, Y, V}
 * @param rgb24x2 The 2 pixels in RGB24 format: {R, G, B, R, G, B}
 */
static inline void pixel_yuyv_to_rgb24x2_bt709(const uint8_t yuyv[4], uint8_t rgb24x2[6])
{
	pixel_yuv24_to_rgb24_bt709(yuyv[0], yuyv[1], yuyv[3], &rgb24x2[0]);
	pixel_yuv24_to_rgb24_bt709(yuyv[2], yuyv[1], yuyv[3], &rgb24x2[3]);
}

/**
 * @brief Convert a pixel from RGB24 to Y8 format (Y channel of YUV24) using the BT.601 profile.
 *
 * @param rgb24 The pixel in RGB24 format: {R, G, B}
 * @return The Y channel for this RGB value, also known as "perceived lightness".
 */
static inline uint8_t pixel_rgb24_to_y8_bt601(const uint8_t rgb24[3])
{
	return pixel_rgb24_to_y8u8v8(rgb24, pixel_rgb_to_y_bt601);
}

/**
 * @brief Convert a pixel from RGB24 to Y8 format (Y channel of YUV24) using the BT.709 profile.
 *
 * @param rgb24 The pixel in RGB24 format: {R, G, B}
 * @return The Y channel for this RGB value, also known as "perceived lightness".
 */
static inline uint8_t pixel_rgb24_to_y8_bt709(const uint8_t rgb24[3])
{
	return pixel_rgb24_to_y8u8v8(rgb24, pixel_rgb_to_y_bt709);
}

/**
 * @brief Convert a pixel from RGB24 format to YUV24 using the BT.601 profile.
 *
 * @param rgb24 The pixel in RGB24 format: {R, G, B}
 * @param yuv24 The pixel in YUV24 format: {Y, U, V}
 */
static inline void pixel_rgb24_to_yuv24_bt601(const uint8_t rgb24[3], uint8_t yuv24[3])
{
	yuv24[0] = pixel_rgb24_to_y8u8v8(rgb24, pixel_rgb_to_y_bt601);
	yuv24[1] = pixel_rgb24_to_y8u8v8(rgb24, pixel_rgb_to_u_bt601);
	yuv24[2] = pixel_rgb24_to_y8u8v8(rgb24, pixel_rgb_to_v_bt601);
}

/**
 * @brief Convert a pixel from RGB24 format to YUV24 using the BT.709 profile.
 *
 * @param rgb24 The pixel in RGB24 format: {R, G, B}
 * @param yuv24 The pixel in YUV24 format: {Y, U, V}
 */
static inline void pixel_rgb24_to_yuv24_bt709(const uint8_t rgb24[3], uint8_t yuv24[3])
{
	yuv24[0] = pixel_rgb24_to_y8u8v8(rgb24, pixel_rgb_to_y_bt709);
	yuv24[1] = pixel_rgb24_to_y8u8v8(rgb24, pixel_rgb_to_u_bt709);
	yuv24[2] = pixel_rgb24_to_y8u8v8(rgb24, pixel_rgb_to_v_bt709);
}

/**
 * @brief Convert two pixels from RGB24 format to YUYV format using the BT.601 profile.
 *
 * @param rgb24x2 The 2 pixels in RGB24 format: {R, G, B, R, G, B}
 * @param yuyv The 2 pixels in YUYV format: {Y, U, Y, V}
 */
static inline void pixel_rgb24x2_to_yuyv_bt601(const uint8_t rgb24x2[6], uint8_t yuyv[4])
{
	yuyv[0] = pixel_rgb24_to_y8u8v8(&rgb24x2[0], pixel_rgb_to_y_bt601);
	yuyv[1] = pixel_rgb24_to_y8u8v8(&rgb24x2[0], pixel_rgb_to_u_bt601);
	yuyv[2] = pixel_rgb24_to_y8u8v8(&rgb24x2[3], pixel_rgb_to_y_bt601);
	yuyv[3] = pixel_rgb24_to_y8u8v8(&rgb24x2[3], pixel_rgb_to_v_bt601);
}

/**
 * @brief Convert two pixels from RGB24 format to YUYV format using the BT.709 profile.
 *
 * @param rgb24x2 The 2 pixels in RGB24 format: {R, G, B, R, G, B}
 * @param yuyv The 2 pixels in YUYV format: {Y, U, Y, V}
 */
static inline void pixel_rgb24x2_to_yuyv_bt709(const uint8_t rgb24x2[6], uint8_t yuyv[4])
{
	yuyv[0] = pixel_rgb24_to_y8u8v8(&rgb24x2[0], pixel_rgb_to_y_bt709);
	yuyv[1] = pixel_rgb24_to_y8u8v8(&rgb24x2[0], pixel_rgb_to_u_bt709);
	yuyv[2] = pixel_rgb24_to_y8u8v8(&rgb24x2[3], pixel_rgb_to_y_bt709);
	yuyv[3] = pixel_rgb24_to_y8u8v8(&rgb24x2[3], pixel_rgb_to_v_bt709);
}

/**
 * @brief Convert a pixel from RGB565 format to RGB24.
 *
 * @param rgb565 The pixel in RGB565 format, in CPU-native endianness
 * @param rgb24 The pixel in RGB24 format: {R, G, B}
 */
static inline void pixel_rgb565_to_rgb24(uint16_t rgb565, uint8_t rgb24[3])
{
	rgb24[0] = rgb565 >> (0 + 6 + 5) << 3;
	rgb24[1] = rgb565 >> (0 + 0 + 5) << 2;
	rgb24[2] = rgb565 >> (0 + 0 + 0) << 3;
}

/**
 * @brief Convert a pixel from RGB565 format to RGB24.
 *
 * @param rgb565le The pixel in RGB565 format, in little-endian.
 * @param rgb24 The pixel in RGB24 format: {R, G, B}
 */
static inline void pixel_rgb565le_to_rgb24(const uint8_t rgb565le[2], uint8_t rgb24[3])
{
	pixel_rgb565_to_rgb24(sys_le16_to_cpu(*(uint16_t *)rgb565le), rgb24);
}

/**
 * @brief Convert a pixel from RGB565 big-endian format to RGB24.
 *
 * @param rgb565be The pixel in RGB565 format, in big-endian.
 * @param rgb24 The pixel in RGB24 format: {R, G, B}
 */
static inline void pixel_rgb565be_to_rgb24(const uint8_t rgb565be[2], uint8_t rgb24[3])
{
	pixel_rgb565_to_rgb24(sys_be16_to_cpu(*(uint16_t *)rgb565be), rgb24);
}

/**
 * @brief Convert a pixel from RGB24 format to RGB565.
 *
 * @param rgb24 The pixel in RGB24 format: {R, G, B}
 * @return The pixel in RGB565 format, in CPU-native endianness
 */
static inline uint16_t pixel_rgb24_to_rgb565(const uint8_t rgb24[3])
{
	uint16_t rgb565 = 0;

	rgb565 |= (uint16_t)rgb24[0] >> 3 << (0 + 6 + 5);
	rgb565 |= (uint16_t)rgb24[1] >> 2 << (0 + 0 + 5);
	rgb565 |= (uint16_t)rgb24[2] >> 3 << (0 + 0 + 0);

	return rgb565;
}

/**
 * @brief Convert a pixel from RGB24 format to RGB565 little-endian.
 *
 * @param rgb24 The pixel in RGB24 format: {R, G, B}
 * @param rgb565le The pixel in RGB565 format, in little-endian.
 */
static inline void pixel_rgb24_to_rgb565le(const uint8_t rgb24[3], uint8_t rgb565le[2])
{
	*(uint16_t *)rgb565le = sys_cpu_to_le16(pixel_rgb24_to_rgb565(rgb24));
}

/**
 * @brief Convert a pixel from RGB24 format to RGB565 big-endian.
 *
 * @param rgb24 The pixel in RGB24 format: {R, G, B}
 * @param rgb565be The pixel in RGB565 format, in big-endian.
 */
static inline void pixel_rgb24_to_rgb565be(const uint8_t rgb24[3], uint8_t rgb565be[2])
{
	*(uint16_t *)rgb565be = sys_cpu_to_be16(pixel_rgb24_to_rgb565(rgb24));
}

/**
 * @brief Convert a pixel from RGB24 to ANSI 8-bit format (color cube of 6x6x6 colors).
 *
 * @param rgb24 The pixel in RGB24 format: {R, G, B}
 * @return Colors in ANSI escape code color format packed in a single 8-bit value.
 */
static inline uint8_t pixel_rgb24_to_256color(const uint8_t rgb24[3])
{
	return 16 + rgb24[0] * 6 / 256 * 36 + rgb24[1] * 6 / 256 * 6 + rgb24[2] * 6 / 256 * 1;
}

/**
 * @brief Convert a pixel from GRAY8 to ANSI 8-bit black and white format (grayscale of 24 values)
 *
 * @param gray8 The pixel in GRAY8 format: 8-bit value.
 * @return Colors in ANSI escape code color format packed in a single 8-bit value.
 */
static inline uint8_t pixel_gray8_to_256color(uint8_t gray8)
{
	return 232 + gray8 * 24 / 256;
}

#endif /* ZEPHYR_INCLUDE_PIXEL_FORMATS_H */
