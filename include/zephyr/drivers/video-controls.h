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
 * @ingroup video_controls
 * @brief Main header file for video controls driver API.
 */

/**
 * @brief Video controls
 * @defgroup video_controls Video Controls
 * @ingroup video_interface
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

/** Picture brightness, or more precisely, the black level. */
#define VIDEO_CID_BRIGHTNESS (VIDEO_CID_BASE + 0)

/** Picture contrast or luma gain. */
#define VIDEO_CID_CONTRAST (VIDEO_CID_BASE + 1)

/** Picture color saturation or chroma gain. */
#define VIDEO_CID_SATURATION (VIDEO_CID_BASE + 2)

/** Hue or color balance. */
#define VIDEO_CID_HUE (VIDEO_CID_BASE + 3)

/** Automatic white balance (cameras). */
#define VIDEO_CID_AUTO_WHITE_BALANCE (VIDEO_CID_BASE + 12)

/** Red chroma balance, as a ratio to the green channel. */
#define VIDEO_CID_RED_BALANCE (VIDEO_CID_BASE + 14)

/** Blue chroma balance, as a ratio to the green channel. */
#define VIDEO_CID_BLUE_BALANCE (VIDEO_CID_BASE + 15)

/** Gamma adjust. */
#define VIDEO_CID_GAMMA (VIDEO_CID_BASE + 16)

/** Image sensor exposure time. */
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

/** Frequency of the power line to compensate for.
 * Avoids flicker due to artificial lighting.
 */
#define VIDEO_CID_POWER_LINE_FREQUENCY (VIDEO_CID_BASE + 24)

/**
 * @brief Video power line frequency
 */
enum video_power_line_frequency {
	VIDEO_CID_POWER_LINE_FREQUENCY_DISABLED = 0, /**< Power line frequency disabled. */
	VIDEO_CID_POWER_LINE_FREQUENCY_50HZ = 1,     /**< 50 Hz power line frequency. */
	VIDEO_CID_POWER_LINE_FREQUENCY_60HZ = 2,     /**< 60 Hz power line frequency. */
	VIDEO_CID_POWER_LINE_FREQUENCY_AUTO = 3,     /**< Automatic power line frequency. */
};

/** Enables automatic hue control by the device.
 * Setting @ref VIDEO_CID_HUE while automatic hue control is enabled is undefined.
 * Drivers should ignore such request.
 */
#define VIDEO_CID_HUE_AUTO (VIDEO_CID_BASE + 25)

/** White balance settings as a color temperature in Kelvin.
 * A driver should have a minimum range of 2800 (incandescent) to 6500 (daylight).
 */
#define VIDEO_CID_WHITE_BALANCE_TEMPERATURE (VIDEO_CID_BASE + 26)

/** Adjusts the sharpness filters in a camera.
 * The minimum value disables the filters, higher values give a sharper picture.
 */
#define VIDEO_CID_SHARPNESS (VIDEO_CID_BASE + 27)

/** Adjusts the backlight compensation in a camera.
 * The minimum value disables backlight compensation.
 */
#define VIDEO_CID_BACKLIGHT_COMPENSATION (VIDEO_CID_BASE + 28)

/** Selects a color effect. */
#define VIDEO_CID_COLORFX (VIDEO_CID_BASE + 31)

/**
 * @brief Video color effects
 */
enum video_colorfx {
	VIDEO_COLORFX_NONE = 0,          /**< No color effect. */
	VIDEO_COLORFX_BW = 1,            /**< Black and white effect. */
	VIDEO_COLORFX_SEPIA = 2,         /**< Sepia tone effect. */
	VIDEO_COLORFX_NEGATIVE = 3,      /**< Negative image effect. */
	VIDEO_COLORFX_EMBOSS = 4,        /**< Emboss effect. */
	VIDEO_COLORFX_SKETCH = 5,        /**< Sketch/drawing effect. */
	VIDEO_COLORFX_SKY_BLUE = 6,      /**< Sky blue tint effect. */
	VIDEO_COLORFX_GRASS_GREEN = 7,   /**< Grass green tint effect. */
	VIDEO_COLORFX_SKIN_WHITEN = 8,   /**< Skin whitening effect. */
	VIDEO_COLORFX_VIVID = 9,         /**< Vivid/bright color effect. */
	VIDEO_COLORFX_AQUA = 10,         /**< Aqua (blue-green) effect. */
	VIDEO_COLORFX_ART_FREEZE = 11,   /**< Art freeze/stylized effect. */
	VIDEO_COLORFX_SILHOUETTE = 12,   /**< Silhouette effect. */
	VIDEO_COLORFX_SOLARIZATION = 13, /**< Solarization effect. */
	VIDEO_COLORFX_ANTIQUE = 14,      /**< Antique/vintage effect. */
};

