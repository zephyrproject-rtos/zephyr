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

#ifndef ZEPHYR_INCLUDE_MEDIA_FORMATS_MISC_H
#define ZEPHYR_INCLUDE_MEDIA_FORMATS_MISC_H

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
#include <zephyr/media/pixel_format.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t pixel_format_t;

/**
 * @brief Convert a four-character-string to a four-character-code
 *
 * Convert a string literal or variable into a four-character-code
 * as defined by @ref PIX_FMT_FOURCC.
 *
 * @param str String to be converted
 * @return Four-character-code.
 */
#define PIX_FMT_FOURCC_FROM_STR(str) PIX_FMT_FOURCC((str)[0], (str)[1], (str)[2], (str)[3])

/**
 * @brief Convert a four-character-code to a four-character-string
 *
 * Convert a four-character code as defined by @ref PIX_FMT_FOURCC into a string that can be used
 * anywhere, such as in debug logs with the %s print formatter.
 *
 * @param fourcc The 32-bit four-character-code integer to be converted, in CPU-native endinaness.
 * @return Four-character-string built out of it.
 */
#define PIX_FMT_FOURCC_TO_STR(fourcc)								\
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
#define PIX_FMT_FOREACH_BAYER(X, ...)						\
	PIX_FMT_FOREACH_BAYER_PADDED(X, __VA_ARGS__)				\
	PIX_FMT_FOREACH_BAYER_MIPI_PACKED(X, __VA_ARGS__)			\
	PIX_FMT_FOREACH_BAYER_NON_PACKED(X, __VA_ARGS__)

