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
 * @brief Main header file for Zephyr pixel formats.
 */

#ifndef ZEPHYR_INCLUDE_MEDIA_FORMATS_H
#define ZEPHYR_INCLUDE_MEDIA_FORMATS_H

/**
 * @brief Zephyr pixel format definitions.
 * @defgroup pixel_formats Pixel Formats
 * @version 0.0.1
 *
 * Pixel format identifiers are also available to devicetree sources through
 * <zephyr/dt-bindings/media/formats.h>.
 *
 * The '|' characters separate the pixels or logical blocks, and spaces separate the bytes.
 * The uppercase letter represents the most significant bit.
 * The lowercase letters represent the rest of the bits.
 *
 * @{
 */

#include <stdint.h>
#include <zephyr/media/formats.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Convert a four-character-string to a four-character-code
 *
 * Convert a string literal or variable into a four-character-code
 * as defined by @ref PIXFMT_FOURCC.
 *
 * @param str String to be converted
 * @return Four-character-code.
 */
#define PIXFMT_FOURCC_FROM_STR(str) PIXFMT_FOURCC((str)[0], (str)[1], (str)[2], (str)[3])

/**
 * @brief Convert a four-character-code to a four-character-string
 *
 * Convert a four-character code as defined by @ref PIXFMT_FOURCC into a string that can be used
 * anywhere, such as in debug logs with the %s print formatter.
 *
 * @param fourcc The 32-bit four-character-code integer to be converted, in CPU-native endinaness.
 * @return Four-character-string built out of it.
 */
