/*
 * Copyright (c) 2019 Linaro Limited.
 * Copyright 2025 NXP
 * Copyright (c) 2025 STMicroelectronics
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup pixel_formats
 * @brief Pixel format identifiers shared by devicetree and C.
 */

#ifndef ZEPHYR_INCLUDE_MEDIA_FORMATS_H_
#define ZEPHYR_INCLUDE_MEDIA_FORMATS_H_

/**
 * @addtogroup pixel_formats
 * @{
 */

/**
 * @brief Four-character-code uniquely identifying the pixel format
 *
 * @param a First byte of the code, stored in bits 0 through 7.
 * @param b Second byte of the code, stored in bits 8 through 15.
 * @param c Third byte of the code, stored in bits 16 through 23.
 * @param d Fourth byte of the code, stored in bits 24 through 31.
 * @return Unsigned FourCC value, usable in C, devicetree and preprocessor expressions.
 */
#define PIX_FMT_FOURCC(a, b, c, d) \
	(((a) & 0xffU) | (((b) & 0xffU) << 8) | (((c) & 0xffU) << 16) | (((d) & 0xffU) << 24))

/**
 * 8-bit bayer format, split in two 4-bit blocks each zero-padded, little endian.
 *
 * @code{.unparsed}
 *   0                   1                   2                   3
 * | 0000bbbb 0000Bbbb | 0000gggg 0000Gggg | 0000gggg 0000Gggg | 0000rrrr 0000Rrrr | ...
 * @endcode
 */
#define PIX_FMT_SBGGR8P16 PIX_FMT_FOURCC('p', 'B', '8', '2')

/**
 * 8-bit bayer format, split in two 4-bit blocks each zero-padded, little endian.
 *
 * @code{.unparsed}
 *   0                   1                   2                   3
 * | 0000gggg 0000Gggg | 0000bbbb 0000Bbbb | 0000rrrr 0000Rrrr | 0000gggg 0000Gggg | ...
 * @endcode
 */
#define PIX_FMT_SGBRG8P16 PIX_FMT_FOURCC('p', 'G', '8', '2')

/**
 * 8-bit bayer format, split in two 4-bit blocks each zero-padded, little endian.
 *
 * @code{.unparsed}
 *   0                   1                   2                   3
 * | 0000gggg 0000Gggg | 0000rrrr 0000Rrrr | 0000bbbb 0000Bbbb | 0000gggg 0000Gggg | ...
 * @endcode
 */
#define PIX_FMT_SGRBG8P16 PIX_FMT_FOURCC('p', 'g', '8', '2')

/**
 * 8-bit bayer format, split in two 4-bit blocks each zero-padded, little endian.
 *
 * @code{.unparsed}
 *   0                   1                   2                   3
 * | 0000rrrr 0000Rrrr | 0000gggg 0000Gggg | 0000gggg 0000Gggg | 0000bbbb 0000Bbbb | ...
 * @endcode
 */
#define PIX_FMT_SRGGB8P16 PIX_FMT_FOURCC('p', 'R', '8', '2')

/**
 * @code{.unparsed}
 *   0          1          2          3          3 2 1 0
 * | Bbbbbbbb | Gggggggg | Bbbbbbbb | Gggggggg | ggbbggbb | ...
 * | Gggggggg | Rrrrrrrr | Gggggggg | Rrrrrrrr | rrggrrgg | ...
 * @endcode
 */
#define PIX_FMT_SBGGR10P PIX_FMT_FOURCC('p', 'B', 'A', 'A')

/**
 * @code{.unparsed}
 *   0          1          2          3          3 2 1 0
 * | Gggggggg | Bbbbbbbb | Gggggggg | Bbbbbbbb | bbggbbgg | ...
 * | Rrrrrrrr | Gggggggg | Rrrrrrrr | Gggggggg | ggrrggrr | ...
 * @endcode
 */
#define PIX_FMT_SGBRG10P PIX_FMT_FOURCC('p', 'G', 'A', 'A')

/**
 * @code{.unparsed}
 *   0          1          2          3          3 2 1 0
 * | Gggggggg | Rrrrrrrr | Gggggggg | Rrrrrrrr | rrggrrgg | ...
 * | Bbbbbbbb | Gggggggg | Bbbbbbbb | Gggggggg | ggbbggbb | ...
 * @endcode
 */
