/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "step_dir_stepper_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(step_dir_stepper, CONFIG_STEPPER_LOG_LEVEL);

int step_dir_stepper_common_step(const struct device *dev)
{
	const struct step_dir_stepper_common_config *config = dev->config;
	int ret = gpio_pin_toggle_dt(&config->step_pin);
	if (ret < 0) {
		LOG_ERR("Failed to toggle step pin: %d", ret);
		return ret;
	}

	if (!config->dual_edge) {
		ret = gpio_pin_toggle_dt(&config->step_pin);
		if (ret < 0) {
			LOG_ERR("Failed to toggle step pin: %d", ret);
			return ret;
		}
	}

	return 0;
}

int step_dir_stepper_common_set_direction(const struct device *dev,
					  const enum stepper_direction dir)
{
	const struct step_dir_stepper_common_config *config = dev->config;
	int ret;

	switch (dir) {
	case STEPPER_DIRECTION_POSITIVE:
		ret = gpio_pin_set_dt(&config->dir_pin, 1 ^ config->invert_direction);
		break;
	case STEPPER_DIRECTION_NEGATIVE:
		ret = gpio_pin_set_dt(&config->dir_pin, 0 ^ config->invert_direction);
		break;
	default:
		LOG_ERR("Unsupported direction: %d", dir);
		return -ENOTSUP;
	}

	return ret;
}

int step_dir_stepper_common_init(const struct device *dev)
{
	const struct step_dir_stepper_common_config *config = dev->config;
	int ret;

	if (!gpio_is_ready_dt(&config->step_pin) || !gpio_is_ready_dt(&config->dir_pin)) {
		LOG_ERR("GPIO pins are not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->step_pin, GPIO_OUTPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure step pin: %d", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&config->dir_pin, GPIO_OUTPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure dir pin: %d", ret);
		return ret;
	}

	return 0;
}
