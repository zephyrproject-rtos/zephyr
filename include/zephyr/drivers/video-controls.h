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

/**
 * @name Control classes
 * @{
 */
#define VIDEO_CTRL_CLASS_GENERIC	0x00000000	/**< Generic class controls */
#define VIDEO_CTRL_CLASS_CAMERA		0x00010000	/**< Camera class controls */
#define VIDEO_CTRL_CLASS_MPEG		0x00020000	/**< MPEG-compression controls */
#define VIDEO_CTRL_CLASS_JPEG		0x00030000	/**< JPEG-compression controls */
#define VIDEO_CTRL_CLASS_VENDOR		0xFFFF0000	/**< Vendor-specific class controls */
/**
 * @}
 */

/**
 * @name Control Get operations
 *
 * Extra flags for video controls to inquire about the dimensions of an existing
 * control: the minimum, maximum, or default value.
 *
 * For instance, OR-ing @c VIDEO_CID_CAMERA_EXPOSURE and @c VIDEO_CTRL_GET_MAX
 * permits to query the maximum exposure time instead of the current exposure
 * time.
 *
 * If no Control Get flag is added to a CID, the behavior is to fetch the current
 * value as with @ref VIDEO_CTRL_GET_CUR.
 * These must only be used along with the @ref video_get_ctrl() API.
 *
 * @{
 */
#define VIDEO_CTRL_GET_CUR		0x00000000	/**< Get the current value */
#define VIDEO_CTRL_GET_MIN		0x00001000	/**< Get the minimum value */
#define VIDEO_CTRL_GET_MAX		0x00002000	/**< Get the maximum value */
#define VIDEO_CTRL_GET_DEF		0x00003000	/**< Get the default value */
#define VIDEO_CTRL_GET_MASK		0x0000f000	/**< Mask for get operations */
/**
 * @}
 */

/**
 * @name Generic class control IDs
 * @{
 */
/** Mirror the picture horizontally */
#define VIDEO_CID_HFLIP			(VIDEO_CTRL_CLASS_GENERIC + 0)
/** Mirror the picture vertically */
#define VIDEO_CID_VFLIP			(VIDEO_CTRL_CLASS_GENERIC + 1)
/**
 * @}
 */

/**
 * @name Camera class control IDs
 * @{
 */
#define VIDEO_CID_CAMERA_EXPOSURE	(VIDEO_CTRL_CLASS_CAMERA + 0)
#define VIDEO_CID_CAMERA_GAIN		(VIDEO_CTRL_CLASS_CAMERA + 1)
#define VIDEO_CID_CAMERA_ZOOM		(VIDEO_CTRL_CLASS_CAMERA + 2)
#define VIDEO_CID_CAMERA_BRIGHTNESS	(VIDEO_CTRL_CLASS_CAMERA + 3)
#define VIDEO_CID_CAMERA_SATURATION	(VIDEO_CTRL_CLASS_CAMERA + 4)
#define VIDEO_CID_CAMERA_WHITE_BAL	(VIDEO_CTRL_CLASS_CAMERA + 5)
#define VIDEO_CID_CAMERA_CONTRAST	(VIDEO_CTRL_CLASS_CAMERA + 6)
#define VIDEO_CID_CAMERA_COLORBAR	(VIDEO_CTRL_CLASS_CAMERA + 7)
#define VIDEO_CID_CAMERA_QUALITY	(VIDEO_CTRL_CLASS_CAMERA + 8)
/**
 * @}
 */

#ifdef __cplusplus
}
#endif

/* Controls */


/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_VIDEO_H_ */
