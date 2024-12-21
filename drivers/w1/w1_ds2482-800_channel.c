/*
 * Copyright (c) 2023 Caspar Friedrich <c.s.w.friedrich@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "w1_ds2482-800.h"
#include "w1_ds2482_84_common.h"

#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/w1.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT maxim_ds2482_800_channel

LOG_MODULE_DECLARE(ds2482, CONFIG_W1_LOG_LEVEL);

struct ds2482_config {
	struct w1_master_config w1_config;
	const struct device *parent;
	const struct i2c_dt_spec i2c_spec;
	uint8_t reg_channel;
	uint8_t reg_channel_rb;
	uint8_t reg_config;
};

struct ds2482_data {
	struct w1_master_data w1_data;
};

static int ds2482_reset_bus(const struct device *dev)
{
	const struct ds2482_config *config = dev->config;

	return ds2482_84_reset_bus(&config->i2c_spec);
}

static int ds2482_read_bit(const struct device *dev)
{
	const struct ds2482_config *config = dev->config;

	return ds2482_84_read_bit(&config->i2c_spec);
}

static int ds2482_write_bit(const struct device *dev, bool bit)
{
	const struct ds2482_config *config = dev->config;

	return ds2482_84_write_bit(&config->i2c_spec, bit);
}

static int ds2482_read_byte(const struct device *dev)
{
	const struct ds2482_config *config = dev->config;

	return ds2482_84_read_byte(&config->i2c_spec);
}

static int ds2482_write_byte(const struct device *dev, uint8_t byte)
{
	const struct ds2482_config *config = dev->config;

	return ds2482_84_write_byte(&config->i2c_spec, byte);
}

static int ds2482_configure(const struct device *dev, enum w1_settings_type type, uint32_t value)
{
	const struct ds2482_config *config = dev->config;

	uint8_t reg_config = config->reg_config;

	switch (type) {
	case W1_SETTING_SPEED:
		WRITE_BIT(reg_config, DEVICE_1WS_pos, value);
		break;
	case W1_SETTING_STRONG_PULLUP:
		WRITE_BIT(reg_config, DEVICE_SPU_pos, value);
		break;
	default:
		return -EINVAL;
	}

	return ds2482_84_write_config(&config->i2c_spec, reg_config);
}

static int ds2482_set_channel(const struct i2c_dt_spec *spec, uint8_t channel, uint8_t channel_rb)
{
	int ret;

	uint8_t reg = channel;

	ret = ds2482_84_write(spec, CMD_CHSL, &reg);
	if (ret < 0) {
		return ret;
	}

	ret = ds2482_84_read(spec, REG_NONE, &reg);
	if (ret < 0) {
		return ret;
	}

	return (reg == channel_rb) ? 0 : -EIO;
}

static int ds2482_change_bus_lock(const struct device *dev, bool lock)
{
	int ret;

	const struct ds2482_config *config = dev->config;

	ret = ds2482_change_bus_lock_impl(config->parent, lock);
	if (ret < 0) {
		LOG_ERR("Failed to acquire bus lock: %d", ret);
		return ret;
	}

	if (!lock) {
		return 0;
	}

	/*
	 * Set channel for subsequent operations
	 */
	ret = ds2482_set_channel(&config->i2c_spec, config->reg_channel, config->reg_channel_rb);
	if (ret < 0) {
		return ret;
	}

	/*
	 * Restore default channel configuration
	 */
	ret = ds2482_84_write_config(&config->i2c_spec, config->reg_config);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int ds2482_init(const struct device *dev)
{
	const struct ds2482_config *config = dev->config;

	if (!device_is_ready(config->parent)) {
		return -ENODEV;
	}

	return 0;
}

static const struct w1_driver_api ds2482_driver_api = {
	.reset_bus = ds2482_reset_bus,
	.read_bit = ds2482_read_bit,
	.write_bit = ds2482_write_bit,
	.read_byte = ds2482_read_byte,
	.write_byte = ds2482_write_byte,
	.configure = ds2482_configure,
	.change_bus_lock = ds2482_change_bus_lock,
};

#define DS2482_CHANNEL_INIT(inst)                                                                  \
	static const struct ds2482_config inst_##inst##_config = {                                 \
		.w1_config.slave_count = W1_INST_SLAVE_COUNT(inst),                                \
		.parent = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                     \
		.i2c_spec = I2C_DT_SPEC_GET(DT_INST_PARENT(inst)),                                 \
		.reg_channel = UTIL_CAT(CHSL_IO, DT_INST_REG_ADDR_RAW(inst)),                      \
		.reg_channel_rb = UTIL_CAT(CHSL_RB_IO, DT_INST_REG_ADDR_RAW(inst)),                \
		.reg_config = DT_INST_PROP(inst, active_pullup) << DEVICE_APU_pos,                 \
	};                                                                                         \
	static struct ds2482_data inst_##inst##_data = {0};                                        \
	DEVICE_DT_INST_DEFINE(inst, ds2482_init, NULL, &inst_##inst##_data, &inst_##inst##_config, \
			      POST_KERNEL, CONFIG_W1_INIT_PRIORITY, &ds2482_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DS2482_CHANNEL_INIT)

/*
 * Make sure that this driver is not initialized before the i2c bus is available
 */
BUILD_ASSERT(CONFIG_W1_INIT_PRIORITY > CONFIG_I2C_INIT_PRIORITY);
