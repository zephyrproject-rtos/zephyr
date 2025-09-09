/*
 * Copyright (c) 2022-2023 Jamie McCrae
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup auxdisplay_interface
 * @brief Main header file for auxiliary (textual/non-graphical) display driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_AUXDISPLAY_H_
#define ZEPHYR_INCLUDE_DRIVERS_AUXDISPLAY_H_

/**
 * @brief Interfaces for auxiliary (textual/non-graphical) displays.
 * @defgroup auxdisplay_interface Auxiliary (Text) Display
 * @since 3.4
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <stdint.h>
#include <stddef.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Used for minimum and maximum brightness/backlight values if not supported */
#define AUXDISPLAY_LIGHT_NOT_SUPPORTED 0

/** @brief Used to describe the mode of an auxiliary (text) display */
typedef uint32_t auxdisplay_mode_t;

/** @brief Used for moving the cursor or display position */
enum auxdisplay_position {
	/** Moves to specified X,Y position */
	AUXDISPLAY_POSITION_ABSOLUTE = 0,

	/** Shifts current position by +/- X,Y position, does not take display direction into
	 *  consideration
	 */
	AUXDISPLAY_POSITION_RELATIVE,

	/** Shifts current position by +/- X,Y position, takes display direction into
	 *  consideration
	 */
	AUXDISPLAY_POSITION_RELATIVE_DIRECTION,

	AUXDISPLAY_POSITION_COUNT,
};

/** @brief Used for setting character append position */
enum auxdisplay_direction {
	/** Each character will be placed to the right of existing characters */
	AUXDISPLAY_DIRECTION_RIGHT = 0,

	/** Each character will be placed to the left of existing characters */
	AUXDISPLAY_DIRECTION_LEFT,

	AUXDISPLAY_DIRECTION_COUNT,
};

/** @brief Light levels for brightness and/or backlight. If not supported by a
 *  display/driver, both minimum and maximum will be AUXDISPLAY_LIGHT_NOT_SUPPORTED.
 */
struct auxdisplay_light {
	/** Minimum light level supported */
	uint8_t minimum;

	/** Maximum light level supported */
	uint8_t maximum;
};

/** @brief Structure holding display capabilities. */
struct auxdisplay_capabilities {
	/** Number of character columns */
	uint16_t columns;

	/** Number of character rows */
	uint16_t rows;

	/** Display-specific data (e.g. 4-bit or 8-bit mode for HD44780-based displays) */
	auxdisplay_mode_t mode;

	/** Brightness details for display (if supported) */
	struct auxdisplay_light brightness;

	/** Backlight details for display (if supported) */
	struct auxdisplay_light backlight;

	/** Number of custom characters supported by display (0 if unsupported) */
	uint8_t custom_characters;

	/** Width (in pixels) of a custom character, supplied custom characters should match. */
	uint8_t custom_character_width;

	/** Height (in pixels) of a custom character, supplied custom characters should match. */
	uint8_t custom_character_height;
};

/** @brief Structure for a custom command. This may be extended by specific drivers. */
struct auxdisplay_custom_data {
	/** Raw command data to be sent */
	uint8_t *data;

	/** Length of supplied data */
	uint16_t len;

	/** Display-driver specific options for command */
	uint32_t options;
};

/** @brief Structure for a custom character. */
struct auxdisplay_character {
	/** Custom character index on the display */
	uint8_t index;

	/** Custom character pixel data, a character must be valid for a display consisting
	 *  of a uint8 array of size character width by character height, values should be
	 *  0x00 for pixel off or 0xff for pixel on, if a display supports shades then values
	 *  between 0x00 and 0xff may be used (display driver dependent).
	 */
	uint8_t *data;