#define PIX_FMT_SGRBG10P PIX_FMT_FOURCC('p', 'g', 'A', 'A')

/**
 * @code{.unparsed}
 *   0          1          2          3          3 2 1 0
 * | Rrrrrrrr | Gggggggg | Rrrrrrrr | Gggggggg | ggrrggrr | ...
 * | Gggggggg | Bbbbbbbb | Gggggggg | Bbbbbbbb | bbggbbgg | ...
 * @endcode
 */
#define PIX_FMT_SRGGB10P PIX_FMT_FOURCC('p', 'R', 'A', 'A')

/**
 * @code{.unparsed}
 *   0          1          1   0      2          3          3   2
 * | Bbbbbbbb | Gggggggg | ggggbbbb | Bbbbbbbb | Gggggggg | ggggbbbb | ...
 * | Gggggggg | Rrrrrrrr | rrrrgggg | Gggggggg | Rrrrrrrr | rrrrgggg | ...
 * @endcode
 */
#define PIX_FMT_SBGGR12P PIX_FMT_FOURCC('p', 'B', 'C', 'C')

/**
 * @code{.unparsed}
 *   0          1          1   0      2          3          3   2
 * | Gggggggg | Bbbbbbbb | bbbbgggg | Gggggggg | Bbbbbbbb | bbbbgggg | ...
 * | Rrrrrrrr | Gggggggg | ggggrrrr | Rrrrrrrr | Gggggggg | ggggrrrr | ...
 * @endcode
 */
#define PIX_FMT_SGBRG12P PIX_FMT_FOURCC('p', 'G', 'C', 'C')

/**
 * @code{.unparsed}
 *   0          1          1   0      2          3          3   2
 * | Gggggggg | Rrrrrrrr | rrrrgggg | Gggggggg | Rrrrrrrr | rrrrgggg | ...
 * | Bbbbbbbb | Gggggggg | ggggbbbb | Bbbbbbbb | Gggggggg | ggggbbbb | ...
 * @endcode
 */
#define PIX_FMT_SGRBG12P PIX_FMT_FOURCC('p', 'g', 'C', 'C')

/**
 * @code{.unparsed}
 *   0          1          1   0      2          3          3   2
 * | Rrrrrrrr | Gggggggg | ggggrrrr | Rrrrrrrr | Gggggggg | ggggrrrr | ...
 * | Gggggggg | Bbbbbbbb | bbbbgggg | Gggggggg | Bbbbbbbb | bbbbgggg | ...
 * @endcode
 */
#define PIX_FMT_SRGGB12P PIX_FMT_FOURCC('p', 'R', 'C', 'C')

/**
 * @code{.unparsed}
 *   0          1          2          3          1 0      2   1    3     2
 * | Bbbbbbbb | Gggggggg | Bbbbbbbb | Gggggggg | ggbbbbbb bbbbgggg ggggggbb | ...
 * | Gggggggg | Rrrrrrrr | Gggggggg | Rrrrrrrr | rrgggggg ggggrrrr rrrrrrgg | ...
 * @endcode
 */
#define PIX_FMT_SBGGR14P PIX_FMT_FOURCC('p', 'B', 'E', 'E')

/**
 * @code{.unparsed}
 *   0          1          2          3          1 0      2   1    3     2
 * | Gggggggg | Bbbbbbbb | Gggggggg | Bbbbbbbb | bbgggggg ggggbbbb bbbbbbgg | ...
 * | Rrrrrrrr | Gggggggg | Rrrrrrrr | Gggggggg | ggrrrrrr rrrrgggg ggggggrr | ...
 * @endcode
 */
#define PIX_FMT_SGBRG14P PIX_FMT_FOURCC('p', 'G', 'E', 'E')

/**
 * @code{.unparsed}
 *   0          1          2          3          1 0      2   1    3     2
 * | Gggggggg | Rrrrrrrr | Gggggggg | Rrrrrrrr | rrgggggg ggggrrrr rrrrrrgg | ...
 * | Bbbbbbbb | Gggggggg | Bbbbbbbb | Gggggggg | ggbbbbbb bbbbgggg ggggggbb | ...
 * @endcode
 */
#define PIX_FMT_SGRBG14P PIX_FMT_FOURCC('p', 'g', 'E', 'E')

