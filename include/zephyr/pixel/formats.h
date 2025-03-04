/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PIXEL_FORMATS_H_
#define ZEPHYR_INCLUDE_PIXEL_FORMATS_H_

#include <stdlib.h>
#include <stdint.h>

#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/pixel/stream.h>

/**
 * @brief Get the luminance (luma channel) of an RGB24 pixel.
 *
 * @param rgb24 Pointer to an RGB24 pixel: red, green, blue channels.
 */
uint8_t pixel_rgb24_get_luma_bt709(const uint8_t rgb24[3]);

/**
 * @brief Convert a line of pixel data from RGB332 to RGB24.
 *
 * See @ref video_pixel_formats for the definition of the input and output formats.
 *
 * @param src Buffer of the input line in the format, @c XXX in @c pixel_XXXline_to_YYYline().
 * @param dst Buffer of the output line in the format, @c YYY in @c pixel_XXXline_to_YYYline().
 * @param width Width of the lines in number of pixels.
 */
void pixel_rgb332line_to_rgb24line(const uint8_t *src, uint8_t *dst, uint16_t width);
/**
 * @brief Convert a line of pixel data from RGB24 to RGB332 little-endian.
 * @copydetails pixel_rgb332line_to_rgb24line()
 */
void pixel_rgb24line_to_rgb332line(const uint8_t *src, uint8_t *dst, uint16_t width);
/**
 * @brief Convert a line of pixel data from RGB565 little-endian to RGB24.
 * @copydetails pixel_rgb332line_to_rgb24line()
 */
void pixel_rgb565leline_to_rgb24line(const uint8_t *src, uint8_t *dst, uint16_t width);
/**
 * @brief Convert a line of pixel data from RGB565 big-endian to RGB24.
 * @copydetails pixel_rgb332line_to_rgb24line()
 */
void pixel_rgb565beline_to_rgb24line(const uint8_t *src, uint8_t *dst, uint16_t width);
/**
 * @brief Convert a line of pixel data from RGB24 to RGB565 little-endian.
 * @copydetails pixel_rgb332line_to_rgb24line()
 */
void pixel_rgb24line_to_rgb565leline(const uint8_t *src, uint8_t *dst, uint16_t width);
/**
 * @brief Convert a line of pixel data from RGB24 to RGB565 big-endian.
 * @copydetails pixel_rgb332line_to_rgb24line()
 */
void pixel_rgb24line_to_rgb565beline(const uint8_t *src, uint8_t *dst, uint16_t width);
/**
 * @brief Convert a line of pixel data from YUYV to RGB24 (BT.709 coefficients).
 * @copydetails pixel_rgb332line_to_rgb24line()
 */
void pixel_yuyvline_to_rgb24line_bt709(const uint8_t *src, uint8_t *dst, uint16_t width);
/**
 * @brief Convert a line of pixel data from RGB24 to YUYV (BT.709 coefficients).
 * @copydetails pixel_rgb332line_to_rgb24line()
 */
void pixel_rgb24line_to_yuyvline_bt709(const uint8_t *src, uint8_t *dst, uint16_t width);
/**
 * @brief Convert a line of pixel data from RGB24 to YUV24 (BT.709 coefficients).
 * @copydetails pixel_rgb332line_to_rgb24line()
 */
void pixel_rgb24line_to_yuv24line_bt709(const uint8_t *src, uint8_t *dst, uint16_t width);
/**
 * @brief Convert a line of pixel data from YUV24 to RGB24 (BT.709 coefficients).
 * @copydetails pixel_rgb332line_to_rgb24line()
 */
void pixel_yuv24line_to_rgb24line_bt709(const uint8_t *src, uint8_t *dst, uint16_t width);
/**
 * @brief Convert a line of pixel data from YUYV to YUV24
 * @copydetails pixel_rgb332line_to_rgb24line()
 */
void pixel_yuyvline_to_yuv24line(const uint8_t *src, uint8_t *dst, uint16_t width);
/**
 * @brief Convert a line of pixel data from YUV24 to YUYV
 * @copydetails pixel_rgb332line_to_rgb24line()
 */
void pixel_yuv24line_to_yuyvline(const uint8_t *src, uint8_t *dst, uint16_t width);

/**
 * @brief Convert a stream of pixel data from RGB332 to RGB24.
 *
 * @param name The symbol of the @ref pixel_stream that will be defined.
 * @param width The total width of the input frame in number of pixels.
 * @param height The total height of the input frame in number of pixels.
 */
#define PIXEL_RGB332STREAM_TO_RGB24STREAM(name, width, height)                                     \
	PIXEL_FORMAT_DEFINE(name, pixel_rgb332stream_to_rgb24stream, (width), (height), 1)
/**
 * @brief Convert a stream of pixel data from RGB24 to RGB332
 * @copydetails PIXEL_RGB332STREAM_TO_RGB24STREAM()
 */
#define PIXEL_RGB24STREAM_TO_RGB332STREAM(name, width, height)                                     \
	PIXEL_FORMAT_DEFINE(name, pixel_rgb24stream_to_rgb332stream, (width), (height), 3)
/**
 * @brief Convert a stream of pixel data from RGB565 little-endian to RGB24
 * @copydetails PIXEL_RGB332STREAM_TO_RGB24STREAM()
 */
#define PIXEL_RGB565LESTREAM_TO_RGB24STREAM(name, width, height)                                   \
	PIXEL_FORMAT_DEFINE(name, pixel_rgb565lestream_to_rgb24stream, (width), (height), 2)
