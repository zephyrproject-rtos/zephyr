/*
 * Copyright (c) 2019 Linaro Limited.
 * Copyright 2025 NXP
 * Copyright (c) 2025 STMicroelectronics
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup video_format
 * @brief Main header file for video format API.
 */

#ifndef ZEPHYR_INCLUDE_VIDEO_FORMATS_H
#define ZEPHYR_INCLUDE_VIDEO_FORMATS_H

/**
 * @brief Video pixel format definitions.
 * @defgroup video_formats Video Formats
 * @since 4.4
 * @version 0.0.1
 * @ingroup video
 *
 * The '|' characters separate the pixels or logical blocks, and spaces separate the bytes.
 * The uppercase letter represents the most significant bit.
 * The lowercase letters represent the rest of the bits.
 *
 * @{
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Four-character-code uniquely identifying the pixel format
 */
#define VIDEO_FOURCC(a, b, c, d)                                                                   \
	((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

/**
 * @brief Convert a four-character-string to a four-character-code
 *
 * Convert a string literal or variable into a four-character-code
 * as defined by @ref VIDEO_FOURCC.
 *
 * @param str String to be converted
 * @return Four-character-code.
 */
#define VIDEO_FOURCC_FROM_STR(str) VIDEO_FOURCC((str)[0], (str)[1], (str)[2], (str)[3])

/**
 * @brief Convert a four-character-code to a four-character-string
 *
 * Convert a four-character code as defined by @ref VIDEO_FOURCC into a string that can be used
 * anywhere, such as in debug logs with the %s print formatter.
 *
 * @param fourcc The 32-bit four-character-code integer to be converted, in CPU-native endinaness.
 * @return Four-character-string built out of it.
 */
#define VIDEO_FOURCC_TO_STR(fourcc)                                                                \
	((char[]){                                                                                 \
		(char)((fourcc) & 0xFF),                                                           \
		(char)(((fourcc) >> 8) & 0xFF),                                                    \
		(char)(((fourcc) >> 16) & 0xFF),                                                   \
		(char)(((fourcc) >> 24) & 0xFF),                                                   \
		'\0'                                                                               \
	})

/**
 * @name Bayer formats (R, G, B channels).
 *
 * The full color information is spread over multiple pixels.
 *
 * When the format includes more than 8-bit per pixel, a strategy becomes needed to pack
 * the bits over multiple bytes, as illustrated for each format.
 *
 * The number above the 'R', 'r', 'G', 'g', 'B', 'b' are hints about which pixel number the
 * following bits belong to.
 *
 * @{
 */

/**
 * @brief Repeat a macro for every Bayer format, passed as first parameter
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define VIDEO_FOREACH_BAYER(X, ...)						\
	VIDEO_FOREACH_BAYER_PADDED(X, __VA_ARGS__)				\
	VIDEO_FOREACH_BAYER_MIPI_PACKED(X, __VA_ARGS__)				\
	VIDEO_FOREACH_BAYER_NON_PACKED(X, __VA_ARGS__)

/**
 * @brief Repeat a macro for every Bayer padded format, passed as first parameter
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define VIDEO_FOREACH_BAYER_PADDED(X, ...)					\
	X(VIDEO_PIX_FMT_SBGGR8P16, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_SGBRG8P16, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_SGRBG8P16, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_SRGGB8P16, __VA_ARGS__)

/**
 * 8-bit bayer format, split in two 4-bit blocks each zero-padded, little endian.
 *
 * @code{.unparsed}
 *   0                   1                   2                   3
 * | 0000bbbb 0000Bbbb | 0000gggg 0000Gggg | 0000gggg 0000Gggg | 0000rrrr 0000Rrrr | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SBGGR8P16 VIDEO_FOURCC('p', 'B', '8', '2')

/**
 * 8-bit bayer format, split in two 4-bit blocks each zero-padded, little endian.
 *
 * @code{.unparsed}
 *   0                   1                   2                   3
 * | 0000gggg 0000Gggg | 0000bbbb 0000Bbbb | 0000rrrr 0000Rrrr | 0000gggg 0000Gggg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGBRG8P16 VIDEO_FOURCC('p', 'G', '8', '2')

/**
 * 8-bit bayer format, split in two 4-bit blocks each zero-padded, little endian.
 *
 * @code{.unparsed}
 *   0                   1                   2                   3
 * | 0000gggg 0000Gggg | 0000rrrr 0000Rrrr | 0000bbbb 0000Bbbb | 0000gggg 0000Gggg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGRBG8P16 VIDEO_FOURCC('p', 'g', '8', '2')

/**
 * 8-bit bayer format, split in two 4-bit blocks each zero-padded, little endian.
 *
 * @code{.unparsed}
 *   0                   1                   2                   3
 * | 0000rrrr 0000Rrrr | 0000gggg 0000Gggg | 0000gggg 0000Gggg | 0000bbbb 0000Bbbb | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SRGGB8P16 VIDEO_FOURCC('p', 'R', '8', '2')

/**
 * @brief Repeat a macro for every Bayer MIPI-packed format, passed as first parameter
 *
 * A red, green, blue channel for every pixel, other than 8-bit per pixel.
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define VIDEO_FOREACH_BAYER_MIPI_PACKED(X, ...)					\
	X(VIDEO_PIX_FMT_SBGGR10P, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_SGBRG10P, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_SGRBG10P, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_SRGGB10P, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_SBGGR12P, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_SGBRG12P, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_SGRBG12P, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_SRGGB12P, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_SBGGR14P, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_SGBRG14P, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_SGRBG14P, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_SRGGB14P, __VA_ARGS__)

/**
 * @code{.unparsed}
 *   0          1          2          3          3 2 1 0
 * | Bbbbbbbb | Gggggggg | Bbbbbbbb | Gggggggg | ggbbggbb | ...
 * | Gggggggg | Rrrrrrrr | Gggggggg | Rrrrrrrr | rrggrrgg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SBGGR10P VIDEO_FOURCC('p', 'B', 'A', 'A')

/**
 * @code{.unparsed}
 *   0          1          2          3          3 2 1 0
 * | Gggggggg | Bbbbbbbb | Gggggggg | Bbbbbbbb | bbggbbgg | ...
 * | Rrrrrrrr | Gggggggg | Rrrrrrrr | Gggggggg | ggrrggrr | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGBRG10P VIDEO_FOURCC('p', 'G', 'A', 'A')

/**
 * @code{.unparsed}
 *   0          1          2          3          3 2 1 0
 * | Gggggggg | Rrrrrrrr | Gggggggg | Rrrrrrrr | rrggrrgg | ...
 * | Bbbbbbbb | Gggggggg | Bbbbbbbb | Gggggggg | ggbbggbb | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGRBG10P VIDEO_FOURCC('p', 'g', 'A', 'A')

/**
 * @code{.unparsed}
 *   0          1          2          3          3 2 1 0
 * | Rrrrrrrr | Gggggggg | Rrrrrrrr | Gggggggg | ggrrggrr | ...
 * | Gggggggg | Bbbbbbbb | Gggggggg | Bbbbbbbb | bbggbbgg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SRGGB10P VIDEO_FOURCC('p', 'R', 'A', 'A')

/**
 * @code{.unparsed}
 *   0          1          1   0      2          3          3   2
 * | Bbbbbbbb | Gggggggg | ggggbbbb | Bbbbbbbb | Gggggggg | ggggbbbb | ...
 * | Gggggggg | Rrrrrrrr | rrrrgggg | Gggggggg | Rrrrrrrr | rrrrgggg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SBGGR12P VIDEO_FOURCC('p', 'B', 'C', 'C')

/**
 * @code{.unparsed}
 *   0          1          1   0      2          3          3   2
 * | Gggggggg | Bbbbbbbb | bbbbgggg | Gggggggg | Bbbbbbbb | bbbbgggg | ...
 * | Rrrrrrrr | Gggggggg | ggggrrrr | Rrrrrrrr | Gggggggg | ggggrrrr | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGBRG12P VIDEO_FOURCC('p', 'G', 'C', 'C')

/**
 * @code{.unparsed}
 *   0          1          1   0      2          3          3   2
 * | Gggggggg | Rrrrrrrr | rrrrgggg | Gggggggg | Rrrrrrrr | rrrrgggg | ...
 * | Bbbbbbbb | Gggggggg | ggggbbbb | Bbbbbbbb | Gggggggg | ggggbbbb | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGRBG12P VIDEO_FOURCC('p', 'g', 'C', 'C')

/**
 * @code{.unparsed}
 *   0          1          1   0      2          3          3   2
 * | Rrrrrrrr | Gggggggg | ggggrrrr | Rrrrrrrr | Gggggggg | ggggrrrr | ...
 * | Gggggggg | Bbbbbbbb | bbbbgggg | Gggggggg | Bbbbbbbb | bbbbgggg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SRGGB12P VIDEO_FOURCC('p', 'R', 'C', 'C')

/**
 * @code{.unparsed}
 *   0          1          2          3          1 0      2   1    3     2
 * | Bbbbbbbb | Gggggggg | Bbbbbbbb | Gggggggg | ggbbbbbb bbbbgggg ggggggbb | ...
 * | Gggggggg | Rrrrrrrr | Gggggggg | Rrrrrrrr | rrgggggg ggggrrrr rrrrrrgg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SBGGR14P VIDEO_FOURCC('p', 'B', 'E', 'E')

/**
 * @code{.unparsed}
 *   0          1          2          3          1 0      2   1    3     2
 * | Gggggggg | Bbbbbbbb | Gggggggg | Bbbbbbbb | bbgggggg ggggbbbb bbbbbbgg | ...
 * | Rrrrrrrr | Gggggggg | Rrrrrrrr | Gggggggg | ggrrrrrr rrrrgggg ggggggrr | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGBRG14P VIDEO_FOURCC('p', 'G', 'E', 'E')

/**
 * @code{.unparsed}
 *   0          1          2          3          1 0      2   1    3     2
 * | Gggggggg | Rrrrrrrr | Gggggggg | Rrrrrrrr | rrgggggg ggggrrrr rrrrrrgg | ...
 * | Bbbbbbbb | Gggggggg | Bbbbbbbb | Gggggggg | ggbbbbbb bbbbgggg ggggggbb | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGRBG14P VIDEO_FOURCC('p', 'g', 'E', 'E')

/**
 * @code{.unparsed}
 *   0          1          2          3          1 0      2   1    3     2
 * | Rrrrrrrr | Gggggggg | Rrrrrrrr | Gggggggg | ggrrrrrr rrrrgggg ggggggrr | ...
 * | Gggggggg | Bbbbbbbb | Gggggggg | Bbbbbbbb | bbgggggg ggggbbbb bbbbbbgg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SRGGB14P VIDEO_FOURCC('p', 'R', 'E', 'E')

/**
 * @brief Repeat a macro for every Bayer non-packed format, passed as first parameter
 *
 * A red, green, blue channel for every pixel, other than 8-bit per pixel.
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define VIDEO_FOREACH_BAYER_NON_PACKED(X, ...)					\
	X(VIDEO_PIX_FMT_SBGGR8, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_SGBRG8, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_SGRBG8, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_SRGGB8, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_SGBRG10, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_SGRBG10, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_SRGGB10, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_SBGGR12, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_SGBRG12, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_SGRBG12, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_SRGGB12, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_SBGGR14, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_SGBRG14, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_SGRBG14, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_SRGGB14, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_SBGGR16, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_SGBRG16, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_SGRBG16, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_SRGGB16, __VA_ARGS__)

/**
 * @code{.unparsed}
 *   0          1          2          3
 * | Bbbbbbbb | Gggggggg | Bbbbbbbb | Gggggggg | ...
 * | Gggggggg | Rrrrrrrr | Gggggggg | Rrrrrrrr | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SBGGR8 VIDEO_FOURCC('B', 'A', '8', '1')

/**
 * @code{.unparsed}
 *   0          1          2          3
 * | Gggggggg | Bbbbbbbb | Gggggggg | Bbbbbbbb | ...
 * | Rrrrrrrr | Gggggggg | Rrrrrrrr | Gggggggg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGBRG8 VIDEO_FOURCC('G', 'B', 'R', 'G')

/**
 * @code{.unparsed}
 *   0          1          2          3
 * | Gggggggg | Rrrrrrrr | Gggggggg | Rrrrrrrr | ...
 * | Bbbbbbbb | Gggggggg | Bbbbbbbb | Gggggggg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGRBG8 VIDEO_FOURCC('G', 'R', 'B', 'G')

/**
 * @code{.unparsed}
 *   0          1          2          3
 * | Rrrrrrrr | Gggggggg | Rrrrrrrr | Gggggggg | ...
 * | Gggggggg | Bbbbbbbb | Gggggggg | Bbbbbbbb | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SRGGB8 VIDEO_FOURCC('R', 'G', 'G', 'B')

/**
 * @code{.unparsed}
 * | bbbbbbbb 000000Bb | gggggggg 000000Gg | bbbbbbbb 000000Bb | gggggggg 000000Gg | ...
 * | gggggggg 000000Gg | rrrrrrrr 000000Rr | gggggggg 000000Gg | rrrrrrrr 000000Rr | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SBGGR10 VIDEO_FOURCC('B', 'G', '1', '0')

/**
 * @code{.unparsed}
 * | gggggggg 000000Gg | bbbbbbbb 000000Bb | gggggggg 000000Gg | bbbbbbbb 000000Bb | ...
 * | rrrrrrrr 000000Rr | gggggggg 000000Gg | rrrrrrrr 000000Rr | gggggggg 000000Gg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGBRG10 VIDEO_FOURCC('G', 'B', '1', '0')

/**
 * @code{.unparsed}
 * | gggggggg 000000Gg | rrrrrrrr 000000Rr | gggggggg 000000Gg | rrrrrrrr 000000Rr | ...
 * | bbbbbbbb 000000Bb | gggggggg 000000Gg | bbbbbbbb 000000Bb | gggggggg 000000Gg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGRBG10 VIDEO_FOURCC('B', 'A', '1', '0')

/**
 * @code{.unparsed}
 * | rrrrrrrr 000000Rr | gggggggg 000000Gg | rrrrrrrr 000000Rr | gggggggg 000000Gg | ...
 * | gggggggg 000000Gg | bbbbbbbb 000000Bb | gggggggg 000000Gg | bbbbbbbb 000000Bb | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SRGGB10 VIDEO_FOURCC('R', 'G', '1', '0')

/**
 * @code{.unparsed}
 * | bbbbbbbb 0000Bbbb | gggggggg 0000Gggg | bbbbbbbb 0000Bbbb | gggggggg 0000Gggg | ...
 * | gggggggg 0000Gggg | rrrrrrrr 0000Rrrr | gggggggg 0000Gggg | rrrrrrrr 0000Rrrr | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SBGGR12 VIDEO_FOURCC('B', 'G', '1', '2')

/**
 * @code{.unparsed}
 * | gggggggg 0000Gggg | bbbbbbbb 0000Bbbb | gggggggg 0000Gggg | bbbbbbbb 0000Bbbb | ...
 * | rrrrrrrr 0000Rrrr | gggggggg 0000Gggg | rrrrrrrr 0000Rrrr | gggggggg 0000Gggg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGBRG12 VIDEO_FOURCC('G', 'B', '1', '2')

/**
 * @code{.unparsed}
 * | gggggggg 0000Gggg | rrrrrrrr 0000Rrrr | gggggggg 0000Gggg | rrrrrrrr 0000Rrrr | ...
 * | bbbbbbbb 0000Bbbb | gggggggg 0000Gggg | bbbbbbbb 0000Bbbb | gggggggg 0000Gggg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGRBG12 VIDEO_FOURCC('B', 'A', '1', '2')

/**
 * @code{.unparsed}
 * | rrrrrrrr 0000Rrrr | gggggggg 0000Gggg | rrrrrrrr 0000Rrrr | gggggggg 0000Gggg | ...
 * | gggggggg 0000Gggg | bbbbbbbb 0000Bbbb | gggggggg 0000Gggg | bbbbbbbb 0000Bbbb | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SRGGB12 VIDEO_FOURCC('R', 'G', '1', '2')

/**
 * @code{.unparsed}
 * | bbbbbbbb 00Bbbbbb | gggggggg 00Gggggg | bbbbbbbb 00Bbbbbb | gggggggg 00Gggggg | ...
 * | gggggggg 00Gggggg | rrrrrrrr 00Rrrrrr | gggggggg 00Gggggg | rrrrrrrr 00Rrrrrr | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SBGGR14 VIDEO_FOURCC('B', 'G', '1', '4')

/**
 * @code{.unparsed}
 * | gggggggg 00Gggggg | bbbbbbbb 00Bbbbbb | gggggggg 00Gggggg | bbbbbbbb 00Bbbbbb | ...
 * | rrrrrrrr 00Rrrrrr | gggggggg 00Gggggg | rrrrrrrr 00Rrrrrr | gggggggg 00Gggggg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGBRG14 VIDEO_FOURCC('G', 'B', '1', '4')

/**
 * @code{.unparsed}
 * | gggggggg 00Gggggg | rrrrrrrr 00Rrrrrr | gggggggg 00Gggggg | rrrrrrrr 00Rrrrrr | ...
 * | bbbbbbbb 00Bbbbbb | gggggggg 00Gggggg | bbbbbbbb 00Bbbbbb | gggggggg 00Gggggg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGRBG14 VIDEO_FOURCC('G', 'R', '1', '4')

/**
 * @code{.unparsed}
 * | rrrrrrrr 00Rrrrrr | gggggggg 00Gggggg | rrrrrrrr 00Rrrrrr | gggggggg 00Gggggg | ...
 * | gggggggg 00Gggggg | bbbbbbbb 00Bbbbbb | gggggggg 00Gggggg | bbbbbbbb 00Bbbbbb | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SRGGB14 VIDEO_FOURCC('R', 'G', '1', '4')

/**
 * @code{.unparsed}
 * | bbbbbbbb Bbbbbbbb | gggggggg Gggggggg | bbbbbbbb Bbbbbbbb | gggggggg Gggggggg | ...
 * | gggggggg Gggggggg | rrrrrrrr Rrrrrrrr | gggggggg Gggggggg | rrrrrrrr Rrrrrrrr | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SBGGR16 VIDEO_FOURCC('B', 'Y', 'R', '2')

/**
 * @code{.unparsed}
 * | gggggggg Gggggggg | bbbbbbbb Bbbbbbbb | gggggggg Gggggggg | bbbbbbbb Bbbbbbbb | ...
 * | rrrrrrrr Rrrrrrrr | gggggggg Gggggggg | rrrrrrrr Rrrrrrrr | gggggggg Gggggggg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGBRG16 VIDEO_FOURCC('G', 'B', '1', '6')

/**
 * @code{.unparsed}
 * | gggggggg Gggggggg | rrrrrrrr Rrrrrrrr | gggggggg Gggggggg | rrrrrrrr Rrrrrrrr | ...
 * | bbbbbbbb Bbbbbbbb | gggggggg Gggggggg | bbbbbbbb Bbbbbbbb | gggggggg Gggggggg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGRBG16 VIDEO_FOURCC('G', 'R', '1', '6')

/**
 * @code{.unparsed}
 * | rrrrrrrr Rrrrrrrr | gggggggg Gggggggg | rrrrrrrr Rrrrrrrr | gggggggg Gggggggg | ...
 * | gggggggg Gggggggg | bbbbbbbb Bbbbbbbb | gggggggg Gggggggg | bbbbbbbb Bbbbbbbb | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SRGGB16 VIDEO_FOURCC('R', 'G', '1', '6')

/**
 * @}
 */

/**
 * @name Grayscale formats
 * Luminance (Y) channel only, in various bit depth and packing.
 *
 * When the format includes more than 8-bit per pixel, a strategy becomes needed to pack
 * the bits over multiple bytes, as illustrated for each format.
 *
 * The number above the 'Y', 'y' are hints about which pixel number the following bits belong to.
 *
 * @{
 */

/**
 * @brief Repeat a macro for every grayscale format, passed as first parameter
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define VIDEO_FOREACH_GRAYSCALE(X, ...)						\
	VIDEO_FOREACH_GRAYSCALE_NON_PACKED(X, __VA_ARGS__)			\
	VIDEO_FOREACH_GRAYSCALE_PADDED(X, __VA_ARGS__)				\
	VIDEO_FOREACH_GRAYSCALE_MIPI_PACKED(X, __VA_ARGS__)

/**
 * @brief Repeat a macro for every grayscale non-packed format, passed as first parameter
 *
 * A red, green, blue channel for every pixel, other than 8-bit per pixel.
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define VIDEO_FOREACH_GRAYSCALE_NON_PACKED(X, ...)				\
	X(VIDEO_PIX_FMT_GREY, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_Y10, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_Y12, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_Y14, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_Y16, __VA_ARGS__)

/**
 * Same as Y8 (8-bit luma-only) following the standard FOURCC naming,
 * or L8 in some graphics libraries.
 *
 * @code{.unparsed}
 *   0          1          2          3
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_GREY VIDEO_FOURCC('G', 'R', 'E', 'Y')

/**
 * Little endian, with the 6 most significant bits set to Zero.
 * @code{.unparsed}
 *   0                   1                   2                   3
 * | yyyyyyyy 000000Yy | yyyyyyyy 000000Yy | yyyyyyyy 000000Yy | yyyyyyyy 000000Yy | ...
 * | yyyyyyyy 000000Yy | yyyyyyyy 000000Yy | yyyyyyyy 000000Yy | yyyyyyyy 000000Yy | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_Y10 VIDEO_FOURCC('Y', '1', '0', ' ')

/**
 * Little endian, with the 4 most significant bits set to Zero.
 * @code{.unparsed}
 *   0                   1                   2                   3
 * | yyyyyyyy 0000Yyyy | yyyyyyyy 0000Yyyy | yyyyyyyy 0000Yyyy | yyyyyyyy 0000Yyyy | ...
 * | yyyyyyyy 0000Yyyy | yyyyyyyy 0000Yyyy | yyyyyyyy 0000Yyyy | yyyyyyyy 0000Yyyy | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_Y12 VIDEO_FOURCC('Y', '1', '2', ' ')

/**
 * Little endian, with the 2 most significant bits set to Zero.
 * @code{.unparsed}
 *   0                   1                   2                   3
 * | yyyyyyyy 00Yyyyyy | yyyyyyyy 00Yyyyyy | yyyyyyyy 00Yyyyyy | yyyyyyyy 00Yyyyyy | ...
 * | yyyyyyyy 00Yyyyyy | yyyyyyyy 00Yyyyyy | yyyyyyyy 00Yyyyyy | yyyyyyyy 00Yyyyyy | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_Y14 VIDEO_FOURCC('Y', '1', '4', ' ')

/**
 * Little endian.
 * @code{.unparsed}
 *   0                   1                   2                   3
 * | yyyyyyyy Yyyyyyyy | yyyyyyyy Yyyyyyyy | yyyyyyyy Yyyyyyyy | yyyyyyyy Yyyyyyyy | ...
 * | yyyyyyyy Yyyyyyyy | yyyyyyyy Yyyyyyyy | yyyyyyyy Yyyyyyyy | yyyyyyyy Yyyyyyyy | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_Y16 VIDEO_FOURCC('Y', '1', '6', ' ')

/**
 * @brief Repeat a macro for every grayscale padded format, passed as first parameter
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define VIDEO_FOREACH_GRAYSCALE_PADDED(X, ...)					\
	X(VIDEO_PIX_FMT_Y8P16, __VA_ARGS__)

/**
 * 8-bit luma-only split in two 4-bit blocks each zero-padded, little endian.
 *
 * @code{.unparsed}
 *   0                   1                   2                   3
 * | 0000yyyy 0000Yyyy | 0000yyyy 0000Yyyy | 0000yyyy 0000Yyyy | 0000yyyy 0000Yyyy | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_Y8P16 VIDEO_FOURCC('Y', '8', 'P', '2')

/**
 * @brief Repeat a macro for every grayscale MIPI-packed format, passed as first parameter
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define VIDEO_FOREACH_GRAYSCALE_MIPI_PACKED(X, ...)				\
	X(VIDEO_PIX_FMT_Y10P, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_Y12P, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_Y14P, __VA_ARGS__)

/**
 * @code{.unparsed}
 *   0          1          2          3          3 2 1 0
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | yyyyyyyy | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_Y10P VIDEO_FOURCC('Y', '1', '0', 'P')

/**
 * @code{.unparsed}
 *   0          1          1   0      2          3          3   2
 * | Yyyyyyyy | Yyyyyyyy | yyyyyyyy | Yyyyyyyy | Yyyyyyyy | yyyyyyyy | ...
 * | Yyyyyyyy | Yyyyyyyy | yyyyyyyy | Yyyyyyyy | Yyyyyyyy | yyyyyyyy | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_Y12P VIDEO_FOURCC('Y', '1', '2', 'P')

/**
 * @code{.unparsed}
 *   0          1          2          3          1 0      2   1    3     2
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | yyyyyyyy yyyyyyyy yyyyyyyy | ...
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | yyyyyyyy yyyyyyyy yyyyyyyy | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_Y14P VIDEO_FOURCC('Y', '1', '4', 'P')

/**
 * @}
 */

/**
 * @name RGB formats
 * Per-color (R, G, B) channels.
 * @{
 */

/**
 * @brief Repeat a macro for every RGB format, passed as first parameter
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define VIDEO_FOREACH_RGB(X, ...)						\
	VIDEO_FOREACH_RGB_PACKED(X, __VA_ARGS__)				\
	VIDEO_FOREACH_RGB_NON_PACKED(X, __VA_ARGS__)				\
	VIDEO_FOREACH_RGB_ALPHA(X, __VA_ARGS__)					\
	VIDEO_FOREACH_RGB_PADDED(X, __VA_ARGS__)

/**
 * @brief Repeat a macro for every RGB packed format, passed as first parameter
 *
 * A red, green, blue channel for every pixel, other than 8-bit per pixel.
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define VIDEO_FOREACH_RGB_PACKED(X, ...)					\
	X(VIDEO_PIX_FMT_RGB565X, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_RGB565, __VA_ARGS__)

/**
 * 5 red bits [15:11], 6 green bits [10:5], 5 blue bits [4:0].
 * This 16-bit integer is then packed in big endian format over two bytes:
 *
 * @code{.unparsed}
 *   15.....8 7......0
 * | RrrrrGgg gggBbbbb | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_RGB565X VIDEO_FOURCC('R', 'G', 'B', 'R')

/**
 * 5 red bits [15:11], 6 green bits [10:5], 5 blue bits [4:0].
 * This 16-bit integer is then packed in little endian format over two bytes:
 *
 * @code{.unparsed}
 *   7......0 15.....8
 * | gggBbbbb RrrrrGgg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_RGB565 VIDEO_FOURCC('R', 'G', 'B', 'P')

/**
 * @brief Repeat a macro for every RGB non-packed format, passed as first parameter
 *
 * A red, green, blue channel for every pixel, 8-bit per pixel.
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define VIDEO_FOREACH_RGB_NON_PACKED(X, ...)					\
	X(VIDEO_PIX_FMT_BGR24, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_RGB24, __VA_ARGS__)

/**
 * 24 bit RGB format with 8 bit per component
 *
 * @code{.unparsed}
 * | Bbbbbbbb Gggggggg Rggggggg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_BGR24 VIDEO_FOURCC('B', 'G', 'R', '3')

/**
 * 24 bit RGB format with 8 bit per component
 *
 * @code{.unparsed}
 * | Rggggggg Gggggggg Bbbbbbbb | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_RGB24 VIDEO_FOURCC('R', 'G', 'B', '3')

/**
 * @brief Repeat a macro for every RGB alpha format, passed as first parameter
 *
 * A red, green, blue, alpha channel for every pixel.
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define VIDEO_FOREACH_RGB_ALPHA(X, ...)						\
	X(VIDEO_PIX_FMT_ARGB32, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_ABGR32, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_RGBA32, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_BGRA32, __VA_ARGS__)

/**
 * The first byte is alpha (A) for each pixel.
 *
 * @code{.unparsed}
 * | Aaaaaaaa Rrrrrrrr Gggggggg Bbbbbbbb | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_ARGB32 VIDEO_FOURCC('B', 'A', '2', '4')

/**
 * The last byte is alpha (A) for each pixel.
 * *
 * @warning Linux calls this format ABGR32 due to a historical typo
 *
 * @code{.unparsed}
 * | Bbbbbbbb Gggggggg Rrrrrrrr Aaaaaaaa | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_BGRA32 VIDEO_FOURCC('A', 'R', '2', '4')

/**
 * The last byte is alpha (A) for each pixel.
 *
 * @code{.unparsed}
 * | Rrrrrrrr Gggggggg Bbbbbbbb Aaaaaaaa | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_RGBA32 VIDEO_FOURCC('A', 'B', '2', '4')

/**
 * The first byte is alpha (A) for each pixel.
 *
 * @warning Linux calls this format BGRA32 due to a historical typo
 *
 * @code{.unparsed}
 * | Aaaaaaaa Bbbbbbbb Gggggggg Rrrrrrrr | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_ABGR32 VIDEO_FOURCC('R', 'A', '2', '4')

/**
 * @brief Repeat a macro for every RGB padded format, passed as first parameter
 *
 * A red, green, blue channel for every pixel.
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define VIDEO_FOREACH_RGB_PADDED(X, ...)					\
	X(VIDEO_PIX_FMT_XRGB32, __VA_ARGS__)

/**
 * The first byte is empty (X) for each pixel.
 *
 * @code{.unparsed}
 * | Xxxxxxxx Rrrrrrrr Gggggggg Bbbbbbbb | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_XRGB32 VIDEO_FOURCC('B', 'X', '2', '4')

/**
 * The first byte is empty (X) for each pixel.
 *
 * @warning Linux calls this format BGRX32 due to a historical typo
 *
 * @code{.unparsed}
 * | Xxxxxxxx Bbbbbbbb Gggggggg Rrrrrrrr | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_XBGR32 VIDEO_FOURCC('R', 'X', '2', '4')

/**
 * The last byte is empty (X) for each pixel.
 *
 * @warning Linux calls this format XBGR32 due to a historical typo
 *
 * @code{.unparsed}
 * | Bbbbbbbb Gggggggg Rrrrrrrr Xxxxxxxx | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_BGRX32 VIDEO_FOURCC('X', 'R', '2', '4')

/**
 * The last byte is empty (X) for each pixel.
 *
 * @code{.unparsed}
 * | Rrrrrrrr Gggggggg Bbbbbbbb Xxxxxxxx | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_RGBX32 VIDEO_FOURCC('X', 'B', '2', '4')

/**
 * @}
 */

/**
 * @name YUV formats
 * Luminance (Y) and chrominance (U, V) channels.
 * @{
 */

/**
 * @brief Repeat a macro for every YUV format, passed as first parameter
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define VIDEO_FOREACH_YUV(X, ...)						\
	VIDEO_FOREACH_YUV_FULL_PLANAR(X, __VA_ARGS__)				\
	VIDEO_FOREACH_YUV_NON_PLANAR(X, __VA_ARGS__)				\
	VIDEO_FOREACH_YUV_SEMI_PLANAR(X, __VA_ARGS__)

/**
 * @brief Repeat a macro for every YUV Non-Planar format, passed as first parameter
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define VIDEO_FOREACH_YUV_NON_PLANAR(X, ...)					\
	X(VIDEO_PIX_FMT_YUYV, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_YVYU, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_VYUY, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_UYVY, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_YUV24, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_XYUV32, __VA_ARGS__)

/**
 * There is either a missing channel per pixel, U or V.
 * The value is to be averaged over 2 pixels to get the value of individual pixel.
 *
 * @code{.unparsed}
 * | Yyyyyyyy Uuuuuuuu | Yyyyyyyy Vvvvvvvv | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_YUYV VIDEO_FOURCC('Y', 'U', 'Y', 'V')

/**
 * @code{.unparsed}
 * | Yyyyyyyy Vvvvvvvv | Yyyyyyyy Uuuuuuuu | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_YVYU VIDEO_FOURCC('Y', 'V', 'Y', 'U')

/**
 * @code{.unparsed}
 * | Vvvvvvvv Yyyyyyyy | Uuuuuuuu Yyyyyyyy | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_VYUY VIDEO_FOURCC('V', 'Y', 'U', 'Y')

/**
 * @code{.unparsed}
 * | Uuuuuuuu Yyyyyyyy | Vvvvvvvv Yyyyyyyy | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_UYVY VIDEO_FOURCC('U', 'Y', 'V', 'Y')

/**
 * The first byte is empty (X) for each pixel.
 *
 * @code{.unparsed}
 * | Xxxxxxxx Yyyyyyyy Uuuuuuuu Vvvvvvvv | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_XYUV32 VIDEO_FOURCC('X', 'Y', 'U', 'V')

/**
 * 24 bit YUV format with 8 bit per component
 *
 * @code{.unparsed}
 * | Yyyyyyyy Uuuuuuuu Vvvvvvvv | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_YUV24 VIDEO_FOURCC('Y', 'U', 'V', '3')

/**
 * @brief Repeat a macro for every YUV Semi-Planar format, passed as first parameter
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define VIDEO_FOREACH_YUV_SEMI_PLANAR(X, ...)					\
	X(VIDEO_PIX_FMT_NV12, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_NV21, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_NV16, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_NV61, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_NV24, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_NV42, __VA_ARGS__)

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
#define VIDEO_PIX_FMT_NV12 VIDEO_FOURCC('N', 'V', '1', '2')

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
#define VIDEO_PIX_FMT_NV21 VIDEO_FOURCC('N', 'V', '2', '1')

/**
 * Chroma (U/V) are subsampled horizontaly
 *
 * @code{.unparsed}
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | ...
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | ...
 * | ... |
 * | Uuuuuuuu   Vvvvvvvv | Uuuuuuuu   Vvvvvvvv | ...
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
 *  U0/1      V0/1      U2/3      V2/3      ...
 *  U6/7      V6/7      U8/9      V8/9      ...
 *  ...
 * @endcode
 */
#define VIDEO_PIX_FMT_NV16 VIDEO_FOURCC('N', 'V', '1', '6')

/**
 * Chroma (U/V) are subsampled horizontaly
 *
 * @code{.unparsed}
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | ...
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | ...
 * | ... |
 * | Vvvvvvvv   Uuuuuuuu | Vvvvvvvv   Uuuuuuuu | ...
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
 *  V0/1      U0/1      V2/3      U2/3      ...
 *  V6/7      U6/7      V8/9      U8/9      ...
 *  ...
 * @endcode
 */

#define VIDEO_PIX_FMT_NV61 VIDEO_FOURCC('N', 'V', '6', '1')

/**
 * Chroma (U/V) are not subsampled
 *
 * @code{.unparsed}
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy |
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy |
 * | ... |
 * | Uuuuuuuu   Vvvvvvvv | Uuuuuuuu   Vvvvvvvv | Uuuuuuuu   Vvvvvvvv | Uuuuuuuu   Vvvvvvvv |
 * | Uuuuuuuu   Vvvvvvvv | Uuuuuuuu   Vvvvvvvv | Uuuuuuuu   Vvvvvvvv | Uuuuuuuu   Vvvvvvvv |
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
 *  U0        V0        U1        V1        U2        V2        U3        V3        ...
 *  U6        V6        U7        V7        U8        V8        U9        V9        ...
 *  ...
 * @endcode
 */
#define VIDEO_PIX_FMT_NV24 VIDEO_FOURCC('N', 'V', '2', '4')

/**
 * Chroma (U/V) are not subsampled
 *
 * @code{.unparsed}
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy |
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy |
 * | ... |
 * | Vvvvvvvv   Uuuuuuuu | Vvvvvvvv   Uuuuuuuu | Vvvvvvvv   Uuuuuuuu | Vvvvvvvv   Uuuuuuuu |
 * | Vvvvvvvv   Uuuuuuuu | Vvvvvvvv   Uuuuuuuu | Vvvvvvvv   Uuuuuuuu | Vvvvvvvv   Uuuuuuuu |
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
 *  V0        U0        V1        U1        V2        U2        V3        U3        ...
 *  V6        U6        V7        U7        V8        U8        V9        U9        ...
 *  ...
 * @endcode
 */
#define VIDEO_PIX_FMT_NV42 VIDEO_FOURCC('N', 'V', '4', '2')

/**
 * @brief Repeat a macro for every YUV Full-Planar format, passed as first parameter
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define VIDEO_FOREACH_YUV_FULL_PLANAR(X, ...)					\
	X(VIDEO_PIX_FMT_YUV420, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_YVU420, __VA_ARGS__)

/**
 * Chroma (U/V) are subsampled horizontaly and vertically
 *
 * @code{.unparsed}
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy |
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy |
 * | ... |
 * | Uuuuuuuu | Uuuuuuuu |
 * | ... |
 * | Vvvvvvvv | Vvvvvvvv |
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
 *  U0/1/6/7      U2/3/8/9      ...
 *  ...
 *
 *  V0/1/6/7      V2/3/8/9      ...
 *  ...
 * @endcode
 */
#define VIDEO_PIX_FMT_YUV420 VIDEO_FOURCC('Y', 'U', '1', '2')

/**
 * Chroma (U/V) are subsampled horizontaly and vertically
 *
 * @code{.unparsed}
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy |
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy |
 * | ... |
 * | Vvvvvvvv | Vvvvvvvv |
 * | ... |
 * | Uuuuuuuu | Uuuuuuuu |
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
 *  V0/1/6/7      V2/3/8/9      ...
 *  ...
 *
 *  U0/1/6/7      U2/3/8/9      ...
 *  ...
 * @endcode
 */
#define VIDEO_PIX_FMT_YVU420 VIDEO_FOURCC('Y', 'V', '1', '2')

/**
 * @}
 */

/**
 * @name Compressed formats
 * @{
 */

/**
 * @brief Repeat a macro for every compressed format, passed as first parameter
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define VIDEO_FOREACH_COMPRESSED(X, ...)					\
	X(VIDEO_PIX_FMT_JPEG, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_H264, __VA_ARGS__)					\
	X(VIDEO_PIX_FMT_H264_NO_SC, __VA_ARGS__)				\
	X(VIDEO_PIX_FMT_PNG, __VA_ARGS__)

/**
 * Both JPEG (single frame) and Motion-JPEG (MJPEG, multiple JPEG frames concatenated)
 */
#define VIDEO_PIX_FMT_JPEG VIDEO_FOURCC('J', 'P', 'E', 'G')

/**
 * H264 with start code
 */
#define VIDEO_PIX_FMT_H264 VIDEO_FOURCC('H', '2', '6', '4')

/**
 * H264 without start code
 */
#define VIDEO_PIX_FMT_H264_NO_SC VIDEO_FOURCC('A', 'V', 'C', '1')

/**
 * PNG
 */
#define VIDEO_PIX_FMT_PNG VIDEO_FOURCC('P', 'N', 'G', ' ')

/**
 * @}
 */

/** @cond INTERNAL_HIDDEN */
#define _VIDEO_FMT_OR_EQ(pixfmt_a, pixfmt_b) || ((pixfmt_a) == (pixfmt_b))
/** @endcond */

/**
 * @brief Test if a fourcc is a grayscale format
 *
 * @param pixfmt FourCC of the pixel format to test
 * @return Whether the format is known to match this category
 */
#define VIDEO_FMT_IS_GRAYSCALE(pixfmt)						\
	(0 VIDEO_FOREACH_GRAYSCALE(_VIDEO_FMT_OR_EQ, pixfmt))

/**
 * @brief Test if a fourcc is a Bayer format
 *
 * @param pixfmt FourCC of the pixel format to test
 * @return Whether the format is known to match this category
 */
#define VIDEO_FMT_IS_BAYER(pixfmt)						\
	(0 VIDEO_FOREACH_BAYER(_VIDEO_FMT_OR_EQ, pixfmt))

/**
 * @brief Test if a fourcc is an RGB format (bayer excluded)
 *
 * @param pixfmt FourCC of the pixel format to test
 * @return Whether the format is known to match this category
 */
#define VIDEO_FMT_IS_RGB(pixfmt)						\
	(0 VIDEO_FOREACH_RGB(_VIDEO_FMT_OR_EQ, pixfmt))

/**
 * @brief Test if a fourcc is an YUV format
 *
 * @param pixfmt FourCC of the pixel format to test
 * @return Whether the format is known to match this category
 */
#define VIDEO_FMT_IS_YUV(pixfmt)						\
	(0 VIDEO_FOREACH_YUV(_VIDEO_FMT_OR_EQ, pixfmt))

/**
 * @brief Test if a fourcc is any of the MIPI-packed formats
 *
 * @param pixfmt FourCC of the pixel format to test
 * @return Whether the format is known to match this category
 */
#define VIDEO_FMT_IS_MIPI_PACKED(pixfmt)					\
	(0 VIDEO_FOREACH_BAYER_MIPI_PACKED(_VIDEO_FMT_OR_EQ, pixfmt)		\
	   VIDEO_FOREACH_GRAYSCALE_MIPI_PACKED(_VIDEO_FMT_OR_EQ, pixfmt))

/**
 * @brief Test if a fourcc is any of the MIPI-packed formats
 *
 * @param pixfmt FourCC of the pixel format to test
 * @return Whether the format is known to match this category
 */
#define VIDEO_FMT_IS_PADDED(pixfmt)						\
	(0 VIDEO_FOREACH_BAYER_PADDED(_VIDEO_FMT_OR_EQ, pixfmt)			\
	   VIDEO_FOREACH_GRAYSCALE_PADDED(_VIDEO_FMT_OR_EQ, pixfmt))

/**
 * @brief Test if a fourcc is any of the semi-planar formats
 *
 * @param pixfmt FourCC of the pixel format to test
 * @return Whether the format is known to match this category
 */
#define VIDEO_FMT_IS_SEMI_PLANAR(pixfmt)					\
	(0 VIDEO_FOREACH_YUV_SEMI_PLANAR(_VIDEO_FMT_OR_EQ, pixfmt))

/**
 * @brief Test if a fourcc is any of the full-planar formats
 *
 * @param pixfmt FourCC of the pixel format to test
 * @return Whether the format is known to match this category
 */
#define VIDEO_FMT_IS_FULL_PLANAR(pixfmt)					\
	(0 VIDEO_FOREACH_YUV_FULL_PLANAR(_VIDEO_FMT_OR_EQ, pixfmt))

/**
 * @brief Get number of bits per pixel of a pixel format
 *
 * @param pixfmt FourCC pixel format value (@ref video_pixel_formats).
 *
 * @retval 0 if the format is unhandled or if it is variable number of bits
 * @retval >0 bit size of one pixel for this format
 */
static inline unsigned int video_bits_per_pixel(uint32_t pixfmt)
{
	switch (pixfmt) {
	case VIDEO_PIX_FMT_SBGGR8:
	case VIDEO_PIX_FMT_SGBRG8:
	case VIDEO_PIX_FMT_SGRBG8:
	case VIDEO_PIX_FMT_SRGGB8:
	case VIDEO_PIX_FMT_GREY:
		return 8;
	case VIDEO_PIX_FMT_SBGGR10P:
	case VIDEO_PIX_FMT_SGBRG10P:
	case VIDEO_PIX_FMT_SGRBG10P:
	case VIDEO_PIX_FMT_SRGGB10P:
	case VIDEO_PIX_FMT_Y10P:
		return 10;
	case VIDEO_PIX_FMT_SBGGR12P:
	case VIDEO_PIX_FMT_SGBRG12P:
	case VIDEO_PIX_FMT_SGRBG12P:
	case VIDEO_PIX_FMT_SRGGB12P:
	case VIDEO_PIX_FMT_Y12P:
	case VIDEO_PIX_FMT_NV12:
	case VIDEO_PIX_FMT_NV21:
	case VIDEO_PIX_FMT_YUV420:
	case VIDEO_PIX_FMT_YVU420:
		return 12;
	case VIDEO_PIX_FMT_SBGGR14P:
	case VIDEO_PIX_FMT_SGBRG14P:
	case VIDEO_PIX_FMT_SGRBG14P:
	case VIDEO_PIX_FMT_SRGGB14P:
	case VIDEO_PIX_FMT_Y14P:
		return 14;
	case VIDEO_PIX_FMT_RGB565:
	case VIDEO_PIX_FMT_YUYV:
	case VIDEO_PIX_FMT_YVYU:
	case VIDEO_PIX_FMT_UYVY:
	case VIDEO_PIX_FMT_VYUY:
	case VIDEO_PIX_FMT_SBGGR8P16:
	case VIDEO_PIX_FMT_SGBRG8P16:
	case VIDEO_PIX_FMT_SGRBG8P16:
	case VIDEO_PIX_FMT_SRGGB8P16:
	case VIDEO_PIX_FMT_SBGGR10:
	case VIDEO_PIX_FMT_SGBRG10:
	case VIDEO_PIX_FMT_SGRBG10:
	case VIDEO_PIX_FMT_SRGGB10:
	case VIDEO_PIX_FMT_SBGGR12:
	case VIDEO_PIX_FMT_SGBRG12:
	case VIDEO_PIX_FMT_SGRBG12:
	case VIDEO_PIX_FMT_SRGGB12:
	case VIDEO_PIX_FMT_SBGGR14:
	case VIDEO_PIX_FMT_SGBRG14:
	case VIDEO_PIX_FMT_SGRBG14:
	case VIDEO_PIX_FMT_SRGGB14:
	case VIDEO_PIX_FMT_SBGGR16:
	case VIDEO_PIX_FMT_SGBRG16:
	case VIDEO_PIX_FMT_SGRBG16:
	case VIDEO_PIX_FMT_SRGGB16:
	case VIDEO_PIX_FMT_Y8P16:
	case VIDEO_PIX_FMT_Y10:
	case VIDEO_PIX_FMT_Y12:
	case VIDEO_PIX_FMT_Y14:
	case VIDEO_PIX_FMT_Y16:
	case VIDEO_PIX_FMT_NV16:
	case VIDEO_PIX_FMT_NV61:
		return 16;
	case VIDEO_PIX_FMT_BGR24:
	case VIDEO_PIX_FMT_RGB24:
	case VIDEO_PIX_FMT_NV24:
	case VIDEO_PIX_FMT_NV42:
	case VIDEO_PIX_FMT_YUV24:
		return 24;
	case VIDEO_PIX_FMT_XYUV32:
	case VIDEO_PIX_FMT_ARGB32:
	case VIDEO_PIX_FMT_ABGR32:
	case VIDEO_PIX_FMT_RGBA32:
	case VIDEO_PIX_FMT_BGRA32:
	case VIDEO_PIX_FMT_XRGB32:
	case VIDEO_PIX_FMT_XBGR32:
	case VIDEO_PIX_FMT_RGBX32:
	case VIDEO_PIX_FMT_BGRX32:
		return 32;
	default:
		/* Variable number of bits per pixel or unknown format */
		return 0;
	}
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_VIDEO_FORMATS_H */
