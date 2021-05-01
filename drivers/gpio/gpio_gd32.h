/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2021 Tokita, Hiroshi <tokita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GPIO_GPIO_GD32_H_
#define ZEPHYR_DRIVERS_GPIO_GPIO_GD32_H_

/**
 * @file header for GD32 GPIO
 */

#include <drivers/gpio.h>

/* GPIO buses definitions */

#define GD32_PORT_NOT_AVAILABLE 0xFFFFFFFF

#define GD32_MODE_INOUT_MASK 0x3
#define GD32_MODE_INOUT_SHIFT 0x0

#define GD32_CNF_IN_MASK 0x3
#define GD32_CNF_IN_SHIFT 2

#define GD32_CNF_IN_ANALOG 0
#define GD32_CNF_IN_FLOAT  0x4
#define GD32_CNF_IN_PUD    0x8

#define GD32_MODE_INPUT 0

#define GD32_PUPD_MASK 0x3
#define GD32_PUPD_SHIFT 4

#define GD32_PUPD_PULL_UP   0x20
#define GD32_PUPD_PULL_DOWN 0x40

#define GD32_CNF_OUT_1_MASK 0x1
#define GD32_CNF_OUT_1_SHIFT 4
#define GD32_MODE_OSPEED_MASK 0x3
#define GD32_MODE_OSPEED_SHIFT 0

#define GD32_MODE_OUTPUT_MAX_10 0x1
#define GD32_MODE_OUTPUT_MAX_2  0x2
#define GD32_MODE_OUTPUT_MAX_50 0x3

#define GD32_CNF_OUT_MASK 0x3
#define GD32_CNF_OUT_SHIFT 2

#define GD32_CNF_OUT_PP (0x0 << GD32_CNF_OUT_SHIFT)
#define GD32_CNF_OUT_OD (0x1 << GD32_CNF_OUT_SHIFT)
#define GD32_CNF_AF_PP  (0x2 << GD32_CNF_OUT_SHIFT)
#define GD32_CNF_AF_OD  (0x3 << GD32_CNF_OUT_SHIFT)

#define AFIO_EXTI_PORT_MASK     ((uint8_t)0x70)
#define AFIO_EXTI_SOURCE_FIELDS ((uint8_t)0x04U) /*!< select AFIO exti source registers */

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
