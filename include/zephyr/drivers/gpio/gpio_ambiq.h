/*
 * Copyright (c) 2025 Ambiq Micro Inc. <www.ambiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GPIO_GPIO_AMBIQ_H_
#define ZEPHYR_DRIVERS_GPIO_GPIO_AMBIQ_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the actual gpio pin number.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pin Pin number of the select gpio group.
 *
 * @retval pin number.
 */
gpio_pin_t ambiq_gpio_get_pinnum(const struct device *dev, gpio_pin_t pin);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_GPIO_GPIO_AMBIQ_H_ */