	/** Will be updated with custom character index to use in the display write function to
	 *  disaplay this custom character
	 */
	uint8_t character_code;
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * For internal use only, skip these in public documentation.
 */

/**
 * @typedef	auxdisplay_display_on_t
 * @brief	Callback API to turn display on
 * See auxdisplay_display_on() for argument description
 */
typedef int (*auxdisplay_display_on_t)(const struct device *dev);

/**
 * @typedef	auxdisplay_display_off_t
 * @brief	Callback API to turn display off
 * See auxdisplay_display_off() for argument description
 */
typedef int (*auxdisplay_display_off_t)(const struct device *dev);

/**
 * @typedef	auxdisplay_cursor_set_enabled_t
 * @brief	Callback API to turn display cursor visibility on or off
 * See auxdisplay_cursor_set_enabled() for argument description
 */
typedef int (*auxdisplay_cursor_set_enabled_t)(const struct device *dev, bool enabled);

/**
 * @typedef	auxdisplay_position_blinking_set_enabled_t
 * @brief	Callback API to turn the current position blinking on or off
 * See auxdisplay_position_blinking_set_enabled() for argument description
 */
typedef int (*auxdisplay_position_blinking_set_enabled_t)(const struct device *dev,
							  bool enabled);

/**
 * @typedef	auxdisplay_cursor_shift_set_t
 * @brief	Callback API to set how the cursor shifts after a character is written
 * See auxdisplay_cursor_shift_set() for argument description
 */
typedef int (*auxdisplay_cursor_shift_set_t)(const struct device *dev, uint8_t direction,
					     bool display_shift);

/**
 * @typedef	auxdisplay_cursor_position_set_t
 * @brief	Callback API to set the cursor position
 * See auxdisplay_cursor_position_set() for argument description
 */
typedef int (*auxdisplay_cursor_position_set_t)(const struct device *dev,
						enum auxdisplay_position type,
						int16_t x, int16_t y);

/**
 * @typedef	auxdisplay_cursor_position_get_t
 * @brief	Callback API to get the cursor position
 * See auxdisplay_cursor_position_get() for argument description
 */
typedef int (*auxdisplay_cursor_position_get_t)(const struct device *dev, int16_t *x,
						int16_t *y);

/**
 * @typedef	auxdisplay_display_position_set_t
 * @brief	Callback API to set the current position of the display
 * See auxdisplay_display_position_set() for argument description
 */
typedef int (*auxdisplay_display_position_set_t)(const struct device *dev,
						 enum auxdisplay_position type,
						 int16_t x, int16_t y);

/**
 * @typedef	auxdisplay_display_position_get_t
 * @brief	Callback API to get the current position of the display
 * See auxdisplay_display_position_get() for argument description
 */
typedef int (*auxdisplay_display_position_get_t)(const struct device *dev, int16_t *x,
						 int16_t *y);

/**
 * @typedef	auxdisplay_capabilities_get_t
 * @brief	Callback API to get details on the display
 * See auxdisplay_capabilities_get() for argument description
 */
typedef int (*auxdisplay_capabilities_get_t)(const struct device *dev,
					     struct auxdisplay_capabilities *capabilities);

/**
 * @typedef	auxdisplay_clear_t
 * @brief	Callback API to clear the contents of the display
 * See auxdisplay_clear() for argument description
 */
typedef int (*auxdisplay_clear_t)(const struct device *dev);

/**
 * @typedef	auxdisplay_brightness_get_t
 * @brief	Callback API to get the current and minimum/maximum supported
 *		brightness settings of the display
 * See auxdisplay_brightness_get_api() for argument description
 */
typedef int (*auxdisplay_brightness_get_t)(const struct device *dev, uint8_t *brightness);

/**
 * @typedef	auxdisplay_brightness_set_t
 * @brief	Callback API to set the brightness of the display
 * See auxdisplay_brightness_set_api() for argument description
 */
typedef int (*auxdisplay_brightness_set_t)(const struct device *dev, uint8_t brightness);

/**
 * @typedef	auxdisplay_backlight_get_t
 * @brief	Callback API to get the current and minimum/maximum supported
 *		backlight settings of the display
 * See auxdisplay_backlight_set() for argument description
 */
typedef int (*auxdisplay_backlight_get_t)(const struct device *dev, uint8_t *backlight);

/**
 * @typedef	auxdisplay_backlight_set_t
 * @brief	Callback API to set the backlight status
 * See auxdisplay_backlight_set() for argument description
 */
typedef int (*auxdisplay_backlight_set_t)(const struct device *dev, uint8_t backlight);

/**
 * @typedef	auxdisplay_is_busy_t
 * @brief	Callback API to check if the display is busy with an operation
 * See auxdisplay_is_busy() for argument description
 */
typedef int (*auxdisplay_is_busy_t)(const struct device *dev);

/**
 * @typedef	auxdisplay_custom_character_set_t
 * @brief	Callback API to set a customer character on the display for usage
 * See auxdisplay_custom_character_set() for argument description
 */
typedef int (*auxdisplay_custom_character_set_t)(const struct device *dev,
						 struct auxdisplay_character *character);

/**
 * @typedef	auxdisplay_write_t
 * @brief	Callback API to write text to the display
 * See auxdisplay_write() for argument description
 */
typedef int (*auxdisplay_write_t)(const struct device *dev, const uint8_t *data, uint16_t len);

/**
 * @typedef	auxdisplay_custom_command_t
 * @brief	Callback API to send a custom command to the display
 * See auxdisplay_custom_command() for argument description
 */
typedef int (*auxdisplay_custom_command_t)(const struct device *dev,
					   struct auxdisplay_custom_data *command);

__subsystem struct auxdisplay_driver_api {
	auxdisplay_display_on_t display_on;
	auxdisplay_display_off_t display_off;
	auxdisplay_cursor_set_enabled_t cursor_set_enabled;
	auxdisplay_position_blinking_set_enabled_t position_blinking_set_enabled;
	auxdisplay_cursor_shift_set_t cursor_shift_set;
	auxdisplay_cursor_position_set_t cursor_position_set;
	auxdisplay_cursor_position_get_t cursor_position_get;
	auxdisplay_display_position_set_t display_position_set;
	auxdisplay_display_position_get_t display_position_get;
	auxdisplay_capabilities_get_t capabilities_get;
	auxdisplay_clear_t clear;
	auxdisplay_brightness_get_t brightness_get;
	auxdisplay_brightness_set_t brightness_set;
	auxdisplay_backlight_get_t backlight_get;
	auxdisplay_backlight_set_t backlight_set;
	auxdisplay_is_busy_t is_busy;
	auxdisplay_custom_character_set_t custom_character_set;
	auxdisplay_write_t write;
	auxdisplay_custom_command_t custom_command;
};

/**
 * @endcond
 */

/**
 * @brief		Turn display on.
 *
 * @param dev		Auxiliary display device instance
 *
 * @retval		0 on success.
 * @retval		-ENOSYS if not supported/implemented.
 * @retval		-errno Negative errno code on other failure.
 */
__syscall int auxdisplay_display_on(const struct device *dev);

static inline int z_impl_auxdisplay_display_on(const struct device *dev)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	if (!api->display_on) {
		return -ENOSYS;
	}

