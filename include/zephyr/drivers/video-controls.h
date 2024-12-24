/*
 * Copyright (c) 2019 Linaro Limited.
 * Copyright (c) 2024 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_VIDEO_CONTROLS_H_
#define ZEPHYR_INCLUDE_VIDEO_CONTROLS_H_

/**
 * @file
 *
 * @brief Public APIs for Video.
 */

/**
 * @brief Video controls
 * @defgroup video_controls Video Controls
 * @ingroup io_interfaces
 *
 * The Video control IDs (CIDs) are introduced with the same name as
 * Linux V4L2 subsystem and under the same class. This facilitates
 * inter-operability and debugging devices end-to-end across Linux and
 * Zephyr.
 *
 * This list is maintained compatible to the Linux kernel definitions in
 * @c linux/include/uapi/linux/v4l2-controls.h
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name Base class control IDs
 * @{
 */
#define VIDEO_CID_BASE 0x00980900

/** Amount of perceived light of the image, the luma (Y') value. */
#define VIDEO_CID_BRIGHTNESS (VIDEO_CID_BASE + 0)

/** Amount of difference between the bright colors and dark colors. */
#define VIDEO_CID_CONTRAST (VIDEO_CID_BASE + 1)

/** Colorfulness of the image while preserving its brightness */
#define VIDEO_CID_SATURATION (VIDEO_CID_BASE + 2)

/** Shift in the tint of every colors, clockwise in a RGB color wheel */
#define VIDEO_CID_HUE (VIDEO_CID_BASE + 3)

/** Amount of time an image sensor is exposed to light, affecting the brightness */
#define VIDEO_CID_EXPOSURE (VIDEO_CID_BASE + 17)

/** Enable/Disable the autogain */
#define VIDEO_CID_AUTOGAIN (VIDEO_CID_BASE + 18)

/** Amount of amplification performed to each pixel electrical signal, affecting the brightness */
#define VIDEO_CID_GAIN (VIDEO_CID_BASE + 19)

/** Flip the image horizontally: the left side becomes the right side */
#define VIDEO_CID_HFLIP (VIDEO_CID_BASE + 20)

/** Flip the image vertically: the top side becomes the bottom side */
#define VIDEO_CID_VFLIP (VIDEO_CID_BASE + 21)

/** Frequency of the power line to compensate for, avoiding flicker due to artificial lighting */
#define VIDEO_CID_POWER_LINE_FREQUENCY (VIDEO_CID_BASE + 24)
enum video_power_line_frequency {
	VIDEO_CID_POWER_LINE_FREQUENCY_DISABLED = 0,
	VIDEO_CID_POWER_LINE_FREQUENCY_50HZ = 1,
	VIDEO_CID_POWER_LINE_FREQUENCY_60HZ = 2,
	VIDEO_CID_POWER_LINE_FREQUENCY_AUTO = 3,
};

/** Balance of colors in direction of blue (cold) or red (warm) */
#define VIDEO_CID_WHITE_BALANCE_TEMPERATURE (VIDEO_CID_BASE + 26)

/**
 * @}
 */

/**
 * @name Stateful codec controls IDs
 * @{
 */
#define VIDEO_CID_CODEC_CLASS_BASE 0x00990900

/**
 * @}
 */

/**
 * @name Camera class controls IDs
 * @{
 */
#define VIDEO_CID_CAMERA_CLASS_BASE 0x009a0900

/** Amount of optical zoom applied through to the camera optics */
#define VIDEO_CID_ZOOM_ABSOLUTE (VIDEO_CID_CAMERA_CLASS_BASE + 13)

/**
 * @}
 */

/**
 * @name Camera Flash class control IDs
 * @{
 */
#define VIDEO_CID_FLASH_CLASS_BASE 0x009c0900

/**
 * @}
 */

/**
 * @name JPEG class control IDs
 * @{
 */
#define VIDEO_CID_JPEG_CLASS_BASE 0x009d0900

/** Quality (Q) factor of the JPEG algorithm, also increasing the data size */
#define VIDEO_CID_JPEG_COMPRESSION_QUALITY (VIDEO_CID_JPEG_CLASS_BASE + 3)

/**
 * @}
 */

/**
 * @name Image Source class control IDs
 * @{
 */
#define VIDEO_CID_IMAGE_SOURCE_CLASS_BASE 0x009e0900

/**
 * @}
 */

/**
 * @name Image Processing class control IDs
 * @{
 */
#define VIDEO_CID_IMAGE_PROC_CLASS_BASE 0x009f0900

/** Pixel rate (pixels/second) in the device's pixel array. This control is read-only. */
#define VIDEO_CID_PIXEL_RATE (VIDEO_CID_IMAGE_PROC_CLASS_BASE + 2)

/** Selection of the type of test pattern to represent */
#define VIDEO_CID_TEST_PATTERN (VIDEO_CID_IMAGE_PROC_CLASS_BASE + 3)

/**
 * @}
 */

/**
 * @name Vendor-specific class control IDs
 * @{
 */
#define VIDEO_CID_PRIVATE_BASE 0x08000000

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_VIDEO_H_ */