/**
 * @code{.unparsed}
 *   0          1          2          3          1 0      2   1    3     2
 * | Rrrrrrrr | Gggggggg | Rrrrrrrr | Gggggggg | ggrrrrrr rrrrgggg ggggggrr | ...
 * | Gggggggg | Bbbbbbbb | Gggggggg | Bbbbbbbb | bbgggggg ggggbbbb bbbbbbgg | ...
 * @endcode
 */
#define PIX_FMT_SRGGB14P PIX_FMT_FOURCC('p', 'R', 'E', 'E')

/**
 * @code{.unparsed}
 *   0          1          2          3
 * | Bbbbbbbb | Gggggggg | Bbbbbbbb | Gggggggg | ...
 * | Gggggggg | Rrrrrrrr | Gggggggg | Rrrrrrrr | ...
 * @endcode
 */
#define PIX_FMT_SBGGR8 PIX_FMT_FOURCC('B', 'A', '8', '1')

/**
 * @code{.unparsed}
 *   0          1          2          3
 * | Gggggggg | Bbbbbbbb | Gggggggg | Bbbbbbbb | ...
 * | Rrrrrrrr | Gggggggg | Rrrrrrrr | Gggggggg | ...
 * @endcode
 */
#define PIX_FMT_SGBRG8 PIX_FMT_FOURCC('G', 'B', 'R', 'G')

/**
 * @code{.unparsed}
 *   0          1          2          3
 * | Gggggggg | Rrrrrrrr | Gggggggg | Rrrrrrrr | ...
 * | Bbbbbbbb | Gggggggg | Bbbbbbbb | Gggggggg | ...
 * @endcode
 */
#define PIX_FMT_SGRBG8 PIX_FMT_FOURCC('G', 'R', 'B', 'G')

/**
 * @code{.unparsed}
 *   0          1          2          3
 * | Rrrrrrrr | Gggggggg | Rrrrrrrr | Gggggggg | ...
 * | Gggggggg | Bbbbbbbb | Gggggggg | Bbbbbbbb | ...
 * @endcode
 */
#define PIX_FMT_SRGGB8 PIX_FMT_FOURCC('R', 'G', 'G', 'B')

/**
 * @code{.unparsed}
 * | bbbbbbbb 000000Bb | gggggggg 000000Gg | bbbbbbbb 000000Bb | gggggggg 000000Gg | ...
 * | gggggggg 000000Gg | rrrrrrrr 000000Rr | gggggggg 000000Gg | rrrrrrrr 000000Rr | ...
 * @endcode
 */
#define PIX_FMT_SBGGR10 PIX_FMT_FOURCC('B', 'G', '1', '0')

/**
 * @code{.unparsed}
 * | gggggggg 000000Gg | bbbbbbbb 000000Bb | gggggggg 000000Gg | bbbbbbbb 000000Bb | ...
 * | rrrrrrrr 000000Rr | gggggggg 000000Gg | rrrrrrrr 000000Rr | gggggggg 000000Gg | ...
 * @endcode
 */
#define PIX_FMT_SGBRG10 PIX_FMT_FOURCC('G', 'B', '1', '0')

/**
 * @code{.unparsed}
 * | gggggggg 000000Gg | rrrrrrrr 000000Rr | gggggggg 000000Gg | rrrrrrrr 000000Rr | ...
 * | bbbbbbbb 000000Bb | gggggggg 000000Gg | bbbbbbbb 000000Bb | gggggggg 000000Gg | ...
 * @endcode
 */
#define PIX_FMT_SGRBG10 PIX_FMT_FOURCC('B', 'A', '1', '0')

/**
 * @code{.unparsed}
 * | rrrrrrrr 000000Rr | gggggggg 000000Gg | rrrrrrrr 000000Rr | gggggggg 000000Gg | ...
 * | gggggggg 000000Gg | bbbbbbbb 000000Bb | gggggggg 000000Gg | bbbbbbbb 000000Bb | ...
 * @endcode
 */
#define PIX_FMT_SRGGB10 PIX_FMT_FOURCC('R', 'G', '1', '0')