	return api->display_on(dev);
}

/**
 * @brief		Turn display off.
 *
 * @param dev		Auxiliary display device instance
 *
 * @retval		0 on success.
 * @retval		-ENOSYS if not supported/implemented.
 * @retval		-errno Negative errno code on other failure.
 */
__syscall int auxdisplay_display_off(const struct device *dev);

static inline int z_impl_auxdisplay_display_off(const struct device *dev)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	if (!api->display_off) {
		return -ENOSYS;
	}

	return api->display_off(dev);
}

/**
 * @brief		Set cursor enabled status on an auxiliary display
 *
 * @param dev		Auxiliary display device instance
 * @param enabled	True to enable cursor, false to disable
 *
 * @retval		0 on success.
 * @retval		-ENOSYS if not supported/implemented.
 * @retval		-errno Negative errno code on other failure.
 */
__syscall int auxdisplay_cursor_set_enabled(const struct device *dev,
					    bool enabled);

static inline int z_impl_auxdisplay_cursor_set_enabled(const struct device *dev,
						       bool enabled)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	if (!api->cursor_set_enabled) {
		return -ENOSYS;
	}

	return api->cursor_set_enabled(dev, enabled);
}

/**
 * @brief		Set cursor blinking status on an auxiliary display
 *
 * @param dev		Auxiliary display device instance
 * @param enabled	Set to true to enable blinking position, false to disable
 *
 * @retval		0 on success.
 * @retval		-ENOSYS if not supported/implemented.
 * @retval		-errno Negative errno code on other failure.
 */
