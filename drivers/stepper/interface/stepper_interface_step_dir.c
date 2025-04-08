/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Andre Stefanov <mail@andrestefanov.de>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stepper_interface_step_dir.h"

#include <zephyr/drivers/stepper.h>
#include <zephyr/drivers/gpio.h>
#include "../stepper_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stepper_interface_step_dir, CONFIG_STEPPER_LOG_LEVEL);

#define STEPPER_COMMON_FROM_DEV(dev) ((const struct stepper_common_config_base *)dev->config)
#define STEPPER_CONFIG_FROM_DEV(dev) (STEPPER_COMMON_FROM_DEV(dev)->config)
#define INTERFACE_FROM_DEV(dev)                                                                    \
	((const struct stepper_interface_step_dir *)STEPPER_CONFIG_FROM_DEV(dev)->interface)

int step_dir_interface_init(const struct device *dev)
{
	const struct stepper_interface_step_dir *interface = INTERFACE_FROM_DEV(dev);
	int ret = 0;

	if (!gpio_is_ready_dt(&interface->en_pin)) {
		LOG_ERR("Enable pin is not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&interface->step_pin)) {
		LOG_ERR("Step pin is not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&interface->dir_pin)) {
		LOG_ERR("Dir pin is not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&interface->en_pin, GPIO_OUTPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure enable pin: %d", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&interface->step_pin, GPIO_OUTPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure step pin: %d", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&interface->dir_pin, GPIO_OUTPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure dir pin: %d", ret);
		return ret;
	}

	return 0;
}

int step_dir_interface_step(const struct device *dev)
{
	const struct stepper_interface_step_dir *interface = INTERFACE_FROM_DEV(dev);
	int ret = 0;

	ret = gpio_pin_set_dt(&interface->step_pin, 1);
	if (ret < 0) {
		LOG_ERR("Failed to set step pin: %d", ret);
		return ret;
	}

	ret = gpio_pin_set_dt(&interface->step_pin, 0);
	if (ret < 0) {
		LOG_ERR("Failed to set step pin: %d", ret);
		return ret;
	}

	return 0;
}

int step_dir_interface_set_dir(const struct device *dev, const enum stepper_direction direction)
{
	const struct stepper_interface_step_dir *interface = INTERFACE_FROM_DEV(dev);
	int ret = 0;

	int dir_value = (direction == STEPPER_DIRECTION_POSITIVE) ? 1 : 0;

	if (interface->invert_direction) {
		dir_value = 1 - dir_value;
	}

	ret = gpio_pin_set_dt(&interface->dir_pin, dir_value);
	if (ret < 0) {
		LOG_ERR("Failed to set direction pin: %d", ret);
		return ret;
	}

	return 0;
}

int step_dir_interface_enable(const struct device *dev)
{
	const struct stepper_interface_step_dir *interface = INTERFACE_FROM_DEV(dev);

	LOG_DBG("Enabling Stepper motor controller %s", dev->name);
	return gpio_pin_set_dt(&interface->en_pin, 1);
}

int step_dir_interface_disable(const struct device *dev)
{
	const struct stepper_interface_step_dir *interface = INTERFACE_FROM_DEV(dev);

	LOG_DBG("Disabling Stepper motor controller %s", dev->name);
	return gpio_pin_set_dt(&interface->en_pin, 0);
}
