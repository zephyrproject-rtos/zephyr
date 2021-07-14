/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GPIO_GPIO_GD32_H_
#define ZEPHYR_DRIVERS_GPIO_GPIO_GD32_H_

/**
 * @file header for GD32 GPIO
 */

#include <drivers/clock_control/gd32_clock_control.h>
#include <pinmux/pinmux_gd32.h>
#include <drivers/gpio.h>

/* GPIO buses definitions */

#define GD32_PORT_NOT_AVAILABLE 0xFFFFFFFF

#if CONFIG_SOC_SERIES_GD32E1X
#define GD32_CLOCK_BUS_GPIO GD32_CLOCK_BUS_APB2
#define GD32_PERIPH_GPIOA LL_APB2_GRP1_PERIPH_GPIOA
#define GD32_PERIPH_GPIOB LL_APB2_GRP1_PERIPH_GPIOB
#define GD32_PERIPH_GPIOC LL_APB2_GRP1_PERIPH_GPIOC
#define GD32_PERIPH_GPIOD LL_APB2_GRP1_PERIPH_GPIOD
#define GD32_PERIPH_GPIOE LL_APB2_GRP1_PERIPH_GPIOE
#define GD32_PERIPH_GPIOF LL_APB2_GRP1_PERIPH_GPIOF
#define GD32_PERIPH_GPIOG LL_APB2_GRP1_PERIPH_GPIOG
#endif

#if CONFIG_SOC_SERIES_GD32E1X
#define GD32_PINCFG_MODE_OUTPUT        (GD32_MODE_OUTPUT     \
					 | GD32_CNF_GP_OUTPUT \
					 | GD32_CNF_PUSH_PULL)
#define GD32_PINCFG_MODE_INPUT         GD32_MODE_INPUT
#define GD32_PINCFG_MODE_ANALOG        (GD32_MODE_INPUT	\
					 | GD32_CNF_IN_ANALOG)
#define GD32_PINCFG_PUSH_PULL          GD32_CNF_PUSH_PULL
#define GD32_PINCFG_OPEN_DRAIN         GD32_CNF_OPEN_DRAIN
#define GD32_PINCFG_PULL_UP            (GD32_CNF_IN_PUPD | GD32_PUPD_PULL_UP)
#define GD32_PINCFG_PULL_DOWN          (GD32_CNF_IN_PUPD | \
					GD32_PUPD_PULL_DOWN)
#define GD32_PINCFG_FLOATING           (GD32_CNF_IN_FLOAT | \
					GD32_PUPD_NO_PULL)
#endif

/**
 * @brief configuration of GPIO device
 */
struct gpio_gd32_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	/* port base address */
	uint32_t *base;
	/* IO port */
	int port;
	struct gd32_pclken pclken;
};

/**
 * @brief driver data
 */
struct gpio_gd32_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* device's owner of this data */
	const struct device *dev;
	/* user ISR cb */
	sys_slist_t cb;
#ifdef CONFIG_PM_DEVICE
	/* device power state */
	enum pm_device_state power_state;
#endif
};

/**
 * @brief helper for configuration of GPIO pin
 *
 * @param dev GPIO port device pointer
 * @param pin IO pin
 * @param conf GPIO mode
 * @param altf Alternate function
 */
int gpio_gd32_configure(const struct device *dev, int pin, int conf, int altf);

/**
 * @brief Enable / disable GPIO port clock.
 *
 * @param dev GPIO port device pointer
 * @param on boolean for on/off clock request
 */
int gpio_gd32_clock_request(const struct device *dev, bool on);

#endif /* ZEPHYR_DRIVERS_GPIO_GPIO_GD32_H_ */