/**
 * @brief Repeat a macro for every Bayer padded format, passed as first parameter
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define PIX_FMT_FOREACH_BAYER_PADDED(X, ...)					\
	X(PIX_FMT_SBGGR8P16, __VA_ARGS__)					\
	X(PIX_FMT_SGBRG8P16, __VA_ARGS__)					\
	X(PIX_FMT_SGRBG8P16, __VA_ARGS__)					\
	X(PIX_FMT_SRGGB8P16, __VA_ARGS__)

/**
 * @brief Repeat a macro for every Bayer MIPI-packed format, passed as first parameter
 *
 * A red, green, blue channel for every pixel, other than 8-bit per pixel.
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define PIX_FMT_FOREACH_BAYER_MIPI_PACKED(X, ...)			\
	X(PIX_FMT_SBGGR10P, __VA_ARGS__)					\
	X(PIX_FMT_SGBRG10P, __VA_ARGS__)					\
	X(PIX_FMT_SGRBG10P, __VA_ARGS__)					\
	X(PIX_FMT_SRGGB10P, __VA_ARGS__)					\
	X(PIX_FMT_SBGGR12P, __VA_ARGS__)					\
	X(PIX_FMT_SGBRG12P, __VA_ARGS__)					\
	X(PIX_FMT_SGRBG12P, __VA_ARGS__)					\
	X(PIX_FMT_SRGGB12P, __VA_ARGS__)					\
	X(PIX_FMT_SBGGR14P, __VA_ARGS__)					\
	X(PIX_FMT_SGBRG14P, __VA_ARGS__)					\
	X(PIX_FMT_SGRBG14P, __VA_ARGS__)					\
	X(PIX_FMT_SRGGB14P, __VA_ARGS__)

/**
 * @brief Repeat a macro for every Bayer non-packed format, passed as first parameter
 *
 * A red, green, blue channel for every pixel, other than 8-bit per pixel.
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define PIX_FMT_FOREACH_BAYER_NON_PACKED(X, ...)				\
	X(PIX_FMT_SBGGR8, __VA_ARGS__)					\
	X(PIX_FMT_SGBRG8, __VA_ARGS__)					\
	X(PIX_FMT_SGRBG8, __VA_ARGS__)					\
	X(PIX_FMT_SRGGB8, __VA_ARGS__)					\
	X(PIX_FMT_SBGGR10, __VA_ARGS__)					\
	X(PIX_FMT_SGBRG10, __VA_ARGS__)					\
	X(PIX_FMT_SGRBG10, __VA_ARGS__)					\
	X(PIX_FMT_SRGGB10, __VA_ARGS__)					\
	X(PIX_FMT_SBGGR12, __VA_ARGS__)					\
	X(PIX_FMT_SGBRG12, __VA_ARGS__)					\
	X(PIX_FMT_SGRBG12, __VA_ARGS__)					\
	X(PIX_FMT_SRGGB12, __VA_ARGS__)					\
	X(PIX_FMT_SBGGR14, __VA_ARGS__)					\
	X(PIX_FMT_SGBRG14, __VA_ARGS__)					\
	X(PIX_FMT_SGRBG14, __VA_ARGS__)					\
	X(PIX_FMT_SRGGB14, __VA_ARGS__)					\
	X(PIX_FMT_SBGGR16, __VA_ARGS__)					\
	X(PIX_FMT_SGBRG16, __VA_ARGS__)					\
	X(PIX_FMT_SGRBG16, __VA_ARGS__)					\
	X(PIX_FMT_SRGGB16, __VA_ARGS__)

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
#define PIX_FMT_FOREACH_GRAYSCALE(X, ...)					\
	PIX_FMT_FOREACH_GRAYSCALE_NON_PACKED(X, __VA_ARGS__)			\
	PIX_FMT_FOREACH_GRAYSCALE_PADDED(X, __VA_ARGS__)				\
	PIX_FMT_FOREACH_GRAYSCALE_MIPI_PACKED(X, __VA_ARGS__)

/**
 * @brief Repeat a macro for every grayscale non-packed format, passed as first parameter
 *
 * A red, green, blue channel for every pixel, other than 8-bit per pixel.
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define PIX_FMT_FOREACH_GRAYSCALE_NON_PACKED(X, ...)			\
	X(PIX_FMT_R1, __VA_ARGS__)					\
	X(PIX_FMT_D1, __VA_ARGS__)					\
	X(PIX_FMT_R4, __VA_ARGS__)					\
	X(PIX_FMT_GREY, __VA_ARGS__)					\
	X(PIX_FMT_AL88, __VA_ARGS__)					\
	X(PIX_FMT_Y10, __VA_ARGS__)					\
	X(PIX_FMT_Y12, __VA_ARGS__)					\
	X(PIX_FMT_Y14, __VA_ARGS__)					\
	X(PIX_FMT_Y16, __VA_ARGS__)

/**
 * @brief Repeat a macro for every grayscale padded format, passed as first parameter
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define PIX_FMT_FOREACH_GRAYSCALE_PADDED(X, ...)					\
	X(PIX_FMT_Y8P16, __VA_ARGS__)

/**
 * @brief Repeat a macro for every grayscale MIPI-packed format, passed as first parameter
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define PIX_FMT_FOREACH_GRAYSCALE_MIPI_PACKED(X, ...)			\
	X(PIX_FMT_Y10P, __VA_ARGS__)					\
	X(PIX_FMT_Y12P, __VA_ARGS__)					\
	X(PIX_FMT_Y14P, __VA_ARGS__)

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
#define PIX_FMT_FOREACH_RGB(X, ...)						\
	PIX_FMT_FOREACH_RGB_PACKED(X, __VA_ARGS__)				\
	PIX_FMT_FOREACH_RGB_NON_PACKED(X, __VA_ARGS__)				\
	PIX_FMT_FOREACH_RGB_ALPHA(X, __VA_ARGS__)				\
	PIX_FMT_FOREACH_RGB_PADDED(X, __VA_ARGS__)

/**
 * @brief Repeat a macro for every RGB packed format, passed as first parameter
 *
 * A red, green, blue channel for every pixel, other than 8-bit per pixel.
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define PIX_FMT_FOREACH_RGB_PACKED(X, ...)				\
	X(PIX_FMT_RGB565X, __VA_ARGS__)					\
	X(PIX_FMT_RGB565, __VA_ARGS__)

/**
 * @brief Repeat a macro for every RGB non-packed format, passed as first parameter
 *
 * A red, green, blue channel for every pixel, 8-bit per pixel.
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define PIX_FMT_FOREACH_RGB_NON_PACKED(X, ...)				\
	X(PIX_FMT_BGR24, __VA_ARGS__)					\
	X(PIX_FMT_RGB24, __VA_ARGS__)

/**
 * @brief Repeat a macro for every RGB alpha format, passed as first parameter
 *
 * A red, green, blue, alpha channel for every pixel.
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define PIX_FMT_FOREACH_RGB_ALPHA(X, ...)				\
	X(PIX_FMT_ARGB32, __VA_ARGS__)					\
	X(PIX_FMT_ABGR32, __VA_ARGS__)					\
	X(PIX_FMT_RGBA32, __VA_ARGS__)					\
	X(PIX_FMT_BGRA32, __VA_ARGS__)

/**
 * @brief Repeat a macro for every RGB padded format, passed as first parameter
 *
 * A red, green, blue channel for every pixel.
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define PIX_FMT_FOREACH_RGB_PADDED(X, ...)				\
	X(PIX_FMT_XRGB32, __VA_ARGS__)					\
	X(PIX_FMT_XBGR32, __VA_ARGS__)					\
	X(PIX_FMT_RGBX32, __VA_ARGS__)					\
	X(PIX_FMT_BGRX32, __VA_ARGS__)

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
#define PIX_FMT_FOREACH_YUV(X, ...)						\
	PIX_FMT_FOREACH_YUV_FULL_PLANAR(X, __VA_ARGS__)				\
	PIX_FMT_FOREACH_YUV_NON_PLANAR(X, __VA_ARGS__)				\
	PIX_FMT_FOREACH_YUV_SEMI_PLANAR(X, __VA_ARGS__)

/**
 * @brief Repeat a macro for every YUV Non-Planar format, passed as first parameter
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define PIX_FMT_FOREACH_YUV_NON_PLANAR(X, ...)				\
	X(PIX_FMT_YUYV, __VA_ARGS__)					\
	X(PIX_FMT_YVYU, __VA_ARGS__)					\
	X(PIX_FMT_VYUY, __VA_ARGS__)					\
	X(PIX_FMT_UYVY, __VA_ARGS__)					\
	X(PIX_FMT_YUV24, __VA_ARGS__)					\
	X(PIX_FMT_XYUV32, __VA_ARGS__)

/**
 * @brief Repeat a macro for every YUV Semi-Planar format, passed as first parameter
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define PIX_FMT_FOREACH_YUV_SEMI_PLANAR(X, ...)				\
	X(PIX_FMT_NV12, __VA_ARGS__)					\
	X(PIX_FMT_NV21, __VA_ARGS__)					\
	X(PIX_FMT_NV16, __VA_ARGS__)					\
	X(PIX_FMT_NV61, __VA_ARGS__)					\
	X(PIX_FMT_NV24, __VA_ARGS__)					\
	X(PIX_FMT_NV42, __VA_ARGS__)

/**
 * @brief Repeat a macro for every YUV Full-Planar format, passed as first parameter
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define PIX_FMT_FOREACH_YUV_FULL_PLANAR(X, ...)				\
	X(PIX_FMT_YUV420, __VA_ARGS__)					\
	X(PIX_FMT_YVU420, __VA_ARGS__)

/**
 * @}
 */

