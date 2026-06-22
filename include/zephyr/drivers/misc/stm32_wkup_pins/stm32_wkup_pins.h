/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for STM32 PWR wake-up pins configuration
 */

#ifndef ZEPHYR_DRIVERS_MISC_STM32_WKUP_PINS_H_
#define ZEPHYR_DRIVERS_MISC_STM32_WKUP_PINS_H_

#include <zephyr/drivers/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configure a GPIO pin as a source for STM32 PWR wake-up pins
 *
 * @param gpio Container for GPIO pin information specified in devicetree
 *
 * @return 0 on success, -EINVAL on invalid values
 */
int stm32_pwr_wkup_pin_cfg_gpio(const struct gpio_dt_spec *gpio);

/**
 * @brief Enable or Disable pull-up and pull-down configuration for
 * GPIO Ports that are associated with STM32 PWR wake-up pins
 */
void stm32_pwr_wkup_pin_cfg_pupd(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_MISC_STM32_WKUP_PINS_H_ */