/** Enables automatic brightness. */
#define VIDEO_CID_AUTOBRIGHTNESS (VIDEO_CID_BASE + 32)

/** Switch the band-stop filter of a camera sensor on or off, or specify its strength.
 * Such band-stop filters can be used, for example, to filter out the fluorescent light component.
 */
#define VIDEO_CID_BAND_STOP_FILTER (VIDEO_CID_BASE + 33)

/** Rotate the image by a given angle, e.g. 90, 180, 270 degree. The application will be then
 * responsible for setting the new width and height of the image using video_set_fmt() if needed.
 */
#define VIDEO_CID_ROTATE (VIDEO_CID_BASE + 34)

/** Sets the alpha color component.
 * Some devices produce data with a user-controllable alpha component. Set the value applied to
 * the alpha channel of every pixel produced.
 */
#define VIDEO_CID_ALPHA_COMPONENT (VIDEO_CID_BASE + 41)

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

/** Enables automatic adjustments of the exposure time and/or iris aperture.
 * Manual exposure or iris changes when it is not @ref VIDEO_EXPOSURE_MANUAL is undefined.
 * Drivers should ignore such requests.
 */
#define VIDEO_CID_EXPOSURE_AUTO (VIDEO_CID_CAMERA_CLASS_BASE + 1)

/**
 * @brief Video exposure type
 */
enum video_exposure_type {
	VIDEO_EXPOSURE_AUTO = 0,             /**< Automatic exposure. */
	VIDEO_EXPOSURE_MANUAL = 1,           /**< Manual exposure. */
	VIDEO_EXPOSURE_SHUTTER_PRIORITY = 2, /**< Shutter priority. */
	VIDEO_EXPOSURE_APERTURE_PRIORITY = 3 /**< Aperture priority. */
};

/** Determines the exposure time of the camera sensor.
 * The exposure time is limited by the frame in terval. Drivers should interpret the values as
 * 100 µs units, where the value 1 stands for 1/10000th of a second, 10000 for 1 second and 100000
 * for 10 seconds.
 */
#define VIDEO_CID_EXPOSURE_ABSOLUTE (VIDEO_CID_CAMERA_CLASS_BASE + 2)

/** Whether the device may dynamically vary the frame rate under the effect of auto-exposure
 * Applicable when @ref VIDEO_CID_EXPOSURE_AUTO is set to @ref VIDEO_EXPOSURE_AUTO or
 * @ref VIDEO_EXPOSURE_APERTURE_PRIORITY. Disabled by default: the frame rate must remain constant.
 */
#define VIDEO_CID_EXPOSURE_AUTO_PRIORITY (VIDEO_CID_CAMERA_CLASS_BASE + 3)

/** This write-only control turns the camera horizontally by the specified amount.
 * The unit is undefined. A positive value moves the camera to the right (clockwise when viewed
 * from above), a negative value to the left. A value of zero does not cause motion.
 */
#define VIDEO_CID_PAN_RELATIVE (VIDEO_CID_CAMERA_CLASS_BASE + 4)

/** This write-only control turns the camera vertically by the specified amount.
 * The unit is undefined. A positive value moves the camera up, a negative value down.
 * A value of zero does not cause motion.
 */
#define VIDEO_CID_TILT_RELATIVE (VIDEO_CID_CAMERA_CLASS_BASE + 5)

/** This control turns the camera horizontally to the specified position.
 * Positive values move the camera to the right (clockwise when viewed from above), negative
 * values to the left. Drivers should interpret the values as arc seconds, with valid values
 * between -180 * 3600 and +180 * 3600 inclusive.
 */
#define VIDEO_CID_PAN_ABSOLUTE (VIDEO_CID_CAMERA_CLASS_BASE + 8)

/** This control turns the camera vertically to the specified position.
 * Positive values move the camera up, negative values down. Drivers should interpret the values as
 * arc seconds, with valid values between -180 * 3600 and +180 * 3600 inclusive.
 */
#define VIDEO_CID_TILT_ABSOLUTE (VIDEO_CID_CAMERA_CLASS_BASE + 9)

/** This control sets the focal point of the camera to the specified position.
 * The unit is undefined. Positive values set the focus closer to the camera, negative values
 * towards infinity.
 */
#define VIDEO_CID_FOCUS_ABSOLUTE (VIDEO_CID_CAMERA_CLASS_BASE + 10)

/** This write-only control moves the focal point of the camera by the specified amount.
 * The unit is undefined. Positive values move the focus closer to the camera, negative values
 * towards infinity.
 */
#define VIDEO_CID_FOCUS_RELATIVE (VIDEO_CID_CAMERA_CLASS_BASE + 11)

/** Enables continuous automatic focus adjustments.
 * Manual focus adjustments while this control is on (set to 1) is undefined.
 * Drivers should ignore such requests.
 */