/**
 * @name Indexed formats
 * @{
 */

/**
 * @brief Repeat a macro for every indexed format, passed as first parameter
 *
 * @param X macro to replicate
 * @param ... extra parameters
 */
#define PIX_FMT_FOREACH_INDEXED(X, ...)						\
	X(PIX_FMT_C4, __VA_ARGS__)

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
#define PIX_FMT_FOREACH_COMPRESSED(X, ...)				\
	X(PIX_FMT_JPEG, __VA_ARGS__)					\
	X(PIX_FMT_H264, __VA_ARGS__)					\
	X(PIX_FMT_H264_NO_SC, __VA_ARGS__)				\
	X(PIX_FMT_PNG, __VA_ARGS__)

/**
 * @}
 */

/** @cond INTERNAL_HIDDEN */
#define _PIX_FMT_FMT_OR_EQ(pixfmt_a, pixfmt_b) || ((pixfmt_a) == (pixfmt_b))
/** @endcond */

/**
 * @brief Test if a fourcc is a grayscale format
 *
 * @param pixfmt FourCC of the pixel format to test
 * @return Whether the format is known to match this category
 */
#define PIX_FMT_IS_GRAYSCALE(pixfmt)						\
	(0 PIX_FMT_FOREACH_GRAYSCALE(_PIX_FMT_FMT_OR_EQ, pixfmt))