/**
 * @code{.unparsed}
 * | bbbbbbbb 0000Bbbb | gggggggg 0000Gggg | bbbbbbbb 0000Bbbb | gggggggg 0000Gggg | ...
 * | gggggggg 0000Gggg | rrrrrrrr 0000Rrrr | gggggggg 0000Gggg | rrrrrrrr 0000Rrrr | ...
 * @endcode
 */
#define PIX_FMT_SBGGR12 PIX_FMT_FOURCC('B', 'G', '1', '2')

/**
 * @code{.unparsed}
 * | gggggggg 0000Gggg | bbbbbbbb 0000Bbbb | gggggggg 0000Gggg | bbbbbbbb 0000Bbbb | ...
 * | rrrrrrrr 0000Rrrr | gggggggg 0000Gggg | rrrrrrrr 0000Rrrr | gggggggg 0000Gggg | ...
 * @endcode
 */
#define PIX_FMT_SGBRG12 PIX_FMT_FOURCC('G', 'B', '1', '2')

/**
 * @code{.unparsed}
 * | gggggggg 0000Gggg | rrrrrrrr 0000Rrrr | gggggggg 0000Gggg | rrrrrrrr 0000Rrrr | ...
 * | bbbbbbbb 0000Bbbb | gggggggg 0000Gggg | bbbbbbbb 0000Bbbb | gggggggg 0000Gggg | ...
 * @endcode
 */
#define PIX_FMT_SGRBG12 PIX_FMT_FOURCC('B', 'A', '1', '2')

/**
 * @code{.unparsed}
 * | rrrrrrrr 0000Rrrr | gggggggg 0000Gggg | rrrrrrrr 0000Rrrr | gggggggg 0000Gggg | ...
 * | gggggggg 0000Gggg | bbbbbbbb 0000Bbbb | gggggggg 0000Gggg | bbbbbbbb 0000Bbbb | ...
 * @endcode
 */
#define PIX_FMT_SRGGB12 PIX_FMT_FOURCC('R', 'G', '1', '2')

/**
 * @code{.unparsed}
 * | bbbbbbbb 00Bbbbbb | gggggggg 00Gggggg | bbbbbbbb 00Bbbbbb | gggggggg 00Gggggg | ...
 * | gggggggg 00Gggggg | rrrrrrrr 00Rrrrrr | gggggggg 00Gggggg | rrrrrrrr 00Rrrrrr | ...
 * @endcode
 */
#define PIX_FMT_SBGGR14 PIX_FMT_FOURCC('B', 'G', '1', '4')

/**
 * @code{.unparsed}
 * | gggggggg 00Gggggg | bbbbbbbb 00Bbbbbb | gggggggg 00Gggggg | bbbbbbbb 00Bbbbbb | ...
 * | rrrrrrrr 00Rrrrrr | gggggggg 00Gggggg | rrrrrrrr 00Rrrrrr | gggggggg 00Gggggg | ...
 * @endcode
 */
#define PIX_FMT_SGBRG14 PIX_FMT_FOURCC('G', 'B', '1', '4')

/**
 * @code{.unparsed}
 * | gggggggg 00Gggggg | rrrrrrrr 00Rrrrrr | gggggggg 00Gggggg | rrrrrrrr 00Rrrrrr | ...
 * | bbbbbbbb 00Bbbbbb | gggggggg 00Gggggg | bbbbbbbb 00Bbbbbb | gggggggg 00Gggggg | ...
 * @endcode
 */
#define PIX_FMT_SGRBG14 PIX_FMT_FOURCC('G', 'R', '1', '4')

/**
 * @code{.unparsed}
 * | rrrrrrrr 00Rrrrrr | gggggggg 00Gggggg | rrrrrrrr 00Rrrrrr | gggggggg 00Gggggg | ...
 * | gggggggg 00Gggggg | bbbbbbbb 00Bbbbbb | gggggggg 00Gggggg | bbbbbbbb 00Bbbbbb | ...
 * @endcode
 */
#define PIX_FMT_SRGGB14 PIX_FMT_FOURCC('R', 'G', '1', '4')

/**
 * @code{.unparsed}
 * | bbbbbbbb Bbbbbbbb | gggggggg Gggggggg | bbbbbbbb Bbbbbbbb | gggggggg Gggggggg | ...
 * | gggggggg Gggggggg | rrrrrrrr Rrrrrrrr | gggggggg Gggggggg | rrrrrrrr Rrrrrrrr | ...
 * @endcode
 */
