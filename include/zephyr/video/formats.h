/*
 * Copyright (c) 2019 Linaro Limited.
 * Copyright 2025 NXP
 * Copyright (c) 2025 STMicroelectronics
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup video_formats
 * @brief Compatibility header for video pixel formats.
 */

#ifndef ZEPHYR_INCLUDE_VIDEO_FORMATS_H
#define ZEPHYR_INCLUDE_VIDEO_FORMATS_H

/**
 * @brief Compatibility definitions for video formats.
 * @defgroup video_formats Video Formats
 * @since 4.4
 * @version 0.0.1
 * @ingroup video_api
 * @deprecated Use the pixel format definitions in <zephyr/media/pixel_format.h> instead.
 *
 * @{
 */

#include <zephyr/media/pixel_format.h>
#include <zephyr/media/pixel_format_misc.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Four-character-code uniquely identifying a pixel format. */
#define VIDEO_FOURCC(a, b, c, d) PIX_FMT_FOURCC((a), (b), (c), (d))

/** @brief Convert a four-character-string to a four-character-code. */
#define VIDEO_FOURCC_FROM_STR(str) PIX_FMT_FOURCC_FROM_STR(str)

/** @brief Convert a four-character-code to a four-character-string. */
#define VIDEO_FOURCC_TO_STR(fourcc) PIX_FMT_FOURCC_TO_STR(fourcc)

#define VIDEO_PIX_FMT_SBGGR8P16 PIX_FMT_SBGGR8P16
#define VIDEO_PIX_FMT_SGBRG8P16 PIX_FMT_SGBRG8P16
#define VIDEO_PIX_FMT_SGRBG8P16 PIX_FMT_SGRBG8P16
#define VIDEO_PIX_FMT_SRGGB8P16 PIX_FMT_SRGGB8P16
#define VIDEO_PIX_FMT_SBGGR10P PIX_FMT_SBGGR10P
#define VIDEO_PIX_FMT_SGBRG10P PIX_FMT_SGBRG10P
#define VIDEO_PIX_FMT_SGRBG10P PIX_FMT_SGRBG10P
#define VIDEO_PIX_FMT_SRGGB10P PIX_FMT_SRGGB10P
#define VIDEO_PIX_FMT_SBGGR12P PIX_FMT_SBGGR12P
#define VIDEO_PIX_FMT_SGBRG12P PIX_FMT_SGBRG12P
#define VIDEO_PIX_FMT_SGRBG12P PIX_FMT_SGRBG12P
#define VIDEO_PIX_FMT_SRGGB12P PIX_FMT_SRGGB12P
#define VIDEO_PIX_FMT_SBGGR14P PIX_FMT_SBGGR14P
#define VIDEO_PIX_FMT_SGBRG14P PIX_FMT_SGBRG14P
#define VIDEO_PIX_FMT_SGRBG14P PIX_FMT_SGRBG14P
#define VIDEO_PIX_FMT_SRGGB14P PIX_FMT_SRGGB14P
#define VIDEO_PIX_FMT_SBGGR8 PIX_FMT_SBGGR8
#define VIDEO_PIX_FMT_SGBRG8 PIX_FMT_SGBRG8
#define VIDEO_PIX_FMT_SGRBG8 PIX_FMT_SGRBG8
#define VIDEO_PIX_FMT_SRGGB8 PIX_FMT_SRGGB8
#define VIDEO_PIX_FMT_SBGGR10 PIX_FMT_SBGGR10
#define VIDEO_PIX_FMT_SGBRG10 PIX_FMT_SGBRG10
#define VIDEO_PIX_FMT_SGRBG10 PIX_FMT_SGRBG10
#define VIDEO_PIX_FMT_SRGGB10 PIX_FMT_SRGGB10
#define VIDEO_PIX_FMT_SBGGR12 PIX_FMT_SBGGR12
#define VIDEO_PIX_FMT_SGBRG12 PIX_FMT_SGBRG12
#define VIDEO_PIX_FMT_SGRBG12 PIX_FMT_SGRBG12
#define VIDEO_PIX_FMT_SRGGB12 PIX_FMT_SRGGB12
#define VIDEO_PIX_FMT_SBGGR14 PIX_FMT_SBGGR14
#define VIDEO_PIX_FMT_SGBRG14 PIX_FMT_SGBRG14
#define VIDEO_PIX_FMT_SGRBG14 PIX_FMT_SGRBG14
#define VIDEO_PIX_FMT_SRGGB14 PIX_FMT_SRGGB14
#define VIDEO_PIX_FMT_SBGGR16 PIX_FMT_SBGGR16
#define VIDEO_PIX_FMT_SGBRG16 PIX_FMT_SGBRG16
#define VIDEO_PIX_FMT_SGRBG16 PIX_FMT_SGRBG16
#define VIDEO_PIX_FMT_SRGGB16 PIX_FMT_SRGGB16
#define VIDEO_PIX_FMT_GREY PIX_FMT_GREY
#define VIDEO_PIX_FMT_Y10 PIX_FMT_Y10
#define VIDEO_PIX_FMT_Y12 PIX_FMT_Y12
#define VIDEO_PIX_FMT_Y14 PIX_FMT_Y14
#define VIDEO_PIX_FMT_Y16 PIX_FMT_Y16
#define VIDEO_PIX_FMT_Y8P16 PIX_FMT_Y8P16
#define VIDEO_PIX_FMT_Y10P PIX_FMT_Y10P
#define VIDEO_PIX_FMT_Y12P PIX_FMT_Y12P
#define VIDEO_PIX_FMT_Y14P PIX_FMT_Y14P
#define VIDEO_PIX_FMT_RGB565X PIX_FMT_RGB565X
#define VIDEO_PIX_FMT_RGB565 PIX_FMT_RGB565
#define VIDEO_PIX_FMT_BGR24 PIX_FMT_BGR24
#define VIDEO_PIX_FMT_RGB24 PIX_FMT_RGB24
#define VIDEO_PIX_FMT_ARGB32 PIX_FMT_ARGB32
#define VIDEO_PIX_FMT_BGRA32 PIX_FMT_BGRA32
#define VIDEO_PIX_FMT_RGBA32 PIX_FMT_RGBA32
#define VIDEO_PIX_FMT_ABGR32 PIX_FMT_ABGR32
#define VIDEO_PIX_FMT_XRGB32 PIX_FMT_XRGB32
#define VIDEO_PIX_FMT_XBGR32 PIX_FMT_XBGR32
#define VIDEO_PIX_FMT_BGRX32 PIX_FMT_BGRX32
#define VIDEO_PIX_FMT_RGBX32 PIX_FMT_RGBX32
#define VIDEO_PIX_FMT_YUYV PIX_FMT_YUYV
#define VIDEO_PIX_FMT_YVYU PIX_FMT_YVYU
#define VIDEO_PIX_FMT_VYUY PIX_FMT_VYUY
#define VIDEO_PIX_FMT_UYVY PIX_FMT_UYVY
#define VIDEO_PIX_FMT_XYUV32 PIX_FMT_XYUV32
#define VIDEO_PIX_FMT_YUV24 PIX_FMT_YUV24
#define VIDEO_PIX_FMT_NV12 PIX_FMT_NV12
#define VIDEO_PIX_FMT_NV21 PIX_FMT_NV21
#define VIDEO_PIX_FMT_NV16 PIX_FMT_NV16
#define VIDEO_PIX_FMT_NV61 PIX_FMT_NV61
#define VIDEO_PIX_FMT_NV24 PIX_FMT_NV24
#define VIDEO_PIX_FMT_NV42 PIX_FMT_NV42
#define VIDEO_PIX_FMT_YUV420 PIX_FMT_YUV420
#define VIDEO_PIX_FMT_YVU420 PIX_FMT_YVU420
#define VIDEO_PIX_FMT_JPEG PIX_FMT_JPEG
#define VIDEO_PIX_FMT_H264 PIX_FMT_H264
#define VIDEO_PIX_FMT_H264_NO_SC PIX_FMT_H264_NO_SC
#define VIDEO_PIX_FMT_PNG PIX_FMT_PNG

