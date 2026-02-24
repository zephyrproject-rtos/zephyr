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
#include <zephyr/drivers/pinctrl.h>

#include <stm32_gpio_shared.h>

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

#endif /* ZEPHYR_DRIVERS_GPIO_GPIO_STM32_H_ */