#define PIX_FMT_SBGGR16 PIX_FMT_FOURCC('B', 'Y', 'R', '2')

/**
 * @code{.unparsed}
 * | gggggggg Gggggggg | bbbbbbbb Bbbbbbbb | gggggggg Gggggggg | bbbbbbbb Bbbbbbbb | ...
 * | rrrrrrrr Rrrrrrrr | gggggggg Gggggggg | rrrrrrrr Rrrrrrrr | gggggggg Gggggggg | ...
 * @endcode
 */
#define PIX_FMT_SGBRG16 PIX_FMT_FOURCC('G', 'B', '1', '6')

/**
 * @code{.unparsed}
 * | gggggggg Gggggggg | rrrrrrrr Rrrrrrrr | gggggggg Gggggggg | rrrrrrrr Rrrrrrrr | ...
 * | bbbbbbbb Bbbbbbbb | gggggggg Gggggggg | bbbbbbbb Bbbbbbbb | gggggggg Gggggggg | ...
 * @endcode
 */
#define PIX_FMT_SGRBG16 PIX_FMT_FOURCC('G', 'R', '1', '6')

/**
 * @code{.unparsed}
 * | rrrrrrrr Rrrrrrrr | gggggggg Gggggggg | rrrrrrrr Rrrrrrrr | gggggggg Gggggggg | ...
 * | gggggggg Gggggggg | bbbbbbbb Bbbbbbbb | gggggggg Gggggggg | bbbbbbbb Bbbbbbbb | ...
 * @endcode
 */
#define PIX_FMT_SRGGB16 PIX_FMT_FOURCC('R', 'G', '1', '6')

/**
 * Same as Y8 (8-bit luma-only) following the standard FOURCC naming,
 * or L8 in some graphics libraries.
 *
 * @code{.unparsed}
 *   0          1          2          3
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | ...
 * @endcode
 */
#define PIX_FMT_GREY PIX_FMT_FOURCC('G', 'R', 'E', 'Y')

/**
 * 16-bit Greyscale format with 8-bit luminance and 8-bit alpha
 *
 * @code{.unparsed}
 *   0          1          2          3
 * | Yyyyyyyy | Aaaaaaaa | Yyyyyyyy | Aaaaaaaa | ...
 * @endcode
 */
#define PIX_FMT_AL88 PIX_FMT_FOURCC('A', 'L', '8', '8')

/**
 * 4-bit Greyscale format with 2 pixels packed per byte
 *
 * @code{.unparsed}
 *   0          1          2          3
 * | RrrrRrrr | RrrrRrrr | RrrrRrrr | RrrrRrrr | ...
 * @endcode
 *
 * The high nibble stores the left pixel and the low nibble stores the
 * right pixel.
 */
#define PIX_FMT_R4 PIX_FMT_FOURCC('R', '4', ' ', ' ')

/**
 * Little endian, with the 6 most significant bits set to Zero.
 * @code{.unparsed}
 *   0                   1                   2                   3
 * | yyyyyyyy 000000Yy | yyyyyyyy 000000Yy | yyyyyyyy 000000Yy | yyyyyyyy 000000Yy | ...
 * | yyyyyyyy 000000Yy | yyyyyyyy 000000Yy | yyyyyyyy 000000Yy | yyyyyyyy 000000Yy | ...
 * @endcode
 */
#define PIX_FMT_Y10 PIX_FMT_FOURCC('Y', '1', '0', ' ')

/**
 * Little endian, with the 4 most significant bits set to Zero.
 * @code{.unparsed}
 *   0                   1                   2                   3
 * | yyyyyyyy 0000Yyyy | yyyyyyyy 0000Yyyy | yyyyyyyy 0000Yyyy | yyyyyyyy 0000Yyyy | ...
 * | yyyyyyyy 0000Yyyy | yyyyyyyy 0000Yyyy | yyyyyyyy 0000Yyyy | yyyyyyyy 0000Yyyy | ...
 * @endcode
 */
#define PIX_FMT_Y12 PIX_FMT_FOURCC('Y', '1', '2', ' ')

