/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for the ADGM3121 MEMS switch driver
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MISC_ADGM3121_H_
#define ZEPHYR_INCLUDE_DRIVERS_MISC_ADGM3121_H_

#include <zephyr/device.h>
#include <stdint.h>

/**
 * @brief Switch enumeration for the ADGM3121
 *
 * ADGM3121 (DPDT): SW1=RF1A-RFCA, SW2=RF2A-RFCA, SW3=RF2B-RFCB, SW4=RF1B-RFCB
 */
enum adgm3121_switch {
	/** @brief Switch 1 (RF1A-RFCA) */
	ADGM3121_SW1 = 0,
	/** @brief Switch 2 (RF2A-RFCA) */
	ADGM3121_SW2 = 1,
	/** @brief Switch 3 (RF2B-RFCB) */
	ADGM3121_SW3 = 2,
	/** @brief Switch 4 (RF1B-RFCB) */
	ADGM3121_SW4 = 3,
};

/**
 * @brief Switch state enumeration
 */
enum adgm3121_state {
	/** @brief Switch disabled (open) */
	ADGM3121_DISABLE = 0,
	/** @brief Switch enabled (closed) */
	ADGM3121_ENABLE = 1,
};

/**
 * @brief Set the state of a specific switch
 *
 * @param dev Device instance
 * @param sw Switch to control (SW1 to SW4)
 * @param state Desired state (ENABLE or DISABLE)
 * @return 0 on success, negative error code otherwise
 */
int adgm3121_set_switch_state(const struct device *dev,
			      enum adgm3121_switch sw,
			      enum adgm3121_state state);

/**
 * @brief Get the current state of a specific switch
 *
 * @param dev Device instance
 * @param sw Switch to read (SW1 to SW4)
 * @param state Pointer to store the current state
 * @return 0 on success, negative error code otherwise
 */
int adgm3121_get_switch_state(const struct device *dev,
			      enum adgm3121_switch sw,
			      enum adgm3121_state *state);

/**
 * @brief Set multiple switches at once using a bitmask
 *
 * @param dev Device instance
 * @param switch_mask Bitmask of switches to enable (bits 0-3 for SW1-SW4)
 * @return 0 on success, negative error code otherwise
 */
int adgm3121_set_switches(const struct device *dev, uint8_t switch_mask);

/**
 * @brief Get all switch states as a bitmask
 *
 * @param dev Device instance
 * @param switch_mask Pointer to store switch states bitmask
 * @return 0 on success, negative error code otherwise
 */
int adgm3121_get_switches(const struct device *dev, uint8_t *switch_mask);

/**
 * @brief Reset all switches to the disabled state
 *
 * @param dev Device instance
 * @return 0 on success, negative error code otherwise
 */
int adgm3121_reset_switches(const struct device *dev);

/**
 * @brief Check for internal errors (SPI mode only)
 *
 * @param dev Device instance
 * @param error_status Pointer to store error status (bits 7:6 of register)
 * @return 0 on success, negative error code otherwise
 * @retval -ENOTSUP if device is in parallel GPIO mode
 */
int adgm3121_check_internal_error(const struct device *dev,
				  uint8_t *error_status);

#endif /* ZEPHYR_INCLUDE_DRIVERS_MISC_ADGM3121_H_ */
