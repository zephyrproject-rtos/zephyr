/*
 * Copyright (c) 2026 ELAN Microelectronics Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file header for EM32 GPIO
 *
 * This header exports the GPIO configuration function for use by the
 * pinctrl driver, following the STM32 design pattern.
 */

#ifndef ZEPHYR_DRIVERS_GPIO_GPIO_EM32_H_
#define ZEPHYR_DRIVERS_GPIO_GPIO_EM32_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Pin configuration bit field definitions
 *
 * These match the definitions in pinctrl_soc.h for proper coordination.
 */
#define EM32_PINCFG_MODER_SHIFT   4
#define EM32_PINCFG_OTYPER_SHIFT  6
#define EM32_PINCFG_OSPEEDR_SHIFT 7
#define EM32_PINCFG_PUPDR_SHIFT   9
#define EM32_PINCFG_ODR_SHIFT     11
#define EM32_PINCFG_DRIVE_SHIFT   13

/* Pull-up/Pull-down values */
#define EM32_PINCFG_NO_PULL   0x0
#define EM32_PINCFG_PULL_UP   0x1
#define EM32_PINCFG_PULL_DOWN 0x2

/* Output type values */
#define EM32_PINCFG_PUSH_PULL  0x0
#define EM32_PINCFG_OPEN_DRAIN 0x1

/**
 * @brief Configure GPIO pin from pinctrl driver
 *
 * This function is called by the pinctrl driver to configure a GPIO pin
 * with the specified multiplexing and electrical settings.
 *
 * @param dev GPIO port device (gpioa or gpiob)
 * @param pin Pin number (0-15)
 * @param conf Pin configuration (mode, type, speed, pull, drive)
 *             encoded as bit fields per EM32_PINCFG_* definitions
 * @param func Alternate function number (0=GPIO, 1-7=AF1-AF7)
 *
 * @return 0 on success, negative errno on failure
 */
int gpio_em32_configure(const struct device *dev, gpio_pin_t pin, uint32_t conf, uint32_t func);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_GPIO_GPIO_EM32_H_ */
