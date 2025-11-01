/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for MpPixelFormat.
 */

#ifndef __MP_PIXEL_FORMAT_H__
#define __MP_PIXEL_FORMAT_H__

/**
 * @brief MP_PIXEL_FORMAT id
 *
 * Supported pixel formats of a media type e.g. video, display.
 */
typedef enum {
	/** Unknown or unsupported pixel format id */
	MP_PIXEL_FORMAT_UNKNOWN,
	/** Encoded formats such as H.264, H.265, etc. */
	MP_PIXEL_FORMAT_ENCODED,
	/** Monochrome (0 = Black and 1 = White) */
	MP_PIXEL_FORMAT_MONO01,
	/** Monochrome (0 = White and 1 = Black) */
	MP_PIXEL_FORMAT_MONO10,
	/** 8-bit grey scale luma-only */
	MP_PIXEL_FORMAT_GREY8,
	/**8-bit grey scale luminance with 8-bit alpha channel first */
	MP_PIXEL_FORMAT_AL88,
	/**
	 * Chroma (U/V) are subsampled horizontaly and vertically
	 *
	 * @code{.unparsed}
	 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | ...
	 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | ...
	 * | ... |
	 * | Uuuuuuuu   Vvvvvvvv | Uuuuuuuu   Vvvvvvvv | ...
	 * | ... |
	 * @endcode
	 *
	 * Below diagram show how luma and chroma relate to each others
	 *
	 * @code{.unparsed}
	 *  Y0        Y1        Y2        Y3        ...
	 *  Y6        Y7        Y8        Y9        ...
	 *  ...
	 *
	 *  U0/1/6/7  V0/1/6/7  U2/3/8/9  V2/3/8/9  ...
	 *  ...
	 * @endcode
	 */
	MP_PIXEL_FORMAT_NV12,
	/**
	 * Chroma (U/V) are subsampled horizontaly and vertically
	 *
	 * @code{.unparsed}
	 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | ...
	 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | ...
	 * | ... |
	 * | Vvvvvvvv   Uuuuuuuu | Vvvvvvvv   Uuuuuuuu | ...
	 * | ... |
	 * @endcode
	 *
	 * Below diagram show how luma and chroma relate to each others
	 *
	 * @code{.unparsed}
	 *  Y0        Y1        Y2        Y3        ...
	 *  Y6        Y7        Y8        Y9        ...
	 *  ...
	 *
	 *  V0/1/6/7  U0/1/6/7  V2/3/8/9  U2/3/8/9  ...
	 *  ...
	 * @endcode
	 */
	MP_PIXEL_FORMAT_NV21,
	/** 
	 * YUV format: Luminance (Y) and chrominance (U, V) channels.
	 * 
	 * @code{.unparsed}
	 * | Uuuuuuuu Yyyyyyyy | Vvvvvvvv Yyyyyyyy | ...
	 * @endcode
	 */
	MP_PIXEL_FORMAT_UYVY,
	/**
	 * There is either a missing channel per pixel, U or V.
	 * The value is to be averaged over 2 pixels to get the value of individual pixel.
	 *
	 * @code{.unparsed}
	 * | Yyyyyyyy Uuuuuuuu | Yyyyyyyy Vvvvvvvv | ...
	 * @endcode
	 */
	MP_PIXEL_FORMAT_YUYV,
	/**
	 * The first byte is empty (X) for each pixel.
	 *
	 * @code{.unparsed}
	 * | Xxxxxxxx Yyyyyyyy Uuuuuuuu Vvvvvvvv | ...
	 * @endcode
	 */
	MP_PIXEL_FORMAT_XYUV32,
	/**
	 * 5 red bits [15:11], 6 green bits [10:5], 5 blue bits [4:0].
	 * This 16-bit integer is then packed in little endian format over two bytes:
	 *
	 * @code{.unparsed}
	 *   7......0 15.....8
	 * | gggBbbbb RrrrrGgg | ...
	 * @endcode
	 */
	MP_PIXEL_FORMAT_RGB565,
	/**
	 * Like @ref MP_PIXEL_FORMAT_RGB565 with R and B swapped
	 */
	MP_PIXEL_FORMAT_BGR565,
	/**
	 * 24 bit RGB format with 8 bit per component
	 *
	 * @code{.unparsed}
	 * | Rggggggg Gggggggg Bbbbbbbb | ...
	 * @endcode
	 */
	MP_PIXEL_FORMAT_RGB24,
	/**
	 * 32-bit RGB with 8-bit alpha channel first
	 * 
	 * @code{.unparsed}
	 * | Aaaaaaaa Rrrrrrrr Gggggggg Bbbbbbbb | ...
	 * @endcode
	 */
	MP_PIXEL_FORMAT_ARGB32,
	/**
	 * 32-bit RGB with the first 8-bit (X) ignored
	 *
	 * @code{.unparsed}
	 * | Xxxxxxxx Rrrrrrrr Gggggggg Bbbbbbbb | ...
	 * @endcode
	 */
	MP_PIXEL_FORMAT_XRGB32,
} MpPixelFormat;

#endif /* __MP_PIXEL_FORMAT_H__ */