__syscall int auxdisplay_position_blinking_set_enabled(const struct device *dev,
						       bool enabled);

static inline int z_impl_auxdisplay_position_blinking_set_enabled(const struct device *dev,
								  bool enabled)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	if (!api->position_blinking_set_enabled) {
		return -ENOSYS;
	}

	return api->position_blinking_set_enabled(dev, enabled);
}

/**
 * @brief		Set cursor shift after character write and display shift
 *
 * @param dev		Auxiliary display device instance
 * @param direction	Sets the direction of the display when characters are written
 * @param display_shift	If true, will shift the display when characters are written
 *			(which makes it look like the display is moving, not the cursor)
 *
 * @retval		0 on success.
 * @retval		-ENOSYS if not supported/implemented.
 * @retval		-EINVAL if provided argument is invalid.
 * @retval		-errno Negative errno code on other failure.
 */
__syscall int auxdisplay_cursor_shift_set(const struct device *dev,
					  uint8_t direction, bool display_shift);

static inline int z_impl_auxdisplay_cursor_shift_set(const struct device *dev,
						     uint8_t direction,
						     bool display_shift)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	if (!api->cursor_shift_set) {
		return -ENOSYS;
	}

	if (direction >= AUXDISPLAY_DIRECTION_COUNT) {
		return -EINVAL;
	}

	return api->cursor_shift_set(dev, direction, display_shift);
}

/**
 * @brief	Set cursor (and write position) on an auxiliary display
 *
 * @param dev	Auxiliary display device instance
 * @param type	Type of move, absolute or offset
 * @param x	Exact or offset X position
 * @param y	Exact or offset Y position
 *
 * @retval	0 on success.
 * @retval	-ENOSYS if not supported/implemented.
 * @retval	-EINVAL if provided argument is invalid.
 * @retval	-errno Negative errno code on other failure.
 */
__syscall int auxdisplay_cursor_position_set(const struct device *dev,
					     enum auxdisplay_position type,
					     int16_t x, int16_t y);

static inline int z_impl_auxdisplay_cursor_position_set(const struct device *dev,
							enum auxdisplay_position type,
							int16_t x, int16_t y)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	if (!api->cursor_position_set) {
		return -ENOSYS;
	} else if (type >= AUXDISPLAY_POSITION_COUNT) {
		return -EINVAL;
	} else if (type == AUXDISPLAY_POSITION_ABSOLUTE && (x < 0 || y < 0)) {
		return -EINVAL;
	}

	return api->cursor_position_set(dev, type, x, y);
}

/**
 * @brief	Get current cursor on an auxiliary display
 *
 * @param dev	Auxiliary display device instance
 * @param x	Will be updated with the exact X position
 * @param y	Will be updated with the exact Y position
 *
 * @retval	0 on success.
 * @retval	-ENOSYS if not supported/implemented.
 * @retval	-EINVAL if provided argument is invalid.
 * @retval	-errno Negative errno code on other failure.
 */
__syscall int auxdisplay_cursor_position_get(const struct device *dev,
					     int16_t *x, int16_t *y);

static inline int z_impl_auxdisplay_cursor_position_get(const struct device *dev,
							int16_t *x, int16_t *y)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	if (!api->cursor_position_get) {
		return -ENOSYS;
	}

	return api->cursor_position_get(dev, x, y);
}

/**
 * @brief	Set display position on an auxiliary display
 *
 * @param dev	Auxiliary display device instance
 * @param type	Type of move, absolute or offset
 * @param x	Exact or offset X position
 * @param y	Exact or offset Y position
 *
 * @retval	0 on success.
 * @retval	-ENOSYS if not supported/implemented.
 * @retval	-EINVAL if provided argument is invalid.
 * @retval	-errno Negative errno code on other failure.
 */
__syscall int auxdisplay_display_position_set(const struct device *dev,
					      enum auxdisplay_position type,
					      int16_t x, int16_t y);