/**
 * @brief Test if a fourcc is a Bayer format
 *
 * @param pixfmt FourCC of the pixel format to test
 * @return Whether the format is known to match this category
 */
#define PIX_FMT_IS_BAYER(pixfmt)						\
	(0 PIX_FMT_FOREACH_BAYER(_PIX_FMT_FMT_OR_EQ, pixfmt))

/**
 * @brief Test if a fourcc is an RGB format (bayer excluded)
 *
 * @param pixfmt FourCC of the pixel format to test
 * @return Whether the format is known to match this category
 */
#define PIX_FMT_IS_RGB(pixfmt)						\
	(0 PIX_FMT_FOREACH_RGB(_PIX_FMT_FMT_OR_EQ, pixfmt))

/**
 * @brief Test if a fourcc is an YUV format
 *
 * @param pixfmt FourCC of the pixel format to test
 * @return Whether the format is known to match this category
 */
#define PIX_FMT_IS_YUV(pixfmt)						\
	(0 PIX_FMT_FOREACH_YUV(_PIX_FMT_FMT_OR_EQ, pixfmt))

/**
 * @brief Test if a fourcc is an indexed format
 *
 * @param pixfmt FourCC of the pixel format to test
 * @return Whether the format is known to match this category
 */
#define PIX_FMT_IS_INDEXED(pixfmt)					\
	(0 PIX_FMT_FOREACH_INDEXED(_PIX_FMT_FMT_OR_EQ, pixfmt))

/**
 * @brief Test if a fourcc is a compressed format
 *
 * @param pixfmt FourCC of the pixel format to test
 * @return Whether the format is known to match this category
 */
#define PIX_FMT_IS_COMPRESSED(pixfmt)					\
	(0 PIX_FMT_FOREACH_COMPRESSED(_PIX_FMT_FMT_OR_EQ, pixfmt))

/**
 * @brief Test if a fourcc is any of the MIPI-packed formats
 *
 * @param pixfmt FourCC of the pixel format to test
 * @return Whether the format is known to match this category
 */
#define PIX_FMT_IS_MIPI_PACKED(pixfmt)					\
	(0 PIX_FMT_FOREACH_BAYER_MIPI_PACKED(_PIX_FMT_FMT_OR_EQ, pixfmt)		\
	   PIX_FMT_FOREACH_GRAYSCALE_MIPI_PACKED(_PIX_FMT_FMT_OR_EQ, pixfmt))

/**
 * @brief Test if a fourcc is any of the MIPI-packed formats
 *
 * @param pixfmt FourCC of the pixel format to test
 * @return Whether the format is known to match this category
 */
#define PIX_FMT_IS_PADDED(pixfmt)						\
	(0 PIX_FMT_FOREACH_BAYER_PADDED(_PIX_FMT_FMT_OR_EQ, pixfmt)			\
	   PIX_FMT_FOREACH_GRAYSCALE_PADDED(_PIX_FMT_FMT_OR_EQ, pixfmt))

/**
 * @brief Test if a fourcc is any of the semi-planar formats
 *
 * @param pixfmt FourCC of the pixel format to test
 * @return Whether the format is known to match this category
 */
#define PIX_FMT_IS_SEMI_PLANAR(pixfmt)					\
	(0 PIX_FMT_FOREACH_YUV_SEMI_PLANAR(_PIX_FMT_FMT_OR_EQ, pixfmt))

