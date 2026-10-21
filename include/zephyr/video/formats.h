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
 * @deprecated Use the pixel format definitions in <zephyr/media/formats.h> instead.
 *
 * @{
 */

#include <zephyr/media/formats.h>
#include <zephyr/media/formats_misc.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Four-character-code uniquely identifying a pixel format. */
#define VIDEO_FOURCC(a, b, c, d) PIXFMT_FOURCC(a, b, c, d)

/** @brief Convert a four-character-string to a four-character-code. */
#define VIDEO_FOURCC_FROM_STR(str) PIXFMT_FOURCC_FROM_STR(str)

/** @brief Convert a four-character-code to a four-character-string. */
#define VIDEO_FOURCC_TO_STR(fourcc) PIXFMT_FOURCC_TO_STR(fourcc)

#define VIDEO_PIX_FMT_SBGGR8P16 PIXFMT_SBGGR8P16
#define VIDEO_PIX_FMT_SGBRG8P16 PIXFMT_SGBRG8P16
#define VIDEO_PIX_FMT_SGRBG8P16 PIXFMT_SGRBG8P16
#define VIDEO_PIX_FMT_SRGGB8P16 PIXFMT_SRGGB8P16
#define VIDEO_PIX_FMT_SBGGR10P PIXFMT_SBGGR10P
#define VIDEO_PIX_FMT_SGBRG10P PIXFMT_SGBRG10P
#define VIDEO_PIX_FMT_SGRBG10P PIXFMT_SGRBG10P
#define VIDEO_PIX_FMT_SRGGB10P PIXFMT_SRGGB10P
#define VIDEO_PIX_FMT_SBGGR12P PIXFMT_SBGGR12P
#define VIDEO_PIX_FMT_SGBRG12P PIXFMT_SGBRG12P
#define VIDEO_PIX_FMT_SGRBG12P PIXFMT_SGRBG12P
#define VIDEO_PIX_FMT_SRGGB12P PIXFMT_SRGGB12P
#define VIDEO_PIX_FMT_SBGGR14P PIXFMT_SBGGR14P
#define VIDEO_PIX_FMT_SGBRG14P PIXFMT_SGBRG14P
#define VIDEO_PIX_FMT_SGRBG14P PIXFMT_SGRBG14P
#define VIDEO_PIX_FMT_SRGGB14P PIXFMT_SRGGB14P
#define VIDEO_PIX_FMT_SBGGR8 PIXFMT_SBGGR8
#define VIDEO_PIX_FMT_SGBRG8 PIXFMT_SGBRG8
#define VIDEO_PIX_FMT_SGRBG8 PIXFMT_SGRBG8
#define VIDEO_PIX_FMT_SRGGB8 PIXFMT_SRGGB8
#define VIDEO_PIX_FMT_SBGGR10 PIXFMT_SBGGR10
#define VIDEO_PIX_FMT_SGBRG10 PIXFMT_SGBRG10
#define VIDEO_PIX_FMT_SGRBG10 PIXFMT_SGRBG10
#define VIDEO_PIX_FMT_SRGGB10 PIXFMT_SRGGB10
#define VIDEO_PIX_FMT_SBGGR12 PIXFMT_SBGGR12
#define VIDEO_PIX_FMT_SGBRG12 PIXFMT_SGBRG12
#define VIDEO_PIX_FMT_SGRBG12 PIXFMT_SGRBG12
#define VIDEO_PIX_FMT_SRGGB12 PIXFMT_SRGGB12
#define VIDEO_PIX_FMT_SBGGR14 PIXFMT_SBGGR14
#define VIDEO_PIX_FMT_SGBRG14 PIXFMT_SGBRG14
#define VIDEO_PIX_FMT_SGRBG14 PIXFMT_SGRBG14
#define VIDEO_PIX_FMT_SRGGB14 PIXFMT_SRGGB14
#define VIDEO_PIX_FMT_SBGGR16 PIXFMT_SBGGR16
#define VIDEO_PIX_FMT_SGBRG16 PIXFMT_SGBRG16
#define VIDEO_PIX_FMT_SGRBG16 PIXFMT_SGRBG16
#define VIDEO_PIX_FMT_SRGGB16 PIXFMT_SRGGB16
#define VIDEO_PIX_FMT_GREY PIXFMT_GREY
#define VIDEO_PIX_FMT_Y10 PIXFMT_Y10
#define VIDEO_PIX_FMT_Y12 PIXFMT_Y12
#define VIDEO_PIX_FMT_Y14 PIXFMT_Y14
#define VIDEO_PIX_FMT_Y16 PIXFMT_Y16
#define VIDEO_PIX_FMT_Y8P16 PIXFMT_Y8P16
#define VIDEO_PIX_FMT_Y10P PIXFMT_Y10P
#define VIDEO_PIX_FMT_Y12P PIXFMT_Y12P
#define VIDEO_PIX_FMT_Y14P PIXFMT_Y14P
#define VIDEO_PIX_FMT_RGB565X PIXFMT_RGB565X
#define VIDEO_PIX_FMT_RGB565 PIXFMT_RGB565
#define VIDEO_PIX_FMT_BGR24 PIXFMT_BGR24
#define VIDEO_PIX_FMT_RGB24 PIXFMT_RGB24
#define VIDEO_PIX_FMT_ARGB32 PIXFMT_ARGB32
#define VIDEO_PIX_FMT_BGRA32 PIXFMT_BGRA32
#define VIDEO_PIX_FMT_RGBA32 PIXFMT_RGBA32
#define VIDEO_PIX_FMT_ABGR32 PIXFMT_ABGR32
#define VIDEO_PIX_FMT_XRGB32 PIXFMT_XRGB32
#define VIDEO_PIX_FMT_XBGR32 PIXFMT_XBGR32
#define VIDEO_PIX_FMT_BGRX32 PIXFMT_BGRX32
#define VIDEO_PIX_FMT_RGBX32 PIXFMT_RGBX32
#define VIDEO_PIX_FMT_YUYV PIXFMT_YUYV
#define VIDEO_PIX_FMT_YVYU PIXFMT_YVYU
#define VIDEO_PIX_FMT_VYUY PIXFMT_VYUY
#define VIDEO_PIX_FMT_UYVY PIXFMT_UYVY
#define VIDEO_PIX_FMT_XYUV32 PIXFMT_XYUV32
#define VIDEO_PIX_FMT_YUV24 PIXFMT_YUV24
#define VIDEO_PIX_FMT_NV12 PIXFMT_NV12
#define VIDEO_PIX_FMT_NV21 PIXFMT_NV21
#define VIDEO_PIX_FMT_NV16 PIXFMT_NV16
#define VIDEO_PIX_FMT_NV61 PIXFMT_NV61
#define VIDEO_PIX_FMT_NV24 PIXFMT_NV24
#define VIDEO_PIX_FMT_NV42 PIXFMT_NV42
#define VIDEO_PIX_FMT_YUV420 PIXFMT_YUV420
#define VIDEO_PIX_FMT_YVU420 PIXFMT_YVU420
#define VIDEO_PIX_FMT_JPEG PIXFMT_JPEG
#define VIDEO_PIX_FMT_H264 PIXFMT_H264
#define VIDEO_PIX_FMT_H264_NO_SC PIXFMT_H264_NO_SC
#define VIDEO_PIX_FMT_PNG PIXFMT_PNG

/** @brief Test if a fourcc is a grayscale format. */
#define VIDEO_FMT_IS_GRAYSCALE(pixfmt) PIXFMT_IS_GRAYSCALE(pixfmt)

/** @brief Test if a fourcc is a Bayer format. */
#define VIDEO_FMT_IS_BAYER(pixfmt) PIXFMT_IS_BAYER(pixfmt)

/** @brief Test if a fourcc is an RGB format (Bayer excluded). */
#define VIDEO_FMT_IS_RGB(pixfmt) PIXFMT_IS_RGB(pixfmt)

/** @brief Test if a fourcc is a YUV format. */
#define VIDEO_FMT_IS_YUV(pixfmt) PIXFMT_IS_YUV(pixfmt)

/** @brief Test if a fourcc is any of the MIPI-packed formats. */
#define VIDEO_FMT_IS_MIPI_PACKED(pixfmt) PIXFMT_IS_MIPI_PACKED(pixfmt)

/** @brief Test if a fourcc is any of the padded formats. */
#define VIDEO_FMT_IS_PADDED(pixfmt) PIXFMT_IS_PADDED(pixfmt)

/** @brief Test if a fourcc is any of the semi-planar formats. */
#define VIDEO_FMT_IS_SEMI_PLANAR(pixfmt) PIXFMT_IS_SEMI_PLANAR(pixfmt)

/** @brief Test if a fourcc is any of the full-planar formats. */
#define VIDEO_FMT_IS_FULL_PLANAR(pixfmt) PIXFMT_IS_FULL_PLANAR(pixfmt)

/**
 * @brief Get the number of bits per pixel of a pixel format.
 *
 * @param pixfmt FourCC pixel format value.
 * @return Bit size of one pixel, or 0 if it is unknown or variable.
 */
static inline unsigned int video_bits_per_pixel(uint32_t pixfmt)
{
	return pixfmt_bpp(pixfmt);
}

#ifdef __cplusplus
}
#endif

/** @}
 */

#endif /* ZEPHYR_INCLUDE_VIDEO_FORMATS_H */
