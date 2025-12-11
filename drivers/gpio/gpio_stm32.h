/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GPIO_GPIO_STM32_H_
#define ZEPHYR_DRIVERS_GPIO_GPIO_STM32_H_

/**
 * @file header for STM32 GPIO
 */

#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/gpio.h>
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_pinctrl)
#include <zephyr/dt-bindings/pinctrl/stm32f1-pinctrl.h>
#else
#include <zephyr/dt-bindings/pinctrl/stm32-pinctrl.h>
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_pinctrl) */

#ifdef CONFIG_SOC_SERIES_STM32F1X
#define STM32_PINCFG_MODE_OUTPUT        (STM32_MODE_OUTPUT     \
					 | STM32_CNF_GP_OUTPUT \
					 | STM32_CNF_PUSH_PULL)
#define STM32_PINCFG_MODE_INPUT         STM32_MODE_INPUT
#define STM32_PINCFG_MODE_ANALOG        (STM32_MODE_INPUT	\
					 | STM32_CNF_IN_ANALOG)
#define STM32_PINCFG_PUSH_PULL          STM32_CNF_PUSH_PULL
#define STM32_PINCFG_OPEN_DRAIN         STM32_CNF_OPEN_DRAIN
#define STM32_PINCFG_PULL_UP            (STM32_CNF_IN_PUPD | STM32_PUPD_PULL_UP)
#define STM32_PINCFG_PULL_DOWN          (STM32_CNF_IN_PUPD | \
					STM32_PUPD_PULL_DOWN)
#define STM32_PINCFG_FLOATING           (STM32_CNF_IN_FLOAT | \
					STM32_PUPD_NO_PULL)
#else
#define STM32_PINCFG_MODE_OUTPUT        STM32_MODER_OUTPUT_MODE
#define STM32_PINCFG_MODE_INPUT         STM32_MODER_INPUT_MODE
#define STM32_PINCFG_MODE_ANALOG        STM32_MODER_ANALOG_MODE
#define STM32_PINCFG_PUSH_PULL          STM32_OTYPER_PUSH_PULL
#define STM32_PINCFG_OPEN_DRAIN         STM32_OTYPER_OPEN_DRAIN
#define STM32_PINCFG_PULL_UP            STM32_PUPDR_PULL_UP
#define STM32_PINCFG_PULL_DOWN          STM32_PUPDR_PULL_DOWN
#define STM32_PINCFG_FLOATING           STM32_PUPDR_NO_PULL
#endif /* CONFIG_SOC_SERIES_STM32F1X */

#if defined(CONFIG_GPIO_GET_CONFIG) && !defined(CONFIG_SOC_SERIES_STM32F1X)
/**
 * @brief structure of a GPIO pin (stm32 LL values) use to get the configuration
 */
struct gpio_stm32_pin {
	unsigned int type; /* LL_GPIO_OUTPUT_PUSHPULL or LL_GPIO_OUTPUT_OPENDRAIN */
	unsigned int pupd; /* LL_GPIO_PULL_NO or LL_GPIO_PULL_UP or LL_GPIO_PULL_DOWN */
	unsigned int mode; /* LL_GPIO_MODE_INPUT or LL_GPIO_MODE_OUTPUT or other */
	unsigned int out_state; /* 1 (high level) or 0 (low level) */
};
#endif /* CONFIG_GPIO_GET_CONFIG */

/**
 * @brief configuration of GPIO device
 */
struct gpio_stm32_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	/* port base address */
	uint32_t *base;
	/* IO port */
	int port;
	struct stm32_pclken pclken;
};

/**
 * @brief driver data
 */
struct gpio_stm32_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* device's owner of this data */
	const struct device *dev;
	/* user ISR cb */
	sys_slist_t cb;
	/* keep track of pins that  are connected and need GPIO clock to be enabled */
	uint32_t pin_has_clock_enabled;
};

/**
 * @brief helper for configuration of GPIO pin
 *
 * @param dev GPIO port device pointer
 * @param pin IO pin
 * @param conf GPIO mode
 * @param func Pin function
 *
 * @return 0 on success, negative errno code on failure
 */
int gpio_stm32_configure(const struct device *dev, gpio_pin_t pin, uint32_t conf, uint32_t func);

#endif /* ZEPHYR_DRIVERS_GPIO_GPIO_STM32_H_ */