/**
 * Little endian, with the 2 most significant bits set to Zero.
 * @code{.unparsed}
 *   0                   1                   2                   3
 * | yyyyyyyy 00Yyyyyy | yyyyyyyy 00Yyyyyy | yyyyyyyy 00Yyyyyy | yyyyyyyy 00Yyyyyy | ...
 * | yyyyyyyy 00Yyyyyy | yyyyyyyy 00Yyyyyy | yyyyyyyy 00Yyyyyy | yyyyyyyy 00Yyyyyy | ...
 * @endcode
 */
#define PIX_FMT_Y14 PIX_FMT_FOURCC('Y', '1', '4', ' ')

/**
 * Little endian.
 * @code{.unparsed}
 *   0                   1                   2                   3
 * | yyyyyyyy Yyyyyyyy | yyyyyyyy Yyyyyyyy | yyyyyyyy Yyyyyyyy | yyyyyyyy Yyyyyyyy | ...
 * | yyyyyyyy Yyyyyyyy | yyyyyyyy Yyyyyyyy | yyyyyyyy Yyyyyyyy | yyyyyyyy Yyyyyyyy | ...
 * @endcode
 */
#define PIX_FMT_Y16 PIX_FMT_FOURCC('Y', '1', '6', ' ')

/**
 * 8-bit luma-only split in two 4-bit blocks each zero-padded, little endian.
 *
 * @code{.unparsed}
 *   0                   1                   2                   3
 * | 0000yyyy 0000Yyyy | 0000yyyy 0000Yyyy | 0000yyyy 0000Yyyy | 0000yyyy 0000Yyyy | ...
 * @endcode
 */
#define PIX_FMT_Y8P16 PIX_FMT_FOURCC('Y', '8', 'P', '2')

/**
 * @code{.unparsed}
 *   0          1          2          3          3 2 1 0
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | yyyyyyyy | ...
 * @endcode
 */
#define PIX_FMT_Y10P PIX_FMT_FOURCC('Y', '1', '0', 'P')

/**
 * @code{.unparsed}
 *   0          1          1   0      2          3          3   2
 * | Yyyyyyyy | Yyyyyyyy | yyyyyyyy | Yyyyyyyy | Yyyyyyyy | yyyyyyyy | ...
 * | Yyyyyyyy | Yyyyyyyy | yyyyyyyy | Yyyyyyyy | Yyyyyyyy | yyyyyyyy | ...
 * @endcode
 */
#define PIX_FMT_Y12P PIX_FMT_FOURCC('Y', '1', '2', 'P')

/**
 * @code{.unparsed}
 *   0          1          2          3          1 0      2   1    3     2
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | yyyyyyyy yyyyyyyy yyyyyyyy | ...
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | yyyyyyyy yyyyyyyy yyyyyyyy | ...
 * @endcode
 */
#define PIX_FMT_Y14P PIX_FMT_FOURCC('Y', '1', '4', 'P')

/**
 * 5 red bits [15:11], 6 green bits [10:5], 5 blue bits [4:0].
 * This 16-bit integer is then packed in big endian format over two bytes:
 *
 * @code{.unparsed}
 *   15.....8 7......0
 * | RrrrrGgg gggBbbbb | ...
 * @endcode
 */
#define PIX_FMT_RGB565X PIX_FMT_FOURCC('R', 'G', 'B', 'R')

/**
 * 5 red bits [15:11], 6 green bits [10:5], 5 blue bits [4:0].
 * This 16-bit integer is then packed in little endian format over two bytes:
 *
 * @code{.unparsed}
 *   7......0 15.....8
 * | gggBbbbb RrrrrGgg | ...
 * @endcode
 */
#define PIX_FMT_RGB565 PIX_FMT_FOURCC('R', 'G', 'B', 'P')

/**
 * 24 bit BGR format with 8 bit per component
 *
 * @code{.unparsed}
 * | Bbbbbbbb Gggggggg Rrrrrrrr | ...
 * @endcode
 */
#define PIX_FMT_BGR24 PIX_FMT_FOURCC('B', 'G', 'R', '3')

/**
 * 24 bit RGB format with 8 bit per component
 *
 * @code{.unparsed}
 * | Rrrrrrrr Gggggggg Bbbbbbbb | ...
 * @endcode
 */
#define PIX_FMT_RGB24 PIX_FMT_FOURCC('R', 'G', 'B', '3')