static inline int z_impl_auxdisplay_display_position_set(const struct device *dev,
							 enum auxdisplay_position type,
							 int16_t x, int16_t y)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	if (!api->display_position_set) {
		return -ENOSYS;
	} else if (type >= AUXDISPLAY_POSITION_COUNT) {
		return -EINVAL;
	} else if (type == AUXDISPLAY_POSITION_ABSOLUTE && (x < 0 || y < 0)) {
		return -EINVAL;
	}

	return api->display_position_set(dev, type, x, y);
}

/**
 * @brief	Get current display position on an auxiliary display
 *
 * @param dev	Auxiliary display device instance
 * @param x	Will be updated with the exact X position
 * @param y	Will be updated with the exact Y position
 *
 * @retval	0 on success.
 * @retval	-ENOSYS if not supported/implemented.
 * @retval	-EINVAL if provided argument is invalid.
 * @retval	-errno Negative errno code on other failure.
 */
__syscall int auxdisplay_display_position_get(const struct device *dev,
					      int16_t *x, int16_t *y);

static inline int z_impl_auxdisplay_display_position_get(const struct device *dev,
							 int16_t *x, int16_t *y)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	if (!api->display_position_get) {
		return -ENOSYS;
	}

	return api->display_position_get(dev, x, y);
}

/**
 * @brief		Fetch capabilities (and details) of auxiliary display
 *
 * @param dev		Auxiliary display device instance
 * @param capabilities	Will be updated with the details of the auxiliary display
 *
 * @retval		0 on success.
 * @retval		-errno Negative errno code on other failure.
 */
__syscall int auxdisplay_capabilities_get(const struct device *dev,
					  struct auxdisplay_capabilities *capabilities);

static inline int z_impl_auxdisplay_capabilities_get(const struct device *dev,
						     struct auxdisplay_capabilities *capabilities)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	return api->capabilities_get(dev, capabilities);
}

/**
 * @brief	Clear display of auxiliary display and return to home position (note that
 *		this does not reset the display configuration, e.g. custom characters and
 *		display mode will persist).
 *
 * @param dev	Auxiliary display device instance
 *
 * @retval	0 on success.
 * @retval	-errno Negative errno code on other failure.
 */
__syscall int auxdisplay_clear(const struct device *dev);

static inline int z_impl_auxdisplay_clear(const struct device *dev)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	return api->clear(dev);
}

/**
 * @brief		Get the current brightness level of an auxiliary display
 *
 * @param dev		Auxiliary display device instance
 * @param brightness	Will be updated with the current brightness
 *
 * @retval		0 on success.
 * @retval		-ENOSYS if not supported/implemented.
 * @retval		-errno Negative errno code on other failure.
 */
__syscall int auxdisplay_brightness_get(const struct device *dev,
					uint8_t *brightness);

static inline int z_impl_auxdisplay_brightness_get(const struct device *dev,
						   uint8_t *brightness)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	if (!api->brightness_get) {
		return -ENOSYS;
	}

	return api->brightness_get(dev, brightness);
}

/**
 * @brief		Update the brightness level of an auxiliary display
 *
 * @param dev		Auxiliary display device instance
 * @param brightness	The brightness level to set
 *
 * @retval		0 on success.
 * @retval		-ENOSYS if not supported/implemented.
 * @retval		-EINVAL if provided argument is invalid.
 * @retval		-errno Negative errno code on other failure.
 */
__syscall int auxdisplay_brightness_set(const struct device *dev,
					uint8_t brightness);

static inline int z_impl_auxdisplay_brightness_set(const struct device *dev,
						   uint8_t brightness)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	if (!api->brightness_set) {
		return -ENOSYS;
	}

	return api->brightness_set(dev, brightness);
}

/**
 * @brief		Get the backlight level details of an auxiliary display
 *
 * @param dev		Auxiliary display device instance
 * @param backlight	Will be updated with the current backlight level
 *
 * @retval		0 on success.
 * @retval		-ENOSYS if not supported/implemented.
 * @retval		-errno Negative errno code on other failure.
 */
__syscall int auxdisplay_backlight_get(const struct device *dev,
				       uint8_t *backlight);

