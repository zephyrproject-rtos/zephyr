/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup slcd_panel_interface
 * @brief Main header file for SLCD Panel device driver API.
 *
 * This driver provides an interface for SLCD panel devices that are
 * connected to an SLCD controller. An SLCD panel device represents a
 * specific segmented LCD display with configurable segment types,
 * positions, and icons.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SLCD_PANEL_H_
#define ZEPHYR_INCLUDE_DRIVERS_SLCD_PANEL_H_

/**
 * @brief Interfaces for SLCD Panel devices.
 * @defgroup slcd_panel_interface SLCD Panel
 * @since 4.4
 * @ingroup display_interface
 * @{
 */

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enumeration of supported segment display types.
 *
 * Defines the number of segments per position in the LCD display.
 */
enum slcd_panel_segment_type {
	/** 3-segment display */
	SLCD_PANEL_SEGMENT_3 = 0,
	/** 7-segment display (standard numeric display) */
	SLCD_PANEL_SEGMENT_7 = 1,
	/** 8-segment display */
	SLCD_PANEL_SEGMENT_8 = 2,
	/** 9-segment display */
	SLCD_PANEL_SEGMENT_9 = 3,
	/** 10-segment display */
	SLCD_PANEL_SEGMENT_10 = 4,
	/** 11-segment display */
	SLCD_PANEL_SEGMENT_11 = 5,
	/** 12-segment display */
	SLCD_PANEL_SEGMENT_12 = 6,
	/** 14-segment display (alphanumeric) */
	SLCD_PANEL_SEGMENT_14 = 7,
	/** 16-segment display (alphanumeric) */
	SLCD_PANEL_SEGMENT_16 = 8,
};

/**
 * @brief SLCD panel capabilities structure.
 *
 * Holds information about the capabilities and characteristics of an SLCD panel,
 * including the segment type, number of character/digit positions, and number
 * of available special icons.
 */
struct slcd_panel_capabilities {
	/** Type of segments in the display (e.g., 7-segment, 14-segment) */
	enum slcd_panel_segment_type segment_type;
	/** Number of character/digit positions on the display */
	uint8_t num_positions;
	/** Number of special icons or indicators on the display */
	uint8_t num_icons;
	/** Whether number is supported to display on position. */
	bool support_number;
	/** Whether letter is supported to display on position. */
	bool support_letter;
};

/** @cond INTERNAL_HIDDEN */

/**
 * @typedef slcd_panel_show_number_t
 * @brief Callback API to display a number at a specific position.
 * See slcd_panel_show_number() for argument description
 */
typedef int (*slcd_panel_show_number_t)(const struct device *dev,
					uint32_t position, uint8_t number,
					bool on);

/**
 * @typedef slcd_panel_show_letter_t
 * @brief Callback API to display a letter at a specific position.
 * See slcd_panel_show_letter() for argument description
 */
typedef int (*slcd_panel_show_letter_t)(const struct device *dev,
					uint32_t position, uint8_t letter,
					bool on);

/**
 * @typedef slcd_panel_show_icon_t
 * @brief Callback API to display a special icon.
 * See slcd_panel_show_icon() for argument description
 */
typedef int (*slcd_panel_show_icon_t)(const struct device *dev,
				      uint32_t icon_index, bool on);

/**
 * @typedef slcd_panel_blink_t
 * @brief Callback API to enable or disable panel blinking.
 * See slcd_panel_blink() for argument description
 */
typedef int (*slcd_panel_blink_t)(const struct device *dev, bool on);

/**
 * @typedef slcd_panel_get_capabilities_t
 * @brief Callback API to get panel capabilities.
 * See slcd_panel_get_capabilities() for argument description
 */
typedef int (*slcd_panel_get_capabilities_t)(const struct device *dev,
					     struct slcd_panel_capabilities *cap);

/**
 * @brief SLCD panel device driver API
 *
 * This structure contains the function pointers for the SLCD panel
 * device driver API. Driver implementations must provide these callbacks.
 */
__subsystem struct slcd_panel_driver_api {
	slcd_panel_show_number_t show_number;
	slcd_panel_show_letter_t show_letter;
	slcd_panel_show_icon_t show_icon;
	slcd_panel_blink_t blink;
	slcd_panel_get_capabilities_t get_capabilities;
};

/** @endcond */

/**
 * @brief Display a number at a specific position.
 *
 * This function controls the display of a numeric digit or pattern at
 * a specific position on the SLCD panel. The number interpretation
 * depends on the segment type of the display and the driver implementation.
 *
 * For 7-segment displays, typical values 0-9 represent the digits.
 * The exact mapping of values to segment patterns is driver-specific
 * and should be documented in the specific SLCD panel device driver.
 *
 * @param dev SLCD panel device instance
 * @param position The position index on the display (0-based, starting from left)
 * @param number The numeric value or pattern to display
 * @param on true to turn on/display the number, false to turn off/clear the position
 *
 * @retval 0 on success
 * @retval -ENOSYS if not supported/implemented
 * @retval -EINVAL if position is out of range or number is invalid
 * @retval -errno Negative errno code on other failure
 */
