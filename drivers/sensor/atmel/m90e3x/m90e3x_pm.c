/*
 * Copyright (c) 2025, Atmel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(m90e3x_pm, CONFIG_PM_DEVICE_LOG_LEVEL);

#include "m90e3x.h"

static int m90e3x_pm_idle_mode(const struct device *dev)
{
	LOG_DBG("Entering IDLE power mode.");

	int ret = 0;
	const struct m90e3x_config *cfg = (struct m90e3x_config *)dev->config;
	const struct m90e3x_data *data = (struct m90e3x_data *)dev->data;

	if (data->current_power_mode == M90E3X_IDLE) {
		LOG_DBG("Device %s is already in IDLE power mode.", dev->name);
		return 0;
	}

	ret = gpio_pin_set_dt(&cfg->pm0, M90E3X_PM0_IDLE_BIT);
	if (ret < 0) {
		return ret;
	}
	ret = gpio_pin_set_dt(&cfg->pm1, M90E3X_PM1_IDLE_BIT);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static int m90e3x_pm_detection_mode(const struct device *dev)
{
	LOG_DBG("Entering DETECTION power mode.");

	int ret = 0;
	const struct m90e3x_config *cfg = (struct m90e3x_config *)dev->config;
	const struct m90e3x_data *data = (struct m90e3x_data *)dev->data;

	if (data->current_power_mode == M90E3X_DETECTION) {
		LOG_DBG("Device %s is already in DETECTION power mode.", dev->name);
		return 0;
	}

	ret = gpio_pin_set_dt(&cfg->pm0, M90E3X_PM0_IDLE_BIT);
	if (ret < 0) {
		return ret;
	}
	ret = gpio_pin_set_dt(&cfg->pm1, M90E3X_PM1_IDLE_BIT);
	if (ret < 0) {
		return ret;
	}

	k_msleep(1);

	ret = gpio_pin_set_dt(&cfg->pm0, M90E3X_PM0_DETECTION_BIT);
	if (ret < 0) {
		return ret;
	}
	ret = gpio_pin_set_dt(&cfg->pm1, M90E3X_PM1_DETECTION_BIT);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static int m90e3x_pm_partial_measurement_mode(const struct device *dev)
{
	LOG_DBG("Entering PARTIAL MEASUREMENT power mode.");

	int ret = 0;
	const struct m90e3x_config *cfg = (struct m90e3x_config *)dev->config;
	const struct m90e3x_data *data = (struct m90e3x_data *)dev->data;

	if (data->current_power_mode == M90E3X_PARTIAL) {
		LOG_DBG("Device %s is already in PARTIAL MEASUREMENT power mode.", dev->name);
		return 0;
	}

	ret = gpio_pin_set_dt(&cfg->pm0, M90E3X_PM0_IDLE_BIT);
	if (ret < 0) {
		return ret;
	}
	ret = gpio_pin_set_dt(&cfg->pm1, M90E3X_PM1_IDLE_BIT);
	if (ret < 0) {
		return ret;
	}

	k_msleep(1);

	ret = gpio_pin_set_dt(&cfg->pm0, M90E3X_PM0_PARTIAL_MEASUREMENT_BIT);
	if (ret < 0) {
		return ret;
	}
	ret = gpio_pin_set_dt(&cfg->pm1, M90E3X_PM1_PARTIAL_MEASUREMENT_BIT);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static int m90e3x_pm_normal_mode(const struct device *dev)
{
	LOG_DBG("Entering NORMAL power mode.");

	int ret = 0;
	const struct m90e3x_config *cfg = (struct m90e3x_config *)dev->config;
	const struct m90e3x_data *data = (struct m90e3x_data *)dev->data;

	if (data->current_power_mode == M90E3X_NORMAL) {
		LOG_DBG("Device %s is already in NORMAL power mode.", dev->name);
		return 0;
	}

	ret = gpio_pin_set_dt(&cfg->pm0, M90E3X_PM0_IDLE_BIT);
	if (ret < 0) {
		return ret;
	}
	ret = gpio_pin_set_dt(&cfg->pm1, M90E3X_PM1_IDLE_BIT);
	if (ret < 0) {
		return ret;
	}

	k_msleep(1);

	ret = gpio_pin_set_dt(&cfg->pm0, M90E3X_PM0_NORMAL_BIT);
	if (ret < 0) {
		return ret;
	}
	ret = gpio_pin_set_dt(&cfg->pm1, M90E3X_PM1_NORMAL_BIT);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

const struct m90e3x_pm_mode_ops m90e3x_pm_mode = {
	.enter_idle_mode = m90e3x_pm_idle_mode,
	.enter_detection_mode = m90e3x_pm_detection_mode,
	.enter_partial_measurement_mode = m90e3x_pm_partial_measurement_mode,
	.enter_normal_mode = m90e3x_pm_normal_mode,
};
