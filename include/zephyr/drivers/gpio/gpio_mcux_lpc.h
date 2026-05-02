/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for NXP MCUX LPC GPIO driver
 * @ingroup gpio_mcux_lpc_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_MCUX_LPC_H_
#define ZEPHYR_INCLUDE_DRIVERS_GPIO_MCUX_LPC_H_

/**
 * @defgroup gpio_mcux_lpc_interface NXP MCUX LPC
 * @ingroup gpio_interface_ext
 * @brief NXP MCUX LPC GPIO controller
 * @{
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

/**
 * @brief Trigger a callback for a given pin.
 *
 * This allows other drivers to fire callbacks for the pin.
 *
 * @param dev  Pointer to the device structure for the driver instance.
 * @param pins The actual pin mask that triggered the interrupt.
 *
 */
void gpio_mcux_lpc_trigger_cb(const struct device *dev, uint32_t pins);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_GPIO_MCUX_LPC_H_ */
