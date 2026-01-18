/*
 * Copyright (c) 2025 Synaptics, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MISC_SYNA_AON_EVENT_GPIO_H_
#define ZEPHYR_DRIVERS_MISC_SYNA_AON_EVENT_GPIO_H_

#include <zephyr/device.h>
#include <stdint.h>

struct syna_aon_gpio_config {
	uint8_t event;
	uint8_t pulse_width;
	uint8_t polarity;
};

int syna_aon_gpio_configure(const struct device *dev, uint8_t gpo,
			    const struct syna_aon_gpio_config *config);

#endif
