/*
 * Copyright (c) 2022 Caspar Friedrich <c.s.w.friedrich@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "w1_ds2482_84_common.h"

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/w1.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

#define DT_DRV_COMPAT maxim_ds2484

LOG_MODULE_REGISTER(ds2484, CONFIG_W1_LOG_LEVEL);

struct ds2484_config {
	struct w1_master_config w1_config;
	const struct i2c_dt_spec i2c_spec;
	const struct gpio_dt_spec slpz_spec;
	bool apu;
};

struct ds2484_data {
	struct w1_master_data w1_data;
	uint8_t reg_device_config;
};

static int ds2484_reset_bus(const struct device *dev)
{
	const struct ds2484_config *config = dev->config;

	return ds2482_84_reset_bus(&config->i2c_spec);
}

static int ds2484_read_bit(const struct device *dev)
{
	const struct ds2484_config *config = dev->config;

	return ds2482_84_read_bit(&config->i2c_spec);
}

static int ds2484_write_bit(const struct device *dev, bool bit)
{
	const struct ds2484_config *config = dev->config;

	return ds2482_84_write_bit(&config->i2c_spec, bit);
}

static int ds2484_read_byte(const struct device *dev)
{
	const struct ds2484_config *config = dev->config;

	return ds2482_84_read_byte(&config->i2c_spec);
}

static int ds2484_write_byte(const struct device *dev, uint8_t byte)
{
	const struct ds2484_config *config = dev->config;

	return ds2482_84_write_byte(&config->i2c_spec, byte);
}

static int ds2484_configure(const struct device *dev, enum w1_settings_type type, uint32_t value)
{
	const struct ds2484_config *config = dev->config;
	struct ds2484_data *data = dev->data;

	switch (type) {
	case W1_SETTING_SPEED:
		WRITE_BIT(data->reg_device_config, DEVICE_1WS_pos, value);
		break;
	case W1_SETTING_STRONG_PULLUP:
		WRITE_BIT(data->reg_device_config, DEVICE_SPU_pos, value);
		break;
	default:
		return -EINVAL;
	}

	return ds2482_84_write_config(&config->i2c_spec, data->reg_device_config);
}

#ifdef CONFIG_PM_DEVICE
static int ds2484_pm_control(const struct device *dev, enum pm_device_action action)
{
	const struct ds2484_config *config = dev->config;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		if (!config->slpz_spec.port) {
			return -ENOTSUP;
		}
		return gpio_pin_set_dt(&config->slpz_spec, 1);
	case PM_DEVICE_ACTION_RESUME:
		if (!config->slpz_spec.port) {
			return -ENOTSUP;
		}
		return gpio_pin_set_dt(&config->slpz_spec, 0);
	default:
		return -ENOTSUP;
	};

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static int ds2484_init(const struct device *dev)
{
	int ret;

	const struct ds2484_config *config = dev->config;
	struct ds2484_data *data = dev->data;

	if (config->slpz_spec.port) {
		if (!gpio_is_ready_dt(&config->slpz_spec)) {
			LOG_ERR("Port (SLPZ) not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->slpz_spec, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Pin configuration (SLPZ) failed: %d", ret);
			return ret;
		}
	}

	if (!device_is_ready(config->i2c_spec.bus)) {
		return -ENODEV;
	}

	ret = ds2482_84_reset_device(&config->i2c_spec);
	if (ret < 0) {
		LOG_ERR("Device reset failed: %d", ret);
		return ret;
	}

	WRITE_BIT(data->reg_device_config, DEVICE_APU_pos, config->apu);

	ret = ds2482_84_write_config(&config->i2c_spec, data->reg_device_config);
	if (ret < 0) {
		LOG_ERR("Device config update failed: %d", ret);
		return ret;
	}

	return 0;
}

static const struct w1_driver_api ds2484_driver_api = {
	.reset_bus = ds2484_reset_bus,
	.read_bit = ds2484_read_bit,
	.write_bit = ds2484_write_bit,
	.read_byte = ds2484_read_byte,
	.write_byte = ds2484_write_byte,
	.configure = ds2484_configure,
};

#define DS2484_INIT(inst)                                                                          \
	static const struct ds2484_config inst_##inst##_config = {                                 \
		.w1_config.slave_count = W1_INST_SLAVE_COUNT(inst),                                \
		.i2c_spec = I2C_DT_SPEC_INST_GET(inst),                                            \
		.slpz_spec = GPIO_DT_SPEC_INST_GET_OR(inst, slpz_gpios, {0}),                      \
		.apu = DT_INST_PROP(inst, active_pullup),                                          \
	};                                                                                         \
	static struct ds2484_data inst_##inst##_data;                                              \
	PM_DEVICE_DT_INST_DEFINE(inst, ds2484_pm_control);                                         \
	DEVICE_DT_INST_DEFINE(inst, ds2484_init, PM_DEVICE_DT_INST_GET(inst), &inst_##inst##_data, \
			      &inst_##inst##_config, POST_KERNEL, CONFIG_W1_INIT_PRIORITY,         \
			      &ds2484_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DS2484_INIT)

/*
 * Make sure that this driver is not initialized before the i2c bus is available
 */
BUILD_ASSERT(CONFIG_W1_INIT_PRIORITY > CONFIG_I2C_INIT_PRIORITY);