/**
 * The first byte is alpha (A) for each pixel.
 *
 * @code{.unparsed}
 * | Aaaaaaaa Rrrrrrrr Gggggggg Bbbbbbbb | ...
 * @endcode
 */
#define PIX_FMT_ARGB32 PIX_FMT_FOURCC('B', 'A', '2', '4')

/**
 * The last byte is alpha (A) for each pixel.
 * *
 * @warning Linux calls this format ABGR32 due to a historical typo
 *
 * @code{.unparsed}
 * | Bbbbbbbb Gggggggg Rrrrrrrr Aaaaaaaa | ...
 * @endcode
 */
#define PIX_FMT_BGRA32 PIX_FMT_FOURCC('A', 'R', '2', '4')

/**
 * The last byte is alpha (A) for each pixel.
 *
 * @code{.unparsed}
 * | Rrrrrrrr Gggggggg Bbbbbbbb Aaaaaaaa | ...
 * @endcode
 */
#define PIX_FMT_RGBA32 PIX_FMT_FOURCC('A', 'B', '2', '4')

/**
 * The first byte is alpha (A) for each pixel.
 *
 * @warning Linux calls this format BGRA32 due to a historical typo
 *
 * @code{.unparsed}
 * | Aaaaaaaa Bbbbbbbb Gggggggg Rrrrrrrr | ...
 * @endcode
 */
#define PIX_FMT_ABGR32 PIX_FMT_FOURCC('R', 'A', '2', '4')

/**
 * The first byte is empty (X) for each pixel.
 *
 * @code{.unparsed}
 * | Xxxxxxxx Rrrrrrrr Gggggggg Bbbbbbbb | ...
 * @endcode
 */
#define PIX_FMT_XRGB32 PIX_FMT_FOURCC('B', 'X', '2', '4')

/**
 * The first byte is empty (X) for each pixel.
 *
 * @warning Linux calls this format BGRX32 due to a historical typo
 *
 * @code{.unparsed}
 * | Xxxxxxxx Bbbbbbbb Gggggggg Rrrrrrrr | ...
 * @endcode
 */
#define PIX_FMT_XBGR32 PIX_FMT_FOURCC('R', 'X', '2', '4')

/**
 * The last byte is empty (X) for each pixel.
 *
 * @warning Linux calls this format XBGR32 due to a historical typo
 *
 * @code{.unparsed}
 * | Bbbbbbbb Gggggggg Rrrrrrrr Xxxxxxxx | ...
 * @endcode
 */
#define PIX_FMT_BGRX32 PIX_FMT_FOURCC('X', 'R', '2', '4')

/**
 * The last byte is empty (X) for each pixel.
 *
 * @code{.unparsed}
 * | Rrrrrrrr Gggggggg Bbbbbbbb Xxxxxxxx | ...
 * @endcode
 */
#define PIX_FMT_RGBX32 PIX_FMT_FOURCC('X', 'B', '2', '4')

/**
 * There is either a missing channel per pixel, U or V.
 * The value is to be averaged over 2 pixels to get the value of individual pixel.
 *
 * @code{.unparsed}
 * | Yyyyyyyy Uuuuuuuu | Yyyyyyyy Vvvvvvvv | ...
 * @endcode
 */
#define PIX_FMT_YUYV PIX_FMT_FOURCC('Y', 'U', 'Y', 'V')

/**
 * @code{.unparsed}
 * | Yyyyyyyy Vvvvvvvv | Yyyyyyyy Uuuuuuuu | ...
 * @endcode
 */
#define PIX_FMT_YVYU PIX_FMT_FOURCC('Y', 'V', 'Y', 'U')

/**
 * @code{.unparsed}
 * | Vvvvvvvv Yyyyyyyy | Uuuuuuuu Yyyyyyyy | ...
 * @endcode
 */
#define PIX_FMT_VYUY PIX_FMT_FOURCC('V', 'Y', 'U', 'Y')

/**
 * @code{.unparsed}
 * | Uuuuuuuu Yyyyyyyy | Vvvvvvvv Yyyyyyyy | ...
 * @endcode
 */
#define PIX_FMT_UYVY PIX_FMT_FOURCC('U', 'Y', 'V', 'Y')

/**
 * The first byte is empty (X) for each pixel.
 *
 * @code{.unparsed}
 * | Xxxxxxxx Yyyyyyyy Uuuuuuuu Vvvvvvvv | ...
 * @endcode
 */
