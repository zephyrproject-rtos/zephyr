/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for the ADG1712/ADG2712 quad SPST analog switch driver
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MISC_ADG1712_H_
#define ZEPHYR_INCLUDE_DRIVERS_MISC_ADG1712_H_

#include <zephyr/device.h>
#include <stdint.h>

/**
 * @brief Switch enumeration for the ADG1712/ADG2712
 */
enum adg1712_switch {
	/** @brief Switch 1 */
	ADG1712_SW1 = 0,
	/** @brief Switch 2 */
	ADG1712_SW2 = 1,
	/** @brief Switch 3 */
	ADG1712_SW3 = 2,
	/** @brief Switch 4 */
	ADG1712_SW4 = 3,
};

/**
 * @brief Switch state enumeration
 */
enum adg1712_state {
	/** @brief Switch disabled (open) */
	ADG1712_DISABLE = 0,
	/** @brief Switch enabled (closed) */
	ADG1712_ENABLE = 1,
};

/**
 * @brief Set the state of a specific switch
 *
 * @param dev Device instance
 * @param sw Switch to control (SW1 to SW4)
 * @param state Desired state (ENABLE or DISABLE)
 * @return 0 on success, negative error code otherwise
 */
int adg1712_set_switch_state(const struct device *dev,
			     enum adg1712_switch sw,
			     enum adg1712_state state);

/**
 * @brief Get the current state of a specific switch
 *
 * @param dev Device instance
 * @param sw Switch to read (SW1 to SW4)
 * @param state Pointer to store the current state
 * @return 0 on success, negative error code otherwise
 */
int adg1712_get_switch_state(const struct device *dev,
			     enum adg1712_switch sw,
			     enum adg1712_state *state);

/**
 * @brief Set multiple switches at once using a bitmask
 *
 * @param dev Device instance
 * @param switch_mask Bitmask of switches to enable (bits 0-3 for SW1-SW4)
 * @return 0 on success, negative error code otherwise
 */
int adg1712_set_switches(const struct device *dev, uint8_t switch_mask);

/**
 * @brief Get all switch states as a bitmask
 *
 * @param dev Device instance
 * @param switch_mask Pointer to store switch states bitmask
 * @return 0 on success, negative error code otherwise
 */
int adg1712_get_switches(const struct device *dev, uint8_t *switch_mask);

/**
 * @brief Reset all switches to the disabled state
 *
 * @param dev Device instance
 * @return 0 on success, negative error code otherwise
 */
int adg1712_reset_switches(const struct device *dev);

#endif /* ZEPHYR_INCLUDE_DRIVERS_MISC_ADG1712_H_ */
