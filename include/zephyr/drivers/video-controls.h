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

/** Automatic white balance (cameras). */
#define VIDEO_CID_AUTO_WHITE_BALANCE (VIDEO_CID_BASE + 12)

/** When set the device will do a white balance and then hold the current setting.
 * This is an action control that acts as trigger, the value is ignored.
 * Unlike @ref VIDEO_CID_AUTO_WHITE_BALANCE that continuously adjust the white balance when on,
 * this control performs a single white balance adjustment cycle whenever set to any value.
 */
#define VIDEO_CID_DO_WHITE_BALANCE (VIDEO_CID_BASE + 13)

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

/** Frequency of the power line to compensate for, avoiding flicker due to artificial lighting */
#define VIDEO_CID_POWER_LINE_FREQUENCY (VIDEO_CID_BASE + 24)
enum video_power_line_frequency {
	VIDEO_CID_POWER_LINE_FREQUENCY_DISABLED = 0,
	VIDEO_CID_POWER_LINE_FREQUENCY_50HZ = 1,
	VIDEO_CID_POWER_LINE_FREQUENCY_60HZ = 2,
	VIDEO_CID_POWER_LINE_FREQUENCY_AUTO = 3,
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

/** Chroma automatic gain control. */
#define VIDEO_CID_CHROMA_AGC (VIDEO_CID_BASE + 29)

/** Enable the color killer, i.e. force a black and white image in case of a weak video signal. */
#define VIDEO_CID_COLOR_KILLER (VIDEO_CID_BASE + 30)

/** Selects a color effect. */
#define VIDEO_CID_COLORFX (VIDEO_CID_BASE + 31)
enum video_colorfx {
	VIDEO_COLORFX_NONE = 0,
	VIDEO_COLORFX_BW = 1,
	VIDEO_COLORFX_SEPIA = 2,
	VIDEO_COLORFX_NEGATIVE = 3,
	VIDEO_COLORFX_EMBOSS = 4,
	VIDEO_COLORFX_SKETCH = 5,
	VIDEO_COLORFX_SKY_BLUE = 6,
	VIDEO_COLORFX_GRASS_GREEN = 7,
	VIDEO_COLORFX_SKIN_WHITEN = 8,
	VIDEO_COLORFX_VIVID = 9,
	VIDEO_COLORFX_AQUA = 10,
	VIDEO_COLORFX_ART_FREEZE = 11,
	VIDEO_COLORFX_SILHOUETTE = 12,
	VIDEO_COLORFX_SOLARIZATION = 13,
	VIDEO_COLORFX_ANTIQUE = 14,
	VIDEO_COLORFX_SET_CBCR = 15,
	VIDEO_COLORFX_SET_RGB = 16,
};

/* Enable Automatic Brightness. */
#define VIDEO_CID_AUTOBRIGHTNESS (VIDEO_CID_BASE + 32)

/** Switch the band-stop filter of a camera sensor on or off, or specify its strength.
 * Such band-stop filters can be used, for example, to filter out the fluorescent light component.
 */
#define VIDEO_CID_BAND_STOP_FILTER (VIDEO_CID_BASE + 33)

/** Rotates the image by the specified angle.
 * Common angles are 90, 270 and 180. Rotating the image to 90 and 270 will reverse the height and
 * width of the display window.
 */
#define VIDEO_CID_ROTATE (VIDEO_CID_BASE + 34)

/** Sets the background color on the current output device.
 * The 32-bit of the supplied value are interpreted as follow:
 * - bits [7:0] as Red component,
 * - bits [15:8] as Green component,
 * - bits [23:16] as Blue component,
 * - bits [31:24] must be zero.
 */
#define VIDEO_CID_BG_COLOR (VIDEO_CID_BASE + 35)

/** Adjusts the Chroma gain control (for use when chroma AGC is disabled). */
#define VIDEO_CID_CHROMA_GAIN (VIDEO_CID_BASE + 36)

/** Switch on or off the illuminator 1 of the device (usually a microscope). */
#define VIDEO_CID_ILLUMINATORS_1 (VIDEO_CID_BASE + 37)

/** Switch on or off the illuminator 2 of the device (usually a microscope). */
#define VIDEO_CID_ILLUMINATORS_2 (VIDEO_CID_BASE + 38)

/** Sets the alpha color component.
 * Some devices produce data with a user-controllable alpha component. Set the value applied to
 * the alpha channel of every pixel produced.
 */
#define VIDEO_CID_ALPHA_COMPONENT (VIDEO_CID_BASE + 41)

/** Determines the Cb and Cr coefficients for the @ref VIDEO_COLORFX_SET_CBCR color effect.
 * The 32-bit of the supplied value are interpreted as follow:
 * - bits [7:0] as Cr component,
 * - bits [15:8] as Cb component,
 * - bits [31:16] must be zero.
 */
#define VIDEO_CID_COLORFX_CBCR (VIDEO_CID_BASE + 42)

/** Determines the Red, Green, and Blue coefficients for @ref VIDEO_COLORFX_SET_RGB color effect.
 * The 32-bit of the supplied value are interpreted as follow:
 * - bits [7:0] as Blue component,
 * - bits [15:8] as Green component,
 * - bits [23:16] as Red component,
 * - bits [31:24] must be zero.
 */
#define VIDEO_CID_COLORFX_RGB (VIDEO_CID_BASE + 43)

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
enum video_exposure_auto {
	VIDEO_EXPOSURE_AUTO = 0,
	VIDEO_EXPOSURE_MANUAL = 1,
	VIDEO_EXPOSURE_SHUTTER_PRIORITY = 2,
	VIDEO_EXPOSURE_APERTURE_PRIORITY = 3
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

/** When this control is set, the camera moves horizontally to the default position. */
#define VIDEO_CID_PAN_RESET (VIDEO_CID_CAMERA_CLASS_BASE + 6)

/** When this control is set, the camera moves vertically to the default position. */
#define VIDEO_CID_TILT_RESET (VIDEO_CID_CAMERA_CLASS_BASE + 7)

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

/** Prevent video from being acquired by the camera.
 * When this control is set to TRUE (1), no image can be captured by the camera. Common means to
 * enforce privacy are mechanical obturation of the sensor and firmware image processing,
 * but the device is not restricted to these methods. Devices that implement the privacy control
 * must support read access and may support write access.
 */
#define VIDEO_CID_PRIVACY (VIDEO_CID_CAMERA_CLASS_BASE + 16)

/** This control sets the camera's aperture to the specified value.
 * The unit is undefined. Larger values open the iris wider, smaller values close it.
 */
#define VIDEO_CID_IRIS_ABSOLUTE (VIDEO_CID_CAMERA_CLASS_BASE + 17)

/** This write-only control modifies the camera's aperture by the specified amount.
 * The unit is undefined. Positive values open the iris one step further, negative values close
 * it one step further.
 */
#define VIDEO_CID_IRIS_RELATIVE (VIDEO_CID_CAMERA_CLASS_BASE + 18)

/** Determines the automatic exposure compensation.
 * It is effective only when @ref VIDEO_CID_EXPOSURE_AUTO control is set to
 * @ref VIDEO_EXPOSURE_AUTO, @ref VIDEO_EXPOSURE_SHUTTER_PRIORITY or
 * @ref VIDEO_EXPOSURE_APERTURE_PRIORITY. It is expressed in terms of Exposure Value (EV).
 * Drivers should interpret the values as 0.001 EV units, where the value 1000 stands for +1 EV.
 * Increasing the exposure compensation value is equivalent to decreasing the EV and will
 * increase the amount of light at the image sensor. The camera performs the exposure
 * compensation by adjusting absolute exposure time and/or aperture.
 */
#define VIDEO_CID_AUTO_EXPOSURE_BIAS (VIDEO_CID_CAMERA_CLASS_BASE + 19)

/** Sets white balance to automatic, manual or a preset.
 * The presets determine color temperature of the light as a hint to the camera for white balance
 * adjustments resulting in most accurate color representation. The following white balance presets
 * are listed in order of increasing color temperature.
 */
#define VIDEO_CID_AUTO_N_PRESET_WHITE_BALANCE (VIDEO_CID_CAMERA_CLASS_BASE + 20)
enum video_auto_n_preset_white_balance {
	VIDEO_WHITE_BALANCE_MANUAL = 0,
	VIDEO_WHITE_BALANCE_AUTO = 1,
	VIDEO_WHITE_BALANCE_INCANDESCENT = 2,
	VIDEO_WHITE_BALANCE_FLUORESCENT = 3,
	VIDEO_WHITE_BALANCE_FLUORESCENT_H = 4,
	VIDEO_WHITE_BALANCE_HORIZON = 5,
	VIDEO_WHITE_BALANCE_DAYLIGHT = 6,
	VIDEO_WHITE_BALANCE_FLASH = 7,
	VIDEO_WHITE_BALANCE_CLOUDY = 8,
	VIDEO_WHITE_BALANCE_SHADE = 9,
	VIDEO_WHITE_BALANCE_GREYWORLD = 10,
};

/** Enables or disables the camera's wide dynamic range feature.
 * This feature allows to obtain clear images in situations where intensity of the illumination
 * varies significantly throughout the scene, i.e. there are simultaneously very dark and very
 * bright areas. It is most commonly realized in cameras by combining two subsequent frames with
 * different exposure times.
 */
#define VIDEO_CID_WIDE_DYNAMIC_RANGE (VIDEO_CID_CAMERA_CLASS_BASE + 21)

/** Enables or disables image stabilization. */
#define VIDEO_CID_IMAGE_STABILIZATION (VIDEO_CID_CAMERA_CLASS_BASE + 22)

/** Determines the ISO equivalent of an image sensor indicating the sensor's sensitivity to light.
 * The numbers are expressed in linear scale. Applications should interpret the values as standard
 * ISO values multiplied by 1000, e.g. control value 800 stands for ISO 0.8. Drivers will usually
 * support only a subset of standard ISO values.
 * Setting this control when @ref VIDEO_CID_ISO_SENSITIVITY_AUTO is not
 * @ref VIDEO_ISO_SENSITIVITY_MANUAL is undefined.
 * Drivers should ignore such requests.
 */
#define VIDEO_CID_ISO_SENSITIVITY (VIDEO_CID_CAMERA_CLASS_BASE + 23)

/** Enables or disables automatic ISO sensitivity adjustments. */
#define VIDEO_CID_ISO_SENSITIVITY_AUTO (VIDEO_CID_CAMERA_CLASS_BASE + 24)
enum video_iso_sensitivity_auto {
	VIDEO_ISO_SENSITIVITY_MANUAL = 0,
	VIDEO_ISO_SENSITIVITY_AUTO = 1,
};

/** Determines how the camera measures the amount of light available for the frame exposure. */
#define VIDEO_CID_EXPOSURE_METERING (VIDEO_CID_CAMERA_CLASS_BASE + 25)
enum video_exposure_metering {
	VIDEO_EXPOSURE_METERING_AVERAGE = 0,
	VIDEO_EXPOSURE_METERING_CENTER_WEIGHTED = 1,
	VIDEO_EXPOSURE_METERING_SPOT = 2,
	VIDEO_EXPOSURE_METERING_MATRIX = 3,
};

/** This control selects scene programs optimized for common shooting scenes.
 * Within these modes the camera determines best exposure, aperture, focusing, light metering,
 * white balance and equivalent sensitivity. The controls of those parameters are influenced by
 * the scene mode control. An exact behavior in each mode is subject to the camera specification.
 * When the scene mode feature is not used, this control should be set to
 * @ref VIDEO_SCENE_MODE_NONE to make sure the other possibly related controls are accessible.
 */
#define VIDEO_CID_SCENE_MODE (VIDEO_CID_CAMERA_CLASS_BASE + 26)
enum video_scene_mode {
	VIDEO_SCENE_MODE_NONE = 0,
	VIDEO_SCENE_MODE_BACKLIGHT = 1,
	VIDEO_SCENE_MODE_BEACH_SNOW = 2,
	VIDEO_SCENE_MODE_CANDLE_LIGHT = 3,
	VIDEO_SCENE_MODE_DAWN_DUSK = 4,
	VIDEO_SCENE_MODE_FALL_COLORS = 5,
	VIDEO_SCENE_MODE_FIREWORKS = 6,
	VIDEO_SCENE_MODE_LANDSCAPE = 7,
	VIDEO_SCENE_MODE_NIGHT = 8,
	VIDEO_SCENE_MODE_PARTY_INDOOR = 9,
	VIDEO_SCENE_MODE_PORTRAIT = 10,
	VIDEO_SCENE_MODE_SPORTS = 11,
	VIDEO_SCENE_MODE_SUNSET = 12,
	VIDEO_SCENE_MODE_TEXT = 13,
};

/** This control locks or unlocks the automatic focus, exposure and white balance.
 * The automatic adjustments can be paused independently by setting their lock bit to 1.
 * The camera then retains the settings until the lock bit is cleared.
 * When a given algorithm is not enabled, drivers return no error.
 * An example might be an application setting bit @ref VIDEO_LOCK_WHITE_BALANCE when the
 * @ref VIDEO_CID_AUTO_WHITE_BALANCE control is off (set to 0).
 * The value of this control may be changed by exposure, white balance or focus controls.
 */
#define VIDEO_CID_3A_LOCK (VIDEO_CID_CAMERA_CLASS_BASE + 27)
enum video_3a_lock {
	VIDEO_LOCK_EXPOSURE = BIT(0),
	VIDEO_LOCK_WHITE_BALANCE = BIT(1),
	VIDEO_LOCK_FOCUS = BIT(2),
};

/** Starts single auto focus process.
 * Setting this control when @ref VIDEO_CID_FOCUS_AUTO is on (set to 1) is undefined.
 * Drivers should ignore such requests.
 */
#define VIDEO_CID_AUTO_FOCUS_START (VIDEO_CID_CAMERA_CLASS_BASE + 28)

/** Aborts automatic focusing started with @ref VIDEO_CID_AUTO_FOCUS_START control.
 * Setting this control when @ref VIDEO_CID_FOCUS_AUTO is on (set to 1) is undefined.
 * Drivers should ignore such requests.
 */
#define VIDEO_CID_AUTO_FOCUS_STOP (VIDEO_CID_CAMERA_CLASS_BASE + 29)

/** The automatic focus status.
 * This is a read-only control.
 * Setting @ref VIDEO_LOCK_FOCUS lock bit of the @ref VIDEO_CID_3A_LOCK control may stop updates
 * of the @ref VIDEO_CID_AUTO_FOCUS_STATUS control value.
 */
#define VIDEO_CID_AUTO_FOCUS_STATUS (VIDEO_CID_CAMERA_CLASS_BASE + 30)
enum video_auto_focus_status {
	VIDEO_AUTO_FOCUS_STATUS_IDLE = 0,
	VIDEO_AUTO_FOCUS_STATUS_BUSY = BIT(0),
	VIDEO_AUTO_FOCUS_STATUS_REACHED = BIT(1),
	VIDEO_AUTO_FOCUS_STATUS_FAILED = BIT(2),
};

/** Determines auto focus distance range for which lens may be adjusted. */
#define VIDEO_CID_AUTO_FOCUS_RANGE (VIDEO_CID_CAMERA_CLASS_BASE + 31)
enum video_auto_focus_range {
	VIDEO_AUTO_FOCUS_RANGE_AUTO = 0,
	VIDEO_AUTO_FOCUS_RANGE_NORMAL = 1,
	VIDEO_AUTO_FOCUS_RANGE_MACRO = 2,
	VIDEO_AUTO_FOCUS_RANGE_INFINITY = 3,
};

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

/** Change the sensor HDR mode.
 * A HDR picture is obtained by merging two captures of the same scene using two different
 * exposure periods. HDR mode describes the way these two captures are merged in the sensor.
 * As modes differ for each sensor, menu items are not standardized by this control and have
 * hardware-specific values.
 */
#define VIDEO_CID_HDR_SENSOR_MODE (VIDEO_CID_CAMERA_CLASS_BASE + 36)

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