/**
 * @brief Convert a stream of pixel data from RGB24 to RGB565 little-endian
 * @copydetails PIXEL_RGB332STREAM_TO_RGB24STREAM()
 */
#define PIXEL_RGB24STREAM_TO_RGB565LESTREAM(name, width, height)                                   \
	PIXEL_FORMAT_DEFINE(name, pixel_rgb24stream_to_rgb565lestream, (width), (height), 3)
/**
 * @brief Convert a stream of pixel data from RGB565 big-endian to RGB24
 * @copydetails PIXEL_RGB332STREAM_TO_RGB24STREAM()
 */
#define PIXEL_RGB565BESTREAM_TO_RGB24STREAM(name, width, height)                                   \
	PIXEL_FORMAT_DEFINE(name, pixel_rgb565bestream_to_rgb24stream, (width), (height), 2)
/**
 * @brief Convert a stream of pixel data from RGB24 to RGB565 big-endian
 * @copydetails PIXEL_RGB332STREAM_TO_RGB24STREAM()
 */
#define PIXEL_RGB24STREAM_TO_RGB565BESTREAM(name, width, height)                                   \
	PIXEL_FORMAT_DEFINE(name, pixel_rgb24stream_to_rgb565bestream, (width), (height), 3)
/**
 * @brief Convert a stream of pixel data from YUYV to RGB24 (BT.709 coefficients)
 * @copydetails PIXEL_RGB332STREAM_TO_RGB24STREAM()
 */
#define PIXEL_YUYVSTREAM_TO_RGB24STREAM_BT709(name, width, height)                                 \
	PIXEL_FORMAT_DEFINE(name, pixel_yuyvstream_to_rgb24stream_bt709, (width), (height), 2)
/**
 * @brief Convert a stream of pixel data from RGB24 to YUYV (BT.709 coefficients)
 * @copydetails PIXEL_RGB332STREAM_TO_RGB24STREAM()
 */
#define PIXEL_RGB24STREAM_TO_YUYVSTREAM_BT709(name, width, height)                                 \
	PIXEL_FORMAT_DEFINE(name, pixel_rgb24stream_to_yuyvstream_bt709, (width), (height), 3)
/**
 * @brief Convert a stream of pixel data from YUV24 to RGB24 (BT.709 coefficients)
 * @copydetails PIXEL_RGB332STREAM_TO_RGB24STREAM()
 */
#define PIXEL_YUV24STREAM_TO_RGB24STREAM_BT709(name, width, height)                                \
	PIXEL_FORMAT_DEFINE(name, pixel_yuv24stream_to_rgb24stream_bt709, (width), (height), 3)
/**
 * @brief Convert a stream of pixel data from RGB24 to YUV24 (BT.709 coefficients)
 * @copydetails PIXEL_RGB332STREAM_TO_RGB24STREAM()
 */
#define PIXEL_RGB24STREAM_TO_YUV24STREAM_BT709(name, width, height)                                \
	PIXEL_FORMAT_DEFINE(name, pixel_rgb24stream_to_yuv24stream_bt709, (width), (height), 3)
/**
 * @brief Convert a stream of pixel data from YUYV to YUV24
 * @copydetails PIXEL_RGB332STREAM_TO_RGB24STREAM()
 */
#define PIXEL_YUYVSTREAM_TO_YUV24STREAM(name, width, height)                                       \
	PIXEL_FORMAT_DEFINE(name, pixel_yuyvstream_to_yuv24stream, (width), (height), 2)
/**
 * @brief Convert a stream of pixel data from YUV24 to YUYV
 * @copydetails PIXEL_RGB332STREAM_TO_RGB24STREAM()
 */
#define PIXEL_YUV24STREAM_TO_YUYVSTREAM(name, width, height)                                       \
	PIXEL_FORMAT_DEFINE(name, pixel_yuv24stream_to_yuyvstream, (width), (height), 3)

/** @cond INTERNAL_HIDDEN */
#define PIXEL_FORMAT_DEFINE(name, fn, width, height, bytes_per_pixel)                              \
	PIXEL_STREAM_DEFINE(name, (fn), (width), (height),                                         \
			    (width) * (bytes_per_pixel), 1 * (width) * (bytes_per_pixel))
void pixel_rgb24stream_to_rgb332stream(struct pixel_stream *strm);
void pixel_rgb332stream_to_rgb24stream(struct pixel_stream *strm);
void pixel_rgb565lestream_to_rgb24stream(struct pixel_stream *strm);
void pixel_rgb24stream_to_rgb565lestream(struct pixel_stream *strm);
void pixel_rgb565bestream_to_rgb24stream(struct pixel_stream *strm);
void pixel_rgb24stream_to_rgb565bestream(struct pixel_stream *strm);
void pixel_yuyvstream_to_rgb24stream_bt709(struct pixel_stream *strm);
void pixel_rgb24stream_to_yuyvstream_bt709(struct pixel_stream *strm);
void pixel_yuv24stream_to_rgb24stream_bt709(struct pixel_stream *strm);
void pixel_rgb24stream_to_yuv24stream_bt709(struct pixel_stream *strm);
void pixel_yuyvstream_to_yuv24stream(struct pixel_stream *strm);
void pixel_yuv24stream_to_yuyvstream(struct pixel_stream *strm);
/** @endcond */

#endif /* ZEPHYR_INCLUDE_PIXEL_FORMATS_H_ */
