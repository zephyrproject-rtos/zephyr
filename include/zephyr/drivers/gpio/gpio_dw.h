/*
 * Copyright Meta Platforms, Inc. and its affiliates.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_DW_H_
#define ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_DW_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enable data and control source for a pin.
 *
 * Configures a pin to be controlled by software or hardware data source (if
 * supported).
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param pin Pin number.
 * @param hw_mode If true set pin to HW control mode.
 *
 * @retval 0 If successful.
 */
int gpio_dw_set_hw_mode(const struct device *port, gpio_pin_t pin, bool hw_mode);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_DW_H_ */
