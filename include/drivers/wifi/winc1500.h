/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_WIFI_WINC1500_H_
#define ZEPHYR_INCLUDE_DRIVERS_WIFI_WINC1500_H_

#include <device.h>

enum winc1500_gpio_index {
	WINC1500_GPIO_IDX_CHIP_EN = 0,
	WINC1500_GPIO_IDX_IRQN,
	WINC1500_GPIO_IDX_RESET_N,

	WINC1500_GPIO_IDX_MAX
};

struct winc1500_gpio_configuration {
	struct device *dev;
	u32_t pin;
};

struct winc1500_gpio_configuration *winc1500_configure_gpios(void);

#endif	/* ZEPHYR_INCLUDE_DRIVERS_WIFI_WINC1500_H_ */