#define VIDEO_CID_FOCUS_AUTO (VIDEO_CID_CAMERA_CLASS_BASE + 12)

/** Specify the objective lens focal length as an absolute value.
 * The zoom unit is driver-specific and its value should be a positive integer.
 */
#define VIDEO_CID_ZOOM_ABSOLUTE (VIDEO_CID_CAMERA_CLASS_BASE + 13)

/** This write-only control sets the objective lens focal length relatively to the current value.
 * Positive values move the zoom lens group towards the telephoto direction, negative values
 * towards the wide-angle direction. The zoom unit is driver-specific.
 */
#define VIDEO_CID_ZOOM_RELATIVE (VIDEO_CID_CAMERA_CLASS_BASE + 14)

/** Start a continuous zoom movement.
 * Move the objective lens group at the specified speed until it reaches physical device limits or
 * until an explicit request to stop the movement. A positive value moves the zoom lens group
 * towards the telephoto direction. A value of zero stops the zoom lens group movement.
 * A negative value moves the zoom lens group towards the wide-angle direction.
 * The zoom speed unit is driver-specific.
 */
#define VIDEO_CID_ZOOM_CONTINUOUS (VIDEO_CID_CAMERA_CLASS_BASE + 15)

/** This control sets the camera's aperture to the specified value.
 * The unit is undefined. Larger values open the iris wider, smaller values close it.
 */
#define VIDEO_CID_IRIS_ABSOLUTE (VIDEO_CID_CAMERA_CLASS_BASE + 17)

/** This write-only control modifies the camera's aperture by the specified amount.
 * The unit is undefined. Positive values open the iris one step further, negative values close
 * it one step further.
 */
#define VIDEO_CID_IRIS_RELATIVE (VIDEO_CID_CAMERA_CLASS_BASE + 18)

/** Enables or disables the camera's wide dynamic range feature.
 * This feature allows to obtain clear images in situations where intensity of the illumination
 * varies significantly throughout the scene, i.e. there are simultaneously very dark and very
 * bright areas. It is most commonly realized in cameras by combining two subsequent frames with
 * different exposure times.
 */
#define VIDEO_CID_WIDE_DYNAMIC_RANGE (VIDEO_CID_CAMERA_CLASS_BASE + 21)

/**This control turns the camera horizontally at the specific speed.
 * The unit is undefined. A positive value moves the camera to the right (clockwise when viewed
 * from above), a negative value to the left. A value of zero stops the motion if one is in
 * progress and has no effect otherwise.
 */
#define VIDEO_CID_PAN_SPEED (VIDEO_CID_CAMERA_CLASS_BASE + 32)

/** This control turns the camera vertically at the specified speed.
 * The unit is undefined. A positive value moves the camera up, a negative value down.
 * A value of zero stops the motion if one is in progress and has no effect otherwise.
 */
#define VIDEO_CID_TILT_SPEED (VIDEO_CID_CAMERA_CLASS_BASE + 33)

/** This read-only control describes the camera position on the device
 * It by reports where the camera camera is installed, its mounting position on the device.
 * This control is particularly meaningful for devices which have a well defined orientation,
 * such as phones, laptops and portable devices since the control is expressed as a position
 * relative to the device's intended usage orientation.
 * , or , are said to have the
 * @ref VIDEO_CAMERA_ORIENTATION_EXTERNAL orientation.
 */
#define VIDEO_CID_CAMERA_ORIENTATION (VIDEO_CID_CAMERA_CLASS_BASE + 34)

/**
 * @brief Video camera orientation
 */
enum video_camera_orientation {
	/** Camera installed on the user-facing side of a phone/tablet/laptop device */
	VIDEO_CAMERA_ORIENTATION_FRONT = 0,
	/** Camera installed on the opposite side of the user */
	VIDEO_CAMERA_ORIENTATION_BACK = 1,
	/** Camera sensors not directly attached to the device or that can move freely */
	VIDEO_CAMERA_ORIENTATION_EXTERNAL = 2,
};

/** This read-only control describes the orientation of the sensor in the device.
 * The value is the rotation correction in degrees in the counter-clockwise direction to be
 * applied to the captured images once captured to memory to compensate for the camera sensor
 * mounting rotation.
 */
#define VIDEO_CID_CAMERA_SENSOR_ROTATION (VIDEO_CID_CAMERA_CLASS_BASE + 35)

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

/** Link frequency, applicable for the CSI2 based devices */
#define VIDEO_CID_LINK_FREQ (VIDEO_CID_IMAGE_PROC_CLASS_BASE + 1)

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
	/** device being queried, application needs to set this field */
	const struct device *dev;
	/** control id, application needs to set this field */
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
	union {
		const char *const *menu;
		const int64_t *int_menu;
	};
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
