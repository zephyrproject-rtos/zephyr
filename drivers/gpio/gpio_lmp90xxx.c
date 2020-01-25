/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief GPIO driver for the LMP90xxx AFE.
 */

#include <drivers/gpio.h>
#include <zephyr.h>

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(gpio_lmp90xxx);

#include <drivers/adc/lmp90xxx.h>

#include "gpio_utils.h"

struct gpio_lmp90xxx_config {
	char *parent_dev_name;
};

struct gpio_lmp90xxx_data {
	struct device *parent;
};

static int gpio_lmp90xxx_config(struct device *dev, int access_op,
				u32_t pin, int flags)
{
	struct gpio_lmp90xxx_data *data = dev->driver_data;
	int err;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	if (pin > LMP90XXX_GPIO_MAX) {
		return -EINVAL;
	}

	if (flags & GPIO_INT) {
		/* LMP90xxx GPIOs do not support interrupts */
		return -ENOTSUP;
	}

	if (flags & GPIO_DIR_OUT) {
		err = lmp90xxx_gpio_set_output(data->parent, pin);
	} else {
		err = lmp90xxx_gpio_set_input(data->parent, pin);
	}

	return err;
}

static int gpio_lmp90xxx_write(struct device *dev, int access_op,
			       u32_t pin, u32_t value)
{
	struct gpio_lmp90xxx_data *data = dev->driver_data;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	if (pin > LMP90XXX_GPIO_MAX) {
		return -EINVAL;
	}

	return lmp90xxx_gpio_set_pin_value(data->parent, pin,
					   value ? true : false);
}

static int gpio_lmp90xxx_read(struct device *dev, int access_op,
			      u32_t pin, u32_t *value)
{
	struct gpio_lmp90xxx_data *data = dev->driver_data;
	bool set;
	int err;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	if (pin > LMP90XXX_GPIO_MAX) {
		return -EINVAL;
	}

	err = lmp90xxx_gpio_get_pin_value(data->parent, pin, &set);
	if (!err) {
		*value = set ? 1 : 0;
	}

	return err;
}

static int gpio_lmp90xxx_init(struct device *dev)
{
	const struct gpio_lmp90xxx_config *config = dev->config->config_info;
	struct gpio_lmp90xxx_data *data = dev->driver_data;

	data->parent = device_get_binding(config->parent_dev_name);
	if (!data->parent) {
		LOG_ERR("parent LMP90xxx device '%s' not found",
			config->parent_dev_name);
		return -EINVAL;
	}

	return 0;
}

static const struct gpio_driver_api gpio_lmp90xxx_api = {
	.config = gpio_lmp90xxx_config,
	.write = gpio_lmp90xxx_write,
	.read = gpio_lmp90xxx_read,
};

BUILD_ASSERT_MSG(CONFIG_GPIO_LMP90XXX_INIT_PRIORITY >
		 CONFIG_ADC_LMP90XXX_INIT_PRIORITY,
		 "LMP90xxx GPIO driver must be initialized after LMP90xxx ADC "
		 "driver");

#define GPIO_LMP90XXX_DEVICE(id)					\
	static const struct gpio_lmp90xxx_config gpio_lmp90xxx_##id##_cfg = {\
		.parent_dev_name =					\
			DT_INST_##id##_TI_LMP90XXX_GPIO_BUS_NAME,	\
	};								\
									\
	static struct gpio_lmp90xxx_data gpio_lmp90xxx_##id##_data;	\
									\
	DEVICE_AND_API_INIT(gpio_lmp90xxx_##id,				\
			    DT_INST_##id##_TI_LMP90XXX_GPIO_LABEL,	\
			    &gpio_lmp90xxx_init,			\
			    &gpio_lmp90xxx_##id##_data,			\
			    &gpio_lmp90xxx_##id##_cfg, POST_KERNEL,	\
			    CONFIG_GPIO_LMP90XXX_INIT_PRIORITY,		\
			    &gpio_lmp90xxx_api)

#ifdef DT_INST_0_TI_LMP90XXX_GPIO
GPIO_LMP90XXX_DEVICE(0);
#endif