/** @brief Test if a fourcc is a grayscale format. */
#define VIDEO_FMT_IS_GRAYSCALE(pixfmt) PIX_FMT_IS_GRAYSCALE(pixfmt)

/** @brief Test if a fourcc is a Bayer format. */
#define VIDEO_FMT_IS_BAYER(pixfmt) PIX_FMT_IS_BAYER(pixfmt)

/** @brief Test if a fourcc is an RGB format (Bayer excluded). */
#define VIDEO_FMT_IS_RGB(pixfmt) PIX_FMT_IS_RGB(pixfmt)

/** @brief Test if a fourcc is a YUV format. */
#define VIDEO_FMT_IS_YUV(pixfmt) PIX_FMT_IS_YUV(pixfmt)

/** @brief Test if a fourcc is any of the MIPI-packed formats. */
#define VIDEO_FMT_IS_MIPI_PACKED(pixfmt) PIX_FMT_IS_MIPI_PACKED(pixfmt)

/** @brief Test if a fourcc is any of the padded formats. */
#define VIDEO_FMT_IS_PADDED(pixfmt) PIX_FMT_IS_PADDED(pixfmt)

/** @brief Test if a fourcc is any of the semi-planar formats. */
#define VIDEO_FMT_IS_SEMI_PLANAR(pixfmt) PIX_FMT_IS_SEMI_PLANAR(pixfmt)

/** @brief Test if a fourcc is any of the full-planar formats. */
#define VIDEO_FMT_IS_FULL_PLANAR(pixfmt) PIX_FMT_IS_FULL_PLANAR(pixfmt)

/**
 * @brief Get the number of bits per pixel of a pixel format.
 *
 * @param pixfmt FourCC pixel format value.
 * @return Bit size of one pixel, or 0 if it is unknown or variable.
 */
static inline unsigned int video_bits_per_pixel(uint32_t pixfmt)
{
	return pix_fmt_bpp(pixfmt);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_VIDEO_FORMATS_H */
