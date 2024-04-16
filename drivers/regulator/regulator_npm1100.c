/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_npm1100

#include <errno.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/dt-bindings/regulator/npm1100.h>
#include <zephyr/toolchain.h>

struct regulator_npm1100_pconfig {
	struct gpio_dt_spec iset;
};

struct regulator_npm1100_config {
	struct regulator_common_config common;
	struct gpio_dt_spec mode;
};

struct regulator_npm1100_data {
	struct regulator_common_data data;
};

static int regulator_npm1100_set_mode(const struct device *dev,
				      regulator_mode_t mode)
{
	const struct regulator_npm1100_config *config = dev->config;

	if ((config->mode.port == NULL) || (mode > NPM1100_MODE_PWM)) {
		return -ENOTSUP;
	}

	return gpio_pin_set_dt(&config->mode,
			       mode == NPM1100_MODE_AUTO ? 0 : 1);
}

static int regulator_npm1100_get_mode(const struct device *dev,
				      regulator_mode_t *mode)
{
	const struct regulator_npm1100_config *config = dev->config;
	int ret;

	if (config->mode.port == NULL) {
		return -ENOTSUP;
	}

	ret = gpio_pin_get_dt(&config->mode);
	if (ret < 0) {
		return ret;
	}

	*mode = (ret == 0) ? NPM1100_MODE_AUTO : NPM1100_MODE_PWM;

	return 0;
}

static __unused int regulator_npm1100_init(const struct device *dev)
{
	const struct regulator_npm1100_config *config = dev->config;
	int ret;

	if (config->mode.port != NULL) {
		if (!gpio_is_ready_dt(&config->mode)) {
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->mode,
					    GPIO_INPUT | GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			return ret;
		}
	}

	regulator_common_data_init(dev);

	return regulator_common_init(dev, true);
}

static int regulator_npm1100_common_init(const struct device *dev)
{
	const struct regulator_npm1100_pconfig *config = dev->config;

	if (config->iset.port != NULL) {
		int ret;

		if (!gpio_is_ready_dt(&config->iset)) {
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->iset,
					    GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static const __unused struct regulator_driver_api api = {
	.set_mode = regulator_npm1100_set_mode,
	.get_mode = regulator_npm1100_get_mode,
};

#define REGULATOR_NPM1100_DEFINE_BUCK(node_id, id)                             \
	static struct regulator_npm1100_data data_##id;                        \
                                                                               \
	static const struct regulator_npm1100_config config_##id = {           \
		.common = REGULATOR_DT_COMMON_CONFIG_INIT(node_id),            \
		.mode = GPIO_DT_SPEC_GET_OR(node_id, nordic_mode_gpios, {}),   \
	};                                                                     \
                                                                               \
	DEVICE_DT_DEFINE(node_id, regulator_npm1100_init, NULL, &data_##id,    \
			 &config_##id, POST_KERNEL,                            \
			 CONFIG_REGULATOR_NPM1100_INIT_PRIORITY, &api);

#define REGULATOR_NPM1100_DEFINE_BUCK_COND(inst)                               \
	COND_CODE_1(DT_NODE_EXISTS(DT_INST_CHILD(inst, buck)),                 \
		    (REGULATOR_NPM1100_DEFINE_BUCK(DT_INST_CHILD(inst, buck),  \
						   buck##inst)),               \
		    ())

#define REGULATOR_NPM1100_DEFINE_ALL(inst)                                     \
	static const struct regulator_npm1100_pconfig config_##inst = {        \
		.iset = GPIO_DT_SPEC_INST_GET_OR(inst, nordic_iset_gpios, {}), \
	};                                                                     \
                                                                               \
	DEVICE_DT_INST_DEFINE(inst, regulator_npm1100_common_init, NULL, NULL, \
			      &config_##inst, POST_KERNEL,                     \
			      CONFIG_REGULATOR_NPM1100_INIT_PRIORITY, NULL);   \
                                                                               \
	REGULATOR_NPM1100_DEFINE_BUCK_COND(inst)

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_NPM1100_DEFINE_ALL)