#define PIX_FMT_XYUV32 PIX_FMT_FOURCC('X', 'Y', 'U', 'V')

/**
 * 24 bit YUV format with 8 bit per component
 *
 * @code{.unparsed}
 * | Yyyyyyyy Uuuuuuuu Vvvvvvvv | ...
 * @endcode
 */
#define PIX_FMT_YUV24 PIX_FMT_FOURCC('Y', 'U', 'V', '3')

/**
 * Chroma (U/V) are subsampled horizontally and vertically
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
#define PIX_FMT_NV12 PIX_FMT_FOURCC('N', 'V', '1', '2')

/**
 * Chroma (U/V) are subsampled horizontally and vertically
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
#define PIX_FMT_NV21 PIX_FMT_FOURCC('N', 'V', '2', '1')

/**
 * Chroma (U/V) are subsampled horizontally
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
#define PIX_FMT_NV16 PIX_FMT_FOURCC('N', 'V', '1', '6')

/**
 * Chroma (U/V) are subsampled horizontally
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

#define PIX_FMT_NV61 PIX_FMT_FOURCC('N', 'V', '6', '1')

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
#define PIX_FMT_NV24 PIX_FMT_FOURCC('N', 'V', '2', '4')

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
#define PIX_FMT_NV42 PIX_FMT_FOURCC('N', 'V', '4', '2')

/**
 * Chroma (U/V) are subsampled horizontally and vertically
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
#define PIX_FMT_YUV420 PIX_FMT_FOURCC('Y', 'U', '1', '2')

/**
 * Chroma (U/V) are subsampled horizontally and vertically
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
#define PIX_FMT_YVU420 PIX_FMT_FOURCC('Y', 'V', '1', '2')

/**
 * 4-bit indexed color format with 2 pixels packed per byte
 *
 * @code{.unparsed}
 *   0          1          2          3
 * | CcccCccc | CcccCccc | CcccCccc | CcccCccc | ...
 * @endcode
 *
 * The high nibble stores the left pixel and the low nibble stores the
 * right pixel. Palette semantics are display-specific.
 */
#define PIX_FMT_C4 PIX_FMT_FOURCC('C', '4', ' ', ' ')

/**
 * 1-bit monochrome format with 1 bit per pixel, thus each byte represent 8 pixels
 * R(1) represent the white while 0 represent black
 * Bit order and tiling are not part of the format and instead are set by
 * SCREEN_INFO_MONO_MSB_FIRST / SCREEN_INFO_MONO_VTILED
 *
 * @code{.unparsed}
 *   0          1          2          3
 * | RRRRRRRR | RRRRRRRR | RRRRRRRR | RRRRRRRR | ...
 * @endcode
 */
#define PIX_FMT_R1	PIX_FMT_FOURCC('R', '1', ' ', ' ')

/**
 * 1-bit monochrome format with 1 bit per pixel, thus each byte represent 8 pixels
 * D(1) represent the black while 0 represent white
 * Bit order and tiling are not part of the format and instead are set by
 * SCREEN_INFO_MONO_MSB_FIRST / SCREEN_INFO_MONO_VTILED
 *
 * @code{.unparsed}
 *   0          1          2          3
 * | DDDDDDDD | DDDDDDDD | DDDDDDDD | DDDDDDDD | ...
 * @endcode
 */
#define PIX_FMT_D1	PIX_FMT_FOURCC('D', '1', ' ', ' ')

/**
 * Both JPEG (single frame) and Motion-JPEG (MJPEG, multiple JPEG frames concatenated)
 */
#define PIX_FMT_JPEG PIX_FMT_FOURCC('J', 'P', 'E', 'G')

/**
 * H264 with start code
 */
#define PIX_FMT_H264 PIX_FMT_FOURCC('H', '2', '6', '4')

/**
 * H264 without start code
 */
#define PIX_FMT_H264_NO_SC PIX_FMT_FOURCC('A', 'V', 'C', '1')

/**
 * PNG
 */
#define PIX_FMT_PNG PIX_FMT_FOURCC('P', 'N', 'G', ' ')

/** @} */

#endif /* ZEPHYR_INCLUDE_MEDIA_FORMATS_H_ */
