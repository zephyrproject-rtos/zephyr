/*
 * Copyright (c) 2019 Linaro Limited.
 * Copyright (c) 2024 tinyVision.ai Inc.
 * Copyright 2025 NXP
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

#include <stdint.h>

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

/** Automatic gain control */
#define VIDEO_CID_AUTOGAIN (VIDEO_CID_BASE + 18)

/** Gain control. Most devices control only digital gain with this control.
 * Devices that recognise the difference between digital and analogue gain use
 * VIDEO_CID_DIGITAL_GAIN and VIDEO_CID_ANALOGUE_GAIN.
 */
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

/** Last base CID + 1 */
#define VIDEO_CID_LASTP1 (VIDEO_CID_BASE + 44)

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

/** Adjustments of exposure time and/or iris aperture. */
#define VIDEO_CID_EXPOSURE_AUTO (VIDEO_CID_CAMERA_CLASS_BASE + 1)
enum video_exposure_auto_type {
	VIDEO_EXPOSURE_AUTO = 0,
	VIDEO_EXPOSURE_MANUAL = 1,
	VIDEO_EXPOSURE_SHUTTER_PRIORITY = 2,
	VIDEO_EXPOSURE_APERTURE_PRIORITY = 3
};

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

/** Analogue gain control. */
#define VIDEO_CID_ANALOGUE_GAIN (VIDEO_CID_IMAGE_SOURCE_CLASS_BASE + 3)

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

/**
 * @name Query flags, to be ORed with the control ID
 * @{
 */
#define VIDEO_CTRL_FLAG_NEXT_CTRL 0x80000000

/**
 * @}
 */

/**
 * @name Public video control structures
 * @{
 */

/**
 * @struct video_control
 * @brief Video control structure
 *
 * Used to get/set a video control.
 * @see video_ctrl for the struct used in the driver implementation
 */
struct video_control {
	/** control id */
	uint32_t id;
	/** control value */
	union {
		int32_t val;
		int64_t val64;
	};
};

/**
 * @struct video_control_range
 * @brief Video control range structure
 *
 * Describe range of a control including min, max, step and default values
 */
struct video_ctrl_range {
	/** control minimum value, inclusive */
	union {
		int32_t min;
		int64_t min64;
	};
	/** control maximum value, inclusive */
	union {
		int32_t max;
		int64_t max64;
	};
	/** control value step */
	union {
		int32_t step;
		int64_t step64;
	};
	/** control default value for VIDEO_CTRL_TYPE_INTEGER, _BOOLEAN, _MENU or
	 * _INTEGER_MENU, not valid for other types
	 */
	union {
		int32_t def;
		int64_t def64;
	};
};

/**
 * @struct video_control_query
 * @brief Video control query structure
 *
 * Used to query information about a control.
 */
struct video_ctrl_query {
	/** control id */
	uint32_t id;
	/** control type */
	uint32_t type;
	/** control name */
	const char *name;
	/** control flags */
	uint32_t flags;
	/** control range */
	struct video_ctrl_range range;
	/** menu if control is of menu type */
	const char *const *menu;
};

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
