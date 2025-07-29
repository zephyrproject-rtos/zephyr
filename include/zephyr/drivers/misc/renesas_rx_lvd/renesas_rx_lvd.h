/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MISC_RENESAS_RX_LVD_H_
#define ZEPHYR_DRIVERS_MISC_RENESAS_RX_LVD_H_

#include "r_lvd_rx_if.h"

/**
 * @brief Get the current status of the LVD (Low Voltage Detection) circuit.
 *
 * This function retrieves the current voltage status from the LVD module,
 * including the position of the voltage level relative to the threshold,
 * and whether the voltage has crossed the threshold.
 * @param[in] dev LVD device instance.
 * @param[out] status_position Pointer to store the LVD status position.
 * @param[out] status_cross Pointer to store the LVD status cross.
 *
 * @retval 0 On success.
 * @retval -EINVAL On failure.
 */
int renesas_rx_lvd_get_status(const struct device *dev, lvd_status_position_t *status_position,
			      lvd_status_cross_t *status_cross);

/**
 * @brief Clear the current status of the LVD (Low Voltage Detection) circuit.
 *
 * This function clears the current voltage status from the LVD module.
 * @param[in] dev LVD device instance.
 *
 * @retval 0 On success.
 * @retval -EINVAL On failure.
 */
int renesas_rx_lvd_clear_status(const struct device *dev);

/**
 * @brief Register a callback function for LVD status changes.
 *
 * This function registers a callback function that will be called when the
 * LVD status changes. This applicable only if the LVD action is set to
 * trigger an interrupt on status change.
 * @param[in] dev LVD device instance.
 * @param[in] callback Pointer to the callback function.
 * @param[in] user_data Pointer to user data that will be passed to the callback.
 *
 * @retval 0 On success.
 * @retval -EINVAL On failure.
 */
int renesas_rx_lvd_register_callback(const struct device *dev, void (*callback)(void *),
				     void *user_data);

/**
 * @brief Set pin for CMPA2 if the target is CMPA2.
 *
 * This function sets the pin configuration for CMPA2 if the target
 * device is CMPA2.
 * @param[in] dev LVD device instance.
 *
 * @retval 0 On success.
 * @retval -EINVAL On failure.
 */
int renesas_rx_pin_set_cmpa2(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_MISC_RENESAS_RX_LVD_H_ */
