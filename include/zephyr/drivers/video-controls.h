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
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name Control classes
 *
 * List of control classes, to group related controls together in blocks.
 *
 * This list is kept identical to the Linux kernel definitions.
 *
 * @{
 */
#define VIDEO_CID_BASE				0x00980900 /**< Base class controls */
#define VIDEO_CID_CODEC_CLASS_BASE		0x00990900 /**< Stateful codec controls */
#define VIDEO_CID_CAMERA_CLASS_BASE		0x009a0900 /**< Camera class controls */
#define VIDEO_CID_FLASH_CLASS_BASE		0x009c0900 /**< Camera flash controls */
#define VIDEO_CID_JPEG_CLASS_BASE		0x009d0900 /**< JPEG-compression controls */
#define VIDEO_CID_IMAGE_SOURCE_CLASS_BASE	0x009e0900 /**< Image source controls */
#define VIDEO_CID_IMAGE_PROC_CLASS_BASE		0x009f0900 /**< Image processing controls */
#define VIDEO_CID_PRIVATE_BASE			0x08000000 /**< Vendor-specific class controls */
/**
 * @}
 */

/**
 * @name Generic class control IDs
 * @{
 */
/** Mirror the picture horizontally */
#define VIDEO_CID_HFLIP			(VIDEO_CID_BASE + 20)
/** Mirror the picture vertically */
#define VIDEO_CID_VFLIP			(VIDEO_CID_BASE + 21)
/** Power line frequency (enum) filter to avoid flicker */
#define VIDEO_CID_POWER_LINE_FREQUENCY	(VIDEO_CID_BASE + 24)
/** Pixel rate (pixels/second) in the device's pixel array. This control is read-only. */
#define VIDEO_CID_PIXEL_RATE		(VIDEO_CID_IMAGE_PROC_CLASS_BASE + 2)
enum video_power_line_frequency {
	VIDEO_CID_POWER_LINE_FREQUENCY_DISABLED = 0,
	VIDEO_CID_POWER_LINE_FREQUENCY_50HZ = 1,
	VIDEO_CID_POWER_LINE_FREQUENCY_60HZ = 2,
	VIDEO_CID_POWER_LINE_FREQUENCY_AUTO = 3,
};
/**
 * @}
 */

/**
 * @name Camera class control IDs
 * @{
 */
#define VIDEO_CID_CAMERA_EXPOSURE	(VIDEO_CID_BASE + 17)
#define VIDEO_CID_CAMERA_GAIN		(VIDEO_CID_BASE + 19)
#define VIDEO_CID_CAMERA_ZOOM		(VIDEO_CID_CAMERA_CLASS_BASE + 13)
#define VIDEO_CID_CAMERA_BRIGHTNESS	(VIDEO_CID_BASE + 0)
#define VIDEO_CID_CAMERA_SATURATION	(VIDEO_CID_BASE + 2)
#define VIDEO_CID_CAMERA_WHITE_BAL	(VIDEO_CID_BASE + 26)
#define VIDEO_CID_CAMERA_CONTRAST	(VIDEO_CID_BASE + 1)
#define VIDEO_CID_CAMERA_COLORBAR	(VIDEO_CID_IMAGE_PROC_CLASS_BASE + 3)
#define VIDEO_CID_CAMERA_HUE		(VIDEO_CID_BASE + 3)
#define VIDEO_CID_CAMERA_QUALITY	(VIDEO_CID_JPEG_CLASS_BASE + 3)
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
