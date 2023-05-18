/*
 * Copyright (c) 2023 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GPIO_GPIO_MAX32_H_
#define ZEPHYR_DRIVERS_GPIO_GPIO_MAX32_H_

#include <zephyr/device.h>

int gpio_max32_config_pinmux(const struct device *dev, int pin, int pinmux, int pincfg);

#endif
