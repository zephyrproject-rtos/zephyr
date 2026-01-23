/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup slcd_interface
 * @brief Main header file for SLCD (Segmented LCD) controller driver API.
 *
 * This driver provides an interface for controlling SLCD (Segmented LCD) displays.
 * The SLCD controller can be implemented using a dedicated SLCD IP or a general
 * GPIO IP, depending on what the SoC provides. The driver offers basic control
 * functions for LCD segments, display state, and blinking functionality.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SLCD_H_
#define ZEPHYR_INCLUDE_DRIVERS_SLCD_H_

/**
 * @brief Interfaces for SLCD (Segmented LCD) controller.
 * @defgroup slcd_interface SLCD Controller
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

/** @cond INTERNAL_HIDDEN */

/**
 * @typedef slcd_set_pin_t
 * @brief Callback API to set LCD pin state for specified COM lines.
 * See slcd_set_pin() for argument description
 */
typedef int (*slcd_set_pin_t)(const struct device *dev, uint32_t pin,
			      uint8_t com_mask, bool on);

/**
 * @typedef slcd_blink_t
 * @brief Callback API to enable or disable display blinking.
 * See slcd_blink() for argument description
 */
typedef int (*slcd_blink_t)(const struct device *dev, bool on);

/**
 * @brief SLCD controller driver API
 *
 * This structure contains the function pointers for the SLCD driver API.
 * Driver implementations must provide these callbacks.
 */
__subsystem struct slcd_driver_api {
	slcd_set_pin_t set_pin;
	slcd_blink_t blink;
};

/** @endcond */

/**
 * @brief Set the LCD pin state for specified COM lines.
 *
 * This function configures the front plane segments for a given LCD pin across
 * multiple COM lines. It sets the phase state for each COM line specified
 * in the COM mask. The pin parameter is 1-based, and the com_mask specifies which
 * COM lines to configure where bit 0 = COM0, bit 1 = COM1, etc. A set bit (1) means
 * the corresponding COM line should be configured.
 *
 * @param dev SLCD controller device instance
 * @param pin The LCD pin number (0-based index)
 * @param com_mask Bit mask indicating which COM lines to configure.
 *                 Bit N = 1 configures COM line N.
 * @param on The state to set for the front plane phase:
 *           - true: Turn on the segment (activate)
 *           - false: Turn off the segment (deactivate)
 *
 * @retval 0 on success
 * @retval -ENOSYS if not supported/implemented
 * @retval -EINVAL if pin is invalid or com_mask is zero
 * @retval -errno Negative errno code on other failure
 */
static inline int slcd_set_pin(const struct device *dev, uint32_t pin,
			       uint8_t com_mask, bool on)
{
	const struct slcd_driver_api *api =
		(const struct slcd_driver_api *)dev->api;

	if (api->set_pin == NULL) {
		return -ENOSYS;
	}

	if (com_mask == 0) {
		return -EINVAL;
	}

	return api->set_pin(dev, pin, com_mask, on);
}

/**
 * @brief Enable or disable display blinking.
 *
 * This function controls the blinking mode of the SLCD display. When enabled,
 * the display will blink at a controller-defined rate. The exact blinking
 * behavior (rate, affected segments) is controller-specific.
 *
 * @param dev SLCD controller device instance
 * @param on true to enable blinking, false to disable blinking
 *
 * @retval 0 on success
 * @retval -ENOSYS if not supported/implemented
 * @retval -errno Negative errno code on other failure
 */
static inline int slcd_blink(const struct device *dev, bool on)
{
	const struct slcd_driver_api *api =
		(const struct slcd_driver_api *)dev->api;

	if (api->blink == NULL) {
		return -ENOSYS;
	}

	return api->blink(dev, on);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SLCD_H_ */
