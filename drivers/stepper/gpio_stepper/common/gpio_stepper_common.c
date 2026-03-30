/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "gpio_stepper_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_stepper_common, CONFIG_STEPPER_LOG_LEVEL);

int gpio_stepper_common_init(const struct device *dev)
{
	const struct gpio_stepper_common_config *config = dev->config;
	int ret;

	if (config->timing_source->init) {
		ret = config->timing_source->init(dev);
		if (ret < 0) {
			LOG_ERR("Failed to initialize timing source: %d", ret);
			return ret;
		}
	}

	return stepper_ctrl_event_handler_init(dev);
}
