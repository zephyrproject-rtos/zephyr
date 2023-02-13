/*
 * Copyright 2019-2020 Peter Bigot Consulting, LLC
 * Copyright 2022 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT regulator_fixed

#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(regulator_fixed, CONFIG_REGULATOR_LOG_LEVEL);

struct regulator_fixed_config {
	struct regulator_common_config common;
	uint32_t startup_delay_us;
	uint32_t off_on_delay_us;
	struct gpio_dt_spec enable;
};

struct regulator_fixed_data {
	struct regulator_common_data common;
};

static int regulator_fixed_enable(const struct device *dev)
{
	const struct regulator_fixed_config *cfg = dev->config;
	int ret;

	ret = gpio_pin_set_dt(&cfg->enable, 1);
	if (ret < 0) {
		return ret;
	}

	if (cfg->off_on_delay_us > 0U) {
		k_sleep(K_USEC(cfg->off_on_delay_us));
	}

	return 0;
}

static int regulator_fixed_disable(const struct device *dev)
{
	const struct regulator_fixed_config *cfg = dev->config;

	return gpio_pin_set_dt(&cfg->enable, 0);
}

static const struct regulator_driver_api regulator_fixed_api = {
	.enable = regulator_fixed_enable,
	.disable = regulator_fixed_disable,
};

static int regulator_fixed_init(const struct device *dev)
{
	const struct regulator_fixed_config *cfg = dev->config;
	int ret;

	regulator_common_data_init(dev);

	if (!device_is_ready(cfg->enable.port)) {
		LOG_ERR("GPIO port: %s not ready", cfg->enable.port->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->enable, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return ret;
	}

	ret = regulator_common_init(dev, false);
	if (ret < 0) {
		return ret;
	}

	if (regulator_is_enabled(dev)) {
		k_busy_wait(cfg->startup_delay_us);
	}

	return 0;
}

#define REGULATOR_FIXED_DEFINE(inst)                                           \
	static struct regulator_fixed_data data##inst;                         \
                                                                               \
	static const struct regulator_fixed_config config##inst = {            \
		.common = REGULATOR_DT_INST_COMMON_CONFIG_INIT(inst),          \
		.startup_delay_us = DT_INST_PROP(inst, startup_delay_us),      \
		.off_on_delay_us = DT_INST_PROP(inst, off_on_delay_us),        \
		.enable = GPIO_DT_SPEC_INST_GET(inst, enable_gpios),           \
	};                                                                     \
                                                                               \
	DEVICE_DT_INST_DEFINE(inst, regulator_fixed_init, NULL, &data##inst,   \
			      &config##inst, POST_KERNEL,                      \
			      CONFIG_REGULATOR_FIXED_INIT_PRIORITY,            \
			      &regulator_fixed_api);

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_FIXED_DEFINE)