/**
 * @brief Test if a fourcc is any of the full-planar formats
 *
 * @param pixfmt FourCC of the pixel format to test
 * @return Whether the format is known to match this category
 */
#define PIX_FMT_IS_FULL_PLANAR(pixfmt)					\
	(0 PIX_FMT_FOREACH_YUV_FULL_PLANAR(_PIX_FMT_FMT_OR_EQ, pixfmt))

/**
 * @brief Get number of bits per pixel of a pixel format
 *
 * @param pixfmt FourCC pixel format value.
 *
 * @retval 0 The format is unhandled or has a variable number of bits.
 * @return Bit size of one pixel for this format.
 */
static inline size_t pix_fmt_bpp(pixel_format_t pixfmt)
{
	switch (pixfmt) {
	case PIX_FMT_SBGGR8:
	case PIX_FMT_SGBRG8:
	case PIX_FMT_SGRBG8:
	case PIX_FMT_SRGGB8:
	case PIX_FMT_GREY:
		return 8;
	case PIX_FMT_SBGGR10P:
	case PIX_FMT_SGBRG10P:
	case PIX_FMT_SGRBG10P:
	case PIX_FMT_SRGGB10P:
	case PIX_FMT_Y10P:
		return 10;
	case PIX_FMT_SBGGR12P:
	case PIX_FMT_SGBRG12P:
	case PIX_FMT_SGRBG12P:
	case PIX_FMT_SRGGB12P:
	case PIX_FMT_Y12P:
	case PIX_FMT_NV12:
	case PIX_FMT_NV21:
	case PIX_FMT_YUV420:
	case PIX_FMT_YVU420:
		return 12;
	case PIX_FMT_SBGGR14P:
	case PIX_FMT_SGBRG14P:
	case PIX_FMT_SGRBG14P:
	case PIX_FMT_SRGGB14P:
	case PIX_FMT_Y14P:
		return 14;
	case PIX_FMT_RGB565:
	case PIX_FMT_YUYV:
	case PIX_FMT_YVYU:
	case PIX_FMT_UYVY:
	case PIX_FMT_VYUY:
	case PIX_FMT_SBGGR8P16:
	case PIX_FMT_SGBRG8P16:
	case PIX_FMT_SGRBG8P16:
	case PIX_FMT_SRGGB8P16:
	case PIX_FMT_SBGGR10:
	case PIX_FMT_SGBRG10:
	case PIX_FMT_SGRBG10:
	case PIX_FMT_SRGGB10:
	case PIX_FMT_SBGGR12:
	case PIX_FMT_SGBRG12:
	case PIX_FMT_SGRBG12:
	case PIX_FMT_SRGGB12:
	case PIX_FMT_SBGGR14:
	case PIX_FMT_SGBRG14:
	case PIX_FMT_SGRBG14:
	case PIX_FMT_SRGGB14:
	case PIX_FMT_SBGGR16:
	case PIX_FMT_SGBRG16:
	case PIX_FMT_SGRBG16:
	case PIX_FMT_SRGGB16:
	case PIX_FMT_Y8P16:
	case PIX_FMT_Y10:
	case PIX_FMT_Y12:
	case PIX_FMT_Y14:
	case PIX_FMT_Y16:
	case PIX_FMT_NV16:
	case PIX_FMT_NV61:
		return 16;
	case PIX_FMT_BGR24:
	case PIX_FMT_RGB24:
	case PIX_FMT_NV24:
	case PIX_FMT_NV42:
	case PIX_FMT_YUV24:
		return 24;
	case PIX_FMT_XYUV32:
	case PIX_FMT_ARGB32:
	case PIX_FMT_ABGR32:
	case PIX_FMT_RGBA32:
	case PIX_FMT_BGRA32:
	case PIX_FMT_XRGB32:
	case PIX_FMT_XBGR32:
	case PIX_FMT_RGBX32:
	case PIX_FMT_BGRX32:
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

#endif /* ZEPHYR_INCLUDE_MEDIA_FORMATS_MISC_H */
