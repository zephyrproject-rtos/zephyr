/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32_GPIO_H_
#define _STM32_GPIO_H_

/**
 * @file header for STM32 GPIO
 */

#include <clock_control/stm32_clock_control.h>
#include <pinmux/stm32/pinmux_stm32.h>
#include <gpio.h>

/**
 * @brief configuration of GPIO device
 */
struct gpio_stm32_config {
	/* port base address */
	uint32_t *base;
	/* IO port */
	enum stm32_pin_port port;
#ifdef CONFIG_SOC_SERIES_STM32F4X
	struct stm32f4x_pclken pclken;
#else /* SOC_SERIES_STM32F1X || SOC_SERIES_STM32F3X || SOC_SERIES_STM32L4X */
	/* clock subsystem */
	clock_control_subsys_t clock_subsys;
#endif
};

/**
 * @brief driver data
 */
struct gpio_stm32_data {
	/* Enabled INT pins generating a cb */
	uint32_t cb_pins;
	/* user ISR cb */
	sys_slist_t cb;
};

/**
 * @brief helper for mapping of GPIO flags to SoC specific config
 *
 * @param flags GPIO encoded flags
 * @param out conf SoC specific pin config
 *
 * @return 0 if flags were mapped to SoC pin config
 */
int stm32_gpio_flags_to_conf(int flags, int *conf);

/**
 * @brief helper for configuration of GPIO pin
 *
 * @param base_addr GPIO port base address
 * @param pin IO pin
 * @param func GPIO mode
 * @param altf Alternate function
 */
int stm32_gpio_configure(uint32_t *base_addr, int pin, int func, int altf);

/**
 * @brief helper for setting of GPIO pin output
 *
 * @param base_addr GPIO port base address
 * @param pin IO pin
 * @param value 1, 0
 */
int stm32_gpio_set(uint32_t *base, int pin, int value);

/**
 * @brief helper for reading of GPIO pin value
 *
 * @param base_addr GPIO port base address
 * @param pin IO pin
 * @return pin value
 */
int stm32_gpio_get(uint32_t *base, int pin);

/**
 * @brief enable interrupt source for GPIO pin
 * @param port
 * @param pin
 */
int stm32_gpio_enable_int(int port, int pin);

#endif /* _STM32_GPIO_H_ */