static inline int z_impl_auxdisplay_backlight_get(const struct device *dev,
						  uint8_t *backlight)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	if (!api->backlight_get) {
		return -ENOSYS;
	}

	return api->backlight_get(dev, backlight);
}

/**
 * @brief		Update the backlight level of an auxiliary display
 *
 * @param dev		Auxiliary display device instance
 * @param backlight	The backlight level to set
 *
 * @retval		0 on success.
 * @retval		-ENOSYS if not supported/implemented.
 * @retval		-EINVAL if provided argument is invalid.
 * @retval		-errno Negative errno code on other failure.
 */
__syscall int auxdisplay_backlight_set(const struct device *dev,
				       uint8_t backlight);

static inline int z_impl_auxdisplay_backlight_set(const struct device *dev,
						  uint8_t backlight)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	if (!api->backlight_set) {
		return -ENOSYS;
	}

	return api->backlight_set(dev, backlight);
}

/**
 * @brief	Check if an auxiliary display driver is busy
 *
 * @param dev	Auxiliary display device instance
 *
 * @retval	1 on success and display busy.
 * @retval	0 on success and display not busy.
 * @retval	-ENOSYS if not supported/implemented.
 * @retval	-errno Negative errno code on other failure.
 */
__syscall int auxdisplay_is_busy(const struct device *dev);

static inline int z_impl_auxdisplay_is_busy(const struct device *dev)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	if (!api->is_busy) {
		return -ENOSYS;
	}

	return api->is_busy(dev);
}

/**
 * @brief		Sets a custom character in the display, the custom character struct
 *			must contain the pixel data for the custom character to add and valid
 *			custom character index, if successful then the character_code variable
 *			in the struct will be set to the character code that can be used with
 *			the auxdisplay_write() function to show it.
 *
 *			A character must be valid for a display consisting of a uint8 array of
 *			size character width by character height, values should be 0x00 for
 *			pixel off or 0xff for pixel on, if a display supports shades then
 *			values between 0x00 and 0xff may be used (display driver dependent).
 *
 * @param dev		Auxiliary display device instance
 * @param character	Pointer to custom character structure
 *
 * @retval		0 on success.
 * @retval		-ENOSYS if not supported/implemented.
 * @retval		-EINVAL if provided argument is invalid.
 * @retval		-errno Negative errno code on other failure.
 */
__syscall int auxdisplay_custom_character_set(const struct device *dev,
					      struct auxdisplay_character *character);

static inline int z_impl_auxdisplay_custom_character_set(const struct device *dev,
							 struct auxdisplay_character *character)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	if (!api->custom_character_set) {
		return -ENOSYS;
	}

	return api->custom_character_set(dev, character);
}

/**
 * @brief	Write data to auxiliary display screen at current position
 *
 * @param dev	Auxiliary display device instance
 * @param data	Text data to write
 * @param len	Length of text data to write
 *
 * @retval	0 on success.
 * @retval	-EINVAL if provided argument is invalid.
 * @retval	-errno Negative errno code on other failure.
 */
__syscall int auxdisplay_write(const struct device *dev, const uint8_t *data,
			       uint16_t len);

static inline int z_impl_auxdisplay_write(const struct device *dev,
					  const uint8_t *data, uint16_t len)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	return api->write(dev, data, len);
}

/**
 * @brief	Send a custom command to the display (if supported by driver)
 *
 * @param dev	Auxiliary display device instance
 * @param data	Custom command structure (this may be extended by specific drivers)
 *
 * @retval	0 on success.
 * @retval	-ENOSYS if not supported/implemented.
 * @retval	-EINVAL if provided argument is invalid.
 * @retval	-errno Negative errno code on other failure.
 */
__syscall int auxdisplay_custom_command(const struct device *dev,
					struct auxdisplay_custom_data *data);

static inline int z_impl_auxdisplay_custom_command(const struct device *dev,
						   struct auxdisplay_custom_data *data)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	if (!api->custom_command) {
		return -ENOSYS;
	}

	return api->custom_command(dev, data);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <zephyr/syscalls/auxdisplay.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_AUXDISPLAY_H_ */
