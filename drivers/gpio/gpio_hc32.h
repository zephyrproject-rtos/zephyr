/*
 * Copyright (C) 2024-2025, Xiaohua Semiconductor Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GPIO_GPIO_HC32_H_
#define ZEPHYR_DRIVERS_GPIO_GPIO_HC32_H_

/**
 * @file header for HC32 GPIO
 */

#include <hc32_ll.h>
#include <zephyr/drivers/gpio.h>
/**
 * @brief configuration of GPIO device
 */
struct gpio_hc32_config {
	struct gpio_driver_config common;
	/* port PCRxy register base address */
	uint16_t *base;
	/* IO port */
	uint8_t port;
};

/**
 * @brief driver data
 */
struct gpio_hc32_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* device's owner of this data */
	const struct device *dev;
	/* user ISR cb */
	sys_slist_t cb;
};

#endif /* ZEPHYR_DRIVERS_GPIO_GPIO_HC32_H_ */