#define PIXFMT_FOURCC_TO_STR(fourcc)								\
	((char[]){										\
		(char)((fourcc) & 0xFF),							\
		(char)(((fourcc) >> 8) & 0xFF),							\
		(char)(((fourcc) >> 16) & 0xFF),						\
		(char)(((fourcc) >> 24) & 0xFF),						\
		'\0'										\
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
#define PIXFMT_FOREACH_BAYER(X, ...)						\
	PIXFMT_FOREACH_BAYER_PADDED(X, __VA_ARGS__)				\
	PIXFMT_FOREACH_BAYER_MIPI_PACKED(X, __VA_ARGS__)			\
	PIXFMT_FOREACH_BAYER_NON_PACKED(X, __VA_ARGS__)

/**
 * @brief Repeat a macro for every Bayer padded format, passed as first parameter
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define PIXFMT_FOREACH_BAYER_PADDED(X, ...)					\
	X(PIXFMT_SBGGR8P16, __VA_ARGS__)					\
	X(PIXFMT_SGBRG8P16, __VA_ARGS__)					\
	X(PIXFMT_SGRBG8P16, __VA_ARGS__)					\
	X(PIXFMT_SRGGB8P16, __VA_ARGS__)

/**
 * @brief Repeat a macro for every Bayer MIPI-packed format, passed as first parameter
 *
 * A red, green, blue channel for every pixel, other than 8-bit per pixel.
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define PIXFMT_FOREACH_BAYER_MIPI_PACKED(X, ...)			\
	X(PIXFMT_SBGGR10P, __VA_ARGS__)					\
	X(PIXFMT_SGBRG10P, __VA_ARGS__)					\
	X(PIXFMT_SGRBG10P, __VA_ARGS__)					\
	X(PIXFMT_SRGGB10P, __VA_ARGS__)					\
	X(PIXFMT_SBGGR12P, __VA_ARGS__)					\
	X(PIXFMT_SGBRG12P, __VA_ARGS__)					\
	X(PIXFMT_SGRBG12P, __VA_ARGS__)					\
	X(PIXFMT_SRGGB12P, __VA_ARGS__)					\
	X(PIXFMT_SBGGR14P, __VA_ARGS__)					\
	X(PIXFMT_SGBRG14P, __VA_ARGS__)					\
	X(PIXFMT_SGRBG14P, __VA_ARGS__)					\
	X(PIXFMT_SRGGB14P, __VA_ARGS__)

/**
 * @brief Repeat a macro for every Bayer non-packed format, passed as first parameter
 *
 * A red, green, blue channel for every pixel, other than 8-bit per pixel.
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define PIXFMT_FOREACH_BAYER_NON_PACKED(X, ...)				\
	X(PIXFMT_SBGGR8, __VA_ARGS__)					\
	X(PIXFMT_SGBRG8, __VA_ARGS__)					\
	X(PIXFMT_SGRBG8, __VA_ARGS__)					\
	X(PIXFMT_SRGGB8, __VA_ARGS__)					\
	X(PIXFMT_SGBRG10, __VA_ARGS__)					\
	X(PIXFMT_SGRBG10, __VA_ARGS__)					\
	X(PIXFMT_SRGGB10, __VA_ARGS__)					\
	X(PIXFMT_SBGGR12, __VA_ARGS__)					\
	X(PIXFMT_SGBRG12, __VA_ARGS__)					\
	X(PIXFMT_SGRBG12, __VA_ARGS__)					\
	X(PIXFMT_SRGGB12, __VA_ARGS__)					\
	X(PIXFMT_SBGGR14, __VA_ARGS__)					\
	X(PIXFMT_SGBRG14, __VA_ARGS__)					\
	X(PIXFMT_SGRBG14, __VA_ARGS__)					\
	X(PIXFMT_SRGGB14, __VA_ARGS__)					\
	X(PIXFMT_SBGGR16, __VA_ARGS__)					\
	X(PIXFMT_SGBRG16, __VA_ARGS__)					\
	X(PIXFMT_SGRBG16, __VA_ARGS__)					\
	X(PIXFMT_SRGGB16, __VA_ARGS__)

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
#define PIXFMT_FOREACH_GRAYSCALE(X, ...)					\
	PIXFMT_FOREACH_GRAYSCALE_NON_PACKED(X, __VA_ARGS__)			\
	PIXFMT_FOREACH_GRAYSCALE_PADDED(X, __VA_ARGS__)				\
	PIXFMT_FOREACH_GRAYSCALE_MIPI_PACKED(X, __VA_ARGS__)

/**
 * @brief Repeat a macro for every grayscale non-packed format, passed as first parameter
 *
 * A red, green, blue channel for every pixel, other than 8-bit per pixel.
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define PIXFMT_FOREACH_GRAYSCALE_NON_PACKED(X, ...)			\
	X(PIXFMT_GREY, __VA_ARGS__)					\
	X(PIXFMT_Y10, __VA_ARGS__)					\
	X(PIXFMT_Y12, __VA_ARGS__)					\
	X(PIXFMT_Y14, __VA_ARGS__)					\
	X(PIXFMT_Y16, __VA_ARGS__)

/**
 * @brief Repeat a macro for every grayscale padded format, passed as first parameter
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define PIXFMT_FOREACH_GRAYSCALE_PADDED(X, ...)					\
	X(PIXFMT_Y8P16, __VA_ARGS__)

/**
 * @brief Repeat a macro for every grayscale MIPI-packed format, passed as first parameter
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define PIXFMT_FOREACH_GRAYSCALE_MIPI_PACKED(X, ...)			\
	X(PIXFMT_Y10P, __VA_ARGS__)					\
	X(PIXFMT_Y12P, __VA_ARGS__)					\
	X(PIXFMT_Y14P, __VA_ARGS__)

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
#define PIXFMT_FOREACH_RGB(X, ...)						\
	PIXFMT_FOREACH_RGB_PACKED(X, __VA_ARGS__)				\
	PIXFMT_FOREACH_RGB_NON_PACKED(X, __VA_ARGS__)				\
	PIXFMT_FOREACH_RGB_ALPHA(X, __VA_ARGS__)				\
	PIXFMT_FOREACH_RGB_PADDED(X, __VA_ARGS__)

/**
 * @brief Repeat a macro for every RGB packed format, passed as first parameter
 *
 * A red, green, blue channel for every pixel, other than 8-bit per pixel.
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define PIXFMT_FOREACH_RGB_PACKED(X, ...)				\
	X(PIXFMT_RGB565X, __VA_ARGS__)					\
	X(PIXFMT_RGB565, __VA_ARGS__)

/**
 * @brief Repeat a macro for every RGB non-packed format, passed as first parameter
 *
 * A red, green, blue channel for every pixel, 8-bit per pixel.
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define PIXFMT_FOREACH_RGB_NON_PACKED(X, ...)				\
	X(PIXFMT_BGR24, __VA_ARGS__)					\
	X(PIXFMT_RGB24, __VA_ARGS__)

/**
 * @brief Repeat a macro for every RGB alpha format, passed as first parameter
 *
 * A red, green, blue, alpha channel for every pixel.
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define PIXFMT_FOREACH_RGB_ALPHA(X, ...)				\
	X(PIXFMT_ARGB32, __VA_ARGS__)					\
	X(PIXFMT_ABGR32, __VA_ARGS__)					\
	X(PIXFMT_RGBA32, __VA_ARGS__)					\
	X(PIXFMT_BGRA32, __VA_ARGS__)

/**
 * @brief Repeat a macro for every RGB padded format, passed as first parameter
 *
 * A red, green, blue channel for every pixel.
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define PIXFMT_FOREACH_RGB_PADDED(X, ...)				\
	X(PIXFMT_XRGB32, __VA_ARGS__)

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
#define PIXFMT_FOREACH_YUV(X, ...)						\
	PIXFMT_FOREACH_YUV_FULL_PLANAR(X, __VA_ARGS__)				\
	PIXFMT_FOREACH_YUV_NON_PLANAR(X, __VA_ARGS__)				\
	PIXFMT_FOREACH_YUV_SEMI_PLANAR(X, __VA_ARGS__)

/**
 * @brief Repeat a macro for every YUV Non-Planar format, passed as first parameter
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define PIXFMT_FOREACH_YUV_NON_PLANAR(X, ...)					\
	X(PIXFMT_YUYV, __VA_ARGS__)					\
	X(PIXFMT_YVYU, __VA_ARGS__)					\
	X(PIXFMT_VYUY, __VA_ARGS__)					\
	X(PIXFMT_UYVY, __VA_ARGS__)					\
	X(PIXFMT_YUV24, __VA_ARGS__)					\
	X(PIXFMT_XYUV32, __VA_ARGS__)

/**
 * @brief Repeat a macro for every YUV Semi-Planar format, passed as first parameter
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define PIXFMT_FOREACH_YUV_SEMI_PLANAR(X, ...)				\
	X(PIXFMT_NV12, __VA_ARGS__)					\
	X(PIXFMT_NV21, __VA_ARGS__)					\
	X(PIXFMT_NV16, __VA_ARGS__)					\
	X(PIXFMT_NV61, __VA_ARGS__)					\
	X(PIXFMT_NV24, __VA_ARGS__)					\
	X(PIXFMT_NV42, __VA_ARGS__)

/**
 * @brief Repeat a macro for every YUV Full-Planar format, passed as first parameter
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define PIXFMT_FOREACH_YUV_FULL_PLANAR(X, ...)				\
	X(PIXFMT_YUV420, __VA_ARGS__)					\
	X(PIXFMT_YVU420, __VA_ARGS__)

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
#define PIXFMT_FOREACH_COMPRESSED(X, ...)				\
	X(PIXFMT_JPEG, __VA_ARGS__)					\
	X(PIXFMT_H264, __VA_ARGS__)					\
	X(PIXFMT_H264_NO_SC, __VA_ARGS__)				\
	X(PIXFMT_PNG, __VA_ARGS__)

/**
 * @}
 */

/** @cond INTERNAL_HIDDEN */
#define _PIXFMT_FMT_OR_EQ(pixfmt_a, pixfmt_b) || ((pixfmt_a) == (pixfmt_b))
/** @endcond */

/**
 * @brief Test if a fourcc is a grayscale format
 *
 * @param pixfmt FourCC of the pixel format to test
 * @return Whether the format is known to match this category
 */
#define PIXFMT_IS_GRAYSCALE(pixfmt)						\
	(0 PIXFMT_FOREACH_GRAYSCALE(_PIXFMT_FMT_OR_EQ, pixfmt))

/**
 * @brief Test if a fourcc is a Bayer format
 *
 * @param pixfmt FourCC of the pixel format to test
 * @return Whether the format is known to match this category
 */
#define PIXFMT_IS_BAYER(pixfmt)						\
	(0 PIXFMT_FOREACH_BAYER(_PIXFMT_FMT_OR_EQ, pixfmt))

/**
 * @brief Test if a fourcc is an RGB format (bayer excluded)
 *
 * @param pixfmt FourCC of the pixel format to test
 * @return Whether the format is known to match this category
 */
#define PIXFMT_IS_RGB(pixfmt)						\
	(0 PIXFMT_FOREACH_RGB(_PIXFMT_FMT_OR_EQ, pixfmt))

/**
 * @brief Test if a fourcc is an YUV format
 *
 * @param pixfmt FourCC of the pixel format to test
 * @return Whether the format is known to match this category
 */
#define PIXFMT_IS_YUV(pixfmt)						\
	(0 PIXFMT_FOREACH_YUV(_PIXFMT_FMT_OR_EQ, pixfmt))

/**
 * @brief Test if a fourcc is any of the MIPI-packed formats
 *
 * @param pixfmt FourCC of the pixel format to test
 * @return Whether the format is known to match this category
 */
#define PIXFMT_IS_MIPI_PACKED(pixfmt)					\
	(0 PIXFMT_FOREACH_BAYER_MIPI_PACKED(_PIXFMT_FMT_OR_EQ, pixfmt)		\
	   PIXFMT_FOREACH_GRAYSCALE_MIPI_PACKED(_PIXFMT_FMT_OR_EQ, pixfmt))

/**
 * @brief Test if a fourcc is any of the MIPI-packed formats
 *
 * @param pixfmt FourCC of the pixel format to test
 * @return Whether the format is known to match this category
 */
#define PIXFMT_IS_PADDED(pixfmt)						\
	(0 PIXFMT_FOREACH_BAYER_PADDED(_PIXFMT_FMT_OR_EQ, pixfmt)			\
	   PIXFMT_FOREACH_GRAYSCALE_PADDED(_PIXFMT_FMT_OR_EQ, pixfmt))

/**
 * @brief Test if a fourcc is any of the semi-planar formats
 *
 * @param pixfmt FourCC of the pixel format to test
 * @return Whether the format is known to match this category
 */
#define PIXFMT_IS_SEMI_PLANAR(pixfmt)					\
	(0 PIXFMT_FOREACH_YUV_SEMI_PLANAR(_PIXFMT_FMT_OR_EQ, pixfmt))

/**
 * @brief Test if a fourcc is any of the full-planar formats
 *
 * @param pixfmt FourCC of the pixel format to test
 * @return Whether the format is known to match this category
 */
#define PIXFMT_IS_FULL_PLANAR(pixfmt)					\
	(0 PIXFMT_FOREACH_YUV_FULL_PLANAR(_PIXFMT_FMT_OR_EQ, pixfmt))

/**
 * @brief Get number of bits per pixel of a pixel format
 *
 * @param pixfmt FourCC pixel format value.
 *
 * @retval 0 The format is unhandled or has a variable number of bits.
 * @return Bit size of one pixel for this format.
 */
static inline unsigned int pixfmt_bpp(uint32_t pixfmt)
{
	switch (pixfmt) {
	case PIXFMT_SBGGR8:
	case PIXFMT_SGBRG8:
	case PIXFMT_SGRBG8:
	case PIXFMT_SRGGB8:
	case PIXFMT_GREY:
		return 8;
	case PIXFMT_SBGGR10P:
	case PIXFMT_SGBRG10P:
	case PIXFMT_SGRBG10P:
	case PIXFMT_SRGGB10P:
	case PIXFMT_Y10P:
		return 10;
	case PIXFMT_SBGGR12P:
	case PIXFMT_SGBRG12P:
	case PIXFMT_SGRBG12P:
	case PIXFMT_SRGGB12P:
	case PIXFMT_Y12P:
	case PIXFMT_NV12:
	case PIXFMT_NV21:
	case PIXFMT_YUV420:
	case PIXFMT_YVU420:
		return 12;
	case PIXFMT_SBGGR14P:
	case PIXFMT_SGBRG14P:
	case PIXFMT_SGRBG14P:
	case PIXFMT_SRGGB14P:
	case PIXFMT_Y14P:
		return 14;
	case PIXFMT_RGB565:
	case PIXFMT_YUYV:
	case PIXFMT_YVYU:
	case PIXFMT_UYVY:
	case PIXFMT_VYUY:
	case PIXFMT_SBGGR8P16:
	case PIXFMT_SGBRG8P16:
	case PIXFMT_SGRBG8P16:
	case PIXFMT_SRGGB8P16:
	case PIXFMT_SBGGR10:
	case PIXFMT_SGBRG10:
	case PIXFMT_SGRBG10:
	case PIXFMT_SRGGB10:
	case PIXFMT_SBGGR12:
	case PIXFMT_SGBRG12:
	case PIXFMT_SGRBG12:
	case PIXFMT_SRGGB12:
	case PIXFMT_SBGGR14:
	case PIXFMT_SGBRG14:
	case PIXFMT_SGRBG14:
	case PIXFMT_SRGGB14:
	case PIXFMT_SBGGR16:
	case PIXFMT_SGBRG16:
	case PIXFMT_SGRBG16:
	case PIXFMT_SRGGB16:
	case PIXFMT_Y8P16:
	case PIXFMT_Y10:
	case PIXFMT_Y12:
	case PIXFMT_Y14:
	case PIXFMT_Y16:
	case PIXFMT_NV16:
	case PIXFMT_NV61:
		return 16;
	case PIXFMT_BGR24:
	case PIXFMT_RGB24:
	case PIXFMT_NV24:
	case PIXFMT_NV42:
	case PIXFMT_YUV24:
		return 24;
	case PIXFMT_XYUV32:
	case PIXFMT_ARGB32:
	case PIXFMT_ABGR32:
	case PIXFMT_RGBA32:
	case PIXFMT_BGRA32:
	case PIXFMT_XRGB32:
	case PIXFMT_XBGR32:
	case PIXFMT_RGBX32:
	case PIXFMT_BGRX32:
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

#endif /* ZEPHYR_INCLUDE_MEDIA_FORMATS_H */