static inline int slcd_panel_show_number(const struct device *dev,
					 uint32_t position, uint8_t number,
					 bool on)
{
	const struct slcd_panel_driver_api *api =
		(const struct slcd_panel_driver_api *)dev->api;

	if (api->show_number == NULL) {
		return -ENOSYS;
	}

	return api->show_number(dev, position, number, on);
}

/**
 * @brief Display a letter at a specific position.
 *
 * This function controls the display of a letter at a specific position
 * on the SLCD panel. The letter interpretation depends on the segment type
 * of the display and the driver implementation.
 *
 * For 7-segment displays, certain letter patterns can be displayed using
 * segment combinations. For 14-segment or 16-segment displays, full
 * alphanumeric characters can be displayed.
 *
 * @param dev SLCD panel device instance
 * @param position The position index on the display (0-based, starting from left)
 * @param letter The letter character to display (ASCII value or driver-specific code)
 * @param on true to turn on/display the letter, false to turn off/clear the position
 *
 * @retval 0 on success
 * @retval -ENOSYS if not supported/implemented
 * @retval -EINVAL if position is out of range or letter is invalid/unsupported
 * @retval -errno Negative errno code on other failure
 */
static inline int slcd_panel_show_letter(const struct device *dev,
					 uint32_t position, uint8_t letter,
					 bool on)
{
	const struct slcd_panel_driver_api *api =
		(const struct slcd_panel_driver_api *)dev->api;

	if (api->show_letter == NULL) {
		return -ENOSYS;
	}

	return api->show_letter(dev, position, letter, on);
}

/**
 * @brief Display a special icon.
 *
 * This function controls the display of special icons on the SLCD panel.
 * Icons are typically non-alphanumeric indicators such as battery symbols,
 * signal strength indicators, or other status icons that are independent
 * of the main numeric/alphanumeric display positions.
 *
 * @param dev SLCD panel device instance
 * @param icon_index The index of the icon to control (0-based)
 * @param on true to turn on/display the icon, false to turn off/hide the icon
 *
 * @retval 0 on success
 * @retval -ENOSYS if not supported/implemented
 * @retval -EINVAL if icon_index is out of range
 * @retval -errno Negative errno code on other failure
 */
static inline int slcd_panel_show_icon(const struct device *dev,
				       uint32_t icon_index, bool on)
{
	const struct slcd_panel_driver_api *api =
		(const struct slcd_panel_driver_api *)dev->api;

	if (api->show_icon == NULL) {
		return -ENOSYS;
	}

	return api->show_icon(dev, icon_index, on);
}

/**
 * @brief Enable or disable panel blinking.
 *
 * This function controls the blinking mode of the SLCD panel. When enabled,
 * the panel will blink the displayed content at a controller-defined rate.
 * The blinking rate is controller-specific.
 *
 * @param dev SLCD panel device instance
 * @param on true to enable blinking, false to disable blinking
 *
 * @retval 0 on success
 * @retval -ENOSYS if not supported/implemented
 * @retval -errno Negative errno code on other failure
 */
static inline int slcd_panel_blink(const struct device *dev, bool on)
{
	const struct slcd_panel_driver_api *api =
		(const struct slcd_panel_driver_api *)dev->api;

	if (api->blink == NULL) {
		return -ENOSYS;
	}

	return api->blink(dev, on);
}

/**
 * @brief Get panel capabilities and characteristics.
 *
 * This function retrieves the capability information of the SLCD panel,
 * including the segment type, number of character/digit positions, and
 * number of available icons, as well as which display types (numbers/letters)
 * are supported on the positions.
 *
 * @param dev SLCD panel device instance
 * @param cap Pointer to a slcd_panel_capabilities structure where the
 *            panel capabilities will be stored
 *
 * @retval 0 on success
 * @retval -ENOSYS if not supported/implemented
 * @retval -EINVAL if cap pointer is NULL
 * @retval -errno Negative errno code on other failure
 */
static inline int slcd_panel_get_capabilities(const struct device *dev,
					      struct slcd_panel_capabilities *cap)
{
	const struct slcd_panel_driver_api *api =
		(const struct slcd_panel_driver_api *)dev->api;

	if (cap == NULL) {
		return -EINVAL;
	}

	if (api->get_capabilities == NULL) {
		return -ENOSYS;
	}

	return api->get_capabilities(dev, cap);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SLCD_PANEL_H_ */
