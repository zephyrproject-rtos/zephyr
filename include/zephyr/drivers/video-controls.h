/**
 * @file
 *
 * @brief Public APIs for Video.
 */

/*
 * Copyright (c) 2019 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_VIDEO_CONTROLS_H_
#define ZEPHYR_INCLUDE_VIDEO_CONTROLS_H_

/**
 * @brief Video controls
 * @defgroup video_controls Video Controls
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/device.h>
#include <stddef.h>
#include <zephyr/kernel.h>

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Control classes */
#define VIDEO_CTRL_CLASS_GENERIC	0x00000000	/* Generic class controls */
#define VIDEO_CTRL_CLASS_CAMERA		0x00010000	/* Camera class controls */
#define VIDEO_CTRL_CLASS_MPEG		0x00020000	/* MPEG-compression controls */
#define VIDEO_CTRL_CLASS_JPEG		0x00030000	/* JPEG-compression controls */
#define VIDEO_CTRL_CLASS_VENDOR		0xFFFF0000	/* Vendor-specific class controls */

/* Generic class control IDs */
#define VIDEO_CID_HFLIP			(VIDEO_CTRL_CLASS_GENERIC + 0) /* Mirror the picture horizontally */
#define VIDEO_CID_VFLIP			(VIDEO_CTRL_CLASS_GENERIC + 1) /* Mirror the picture vertically */

/* Camera class control IDs */
#define VIDEO_CID_CAMERA_EXPOSURE	(VIDEO_CTRL_CLASS_CAMERA + 0)
#define VIDEO_CID_CAMERA_GAIN		(VIDEO_CTRL_CLASS_CAMERA + 1)
#define VIDEO_CID_CAMERA_ZOOM		(VIDEO_CTRL_CLASS_CAMERA + 2)
#define VIDEO_CID_CAMERA_BRIGHTNESS	(VIDEO_CTRL_CLASS_CAMERA + 3)
#define VIDEO_CID_CAMERA_SATURATION	(VIDEO_CTRL_CLASS_CAMERA + 4)
#define VIDEO_CID_CAMERA_WHITE_BAL	(VIDEO_CTRL_CLASS_CAMERA + 5)
#define VIDEO_CID_CAMERA_CONTRAST	(VIDEO_CTRL_CLASS_CAMERA + 6)
#define VIDEO_CID_CAMERA_COLORBAR	(VIDEO_CTRL_CLASS_CAMERA + 7)
#define VIDEO_CID_CAMERA_QUALITY	(VIDEO_CTRL_CLASS_CAMERA + 8)

#ifdef __cplusplus
}
#endif

/* Controls */


/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_VIDEO_H_ */
