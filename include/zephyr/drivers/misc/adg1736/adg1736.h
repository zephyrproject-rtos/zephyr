/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for the ADG736/ADG1736 dual SPDT analog switch driver
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MISC_ADG1736_H_
#define ZEPHYR_INCLUDE_DRIVERS_MISC_ADG1736_H_

#include <zephyr/device.h>
#include <stdint.h>

/**
 * @brief Switch enumeration for the ADG1736 family
 */
enum adg1736_switch {
	/** @brief Switch 1 */
	ADG1736_SW1 = 0,
	/** @brief Switch 2 */
	ADG1736_SW2 = 1,
};

/**
 * @brief Switch state enumeration (path selection)
 */
enum adg1736_state {
	/** @brief Switch connected to path B (default) */
	ADG1736_CONNECT_B = 0,
	/** @brief Switch connected to path A */
	ADG1736_CONNECT_A = 1,
};

/**
 * @brief Set the state of a specific switch
 *
 * @param dev Device instance
 * @param sw Switch to control (SW1 or SW2)
 * @param state Desired path (CONNECT_A or CONNECT_B)
 * @return 0 on success, negative error code otherwise
 */
int adg1736_set_switch_state(const struct device *dev,
			     enum adg1736_switch sw,
			     enum adg1736_state state);

/**
 * @brief Get the current state of a specific switch
 *
 * @param dev Device instance
 * @param sw Switch to read (SW1 or SW2)
 * @param state Pointer to store the current path
 * @return 0 on success, negative error code otherwise
 */
int adg1736_get_switch_state(const struct device *dev,
			     enum adg1736_switch sw,
			     enum adg1736_state *state);

/**
 * @brief Enable the device via the EN pin
 *
 * Only supported on the ADG1736 variant that has an en-gpios
 * property defined.
 *
 * @param dev Device instance
 * @return 0 on success, -ENOTSUP if no EN pin, negative error code otherwise
 */
int adg1736_enable(const struct device *dev);

/**
 * @brief Disable the device via the EN pin
 *
 * When disabled, both switches are open. Only supported on the
 * ADG1736 variant that has an en-gpios property defined.
 *
 * @param dev Device instance
 * @return 0 on success, -ENOTSUP if no EN pin, negative error code otherwise
 */
int adg1736_disable(const struct device *dev);

/**
 * @brief Reset both switches to the default state (CONNECT_B)
 *
 * @param dev Device instance
 * @return 0 on success, negative error code otherwise
 */
int adg1736_reset_switches(const struct device *dev);

#endif /* ZEPHYR_INCLUDE_DRIVERS_MISC_ADG1736_H_ */
