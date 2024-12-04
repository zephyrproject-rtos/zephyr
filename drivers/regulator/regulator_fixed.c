/*
 * Copyright 2019-2020 Peter Bigot Consulting, LLC
 * Copyright 2022 Nordic Semiconductor ASA
 * Copyright 2023 EPAM Systems
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT regulator_fixed

#include <stdint.h>

#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(regulator_fixed, CONFIG_REGULATOR_LOG_LEVEL);

struct regulator_fixed_config {
	struct regulator_common_config common;
	struct gpio_dt_spec enable;
};

struct regulator_fixed_data {
	struct regulator_common_data common;
};

static int regulator_fixed_enable(const struct device *dev)
{
	const struct regulator_fixed_config *cfg = dev->config;
	int ret;

	if (!cfg->enable.port) {
		return -ENOTSUP;
	}

	ret = gpio_pin_set_dt(&cfg->enable, 1);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int regulator_fixed_disable(const struct device *dev)
{
	const struct regulator_fixed_config *cfg = dev->config;

	if (!cfg->enable.port) {
		return -ENOTSUP;
	}

	return gpio_pin_set_dt(&cfg->enable, 0);
}

static unsigned int regulator_fixed_count_voltages(const struct device *dev)
{
	int32_t min_uv;

	return (regulator_common_get_min_voltage(dev, &min_uv) < 0) ? 0U : 1U;
}

static int regulator_fixed_list_voltage(const struct device *dev,
					unsigned int idx,
					int32_t *volt_uv)
{
	if (idx != 0) {
		return -EINVAL;
	}

	if (regulator_common_get_min_voltage(dev, volt_uv) < 0) {
		return -EINVAL;
	}

	return 0;
}

static DEVICE_API(regulator, regulator_fixed_api) = {
	.enable = regulator_fixed_enable,
	.disable = regulator_fixed_disable,
	.count_voltages = regulator_fixed_count_voltages,
	.list_voltage = regulator_fixed_list_voltage,
};

static int regulator_fixed_init(const struct device *dev)
{
	const struct regulator_fixed_config *cfg = dev->config;

	regulator_common_data_init(dev);

	if (cfg->enable.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->enable)) {
			LOG_ERR("GPIO port: %s not ready", cfg->enable.port->name);
			return -ENODEV;
		}

		int ret = gpio_pin_configure_dt(&cfg->enable, GPIO_OUTPUT);

		if (ret < 0) {
			return ret;
		}
	}

	return regulator_common_init(dev, false);
}

#define REGULATOR_FIXED_DEFINE(inst)                                              \
	BUILD_ASSERT(DT_INST_PROP_OR(inst, regulator_min_microvolt, 0) ==         \
		     DT_INST_PROP_OR(inst, regulator_max_microvolt, 0),           \
		     "Regulator requires fixed voltages");                        \
	static struct regulator_fixed_data data##inst;                            \
                                                                                  \
	static const struct regulator_fixed_config config##inst = {               \
		.common = REGULATOR_DT_INST_COMMON_CONFIG_INIT(inst),             \
		.enable = GPIO_DT_SPEC_INST_GET_OR(inst, enable_gpios, {0}),      \
	};                                                                        \
                                                                                  \
	DEVICE_DT_INST_DEFINE(inst, regulator_fixed_init, NULL, &data##inst,      \
			      &config##inst, POST_KERNEL,                         \
			      CONFIG_REGULATOR_FIXED_INIT_PRIORITY,               \
			      &regulator_fixed_api);

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_FIXED_DEFINE)
