/*
 * Copyright (c) 2024 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT maxim_max31790_fan_fault

#include <zephyr/drivers/mfd/max31790.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/max31790.h>
#include <zephyr/logging/log.h>

#include "max31790_fan_fault.h"

LOG_MODULE_REGISTER(MAX31790_FAN_FAULT, CONFIG_SENSOR_LOG_LEVEL);

static int max31790_fan_fault_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct max31790_fan_fault_config *config = dev->config;
	struct max31790_fan_fault_data *data = dev->data;
	int result;
	uint8_t value;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	result = i2c_reg_read_byte_dt(&config->i2c, MAX37190_REGISTER_FANFAULTSTATUS1, &value);
	if (result != 0) {
		return result;
	}

	data->value = value & 0x3F;

	return 0;
}

static int max31790_fan_fault_channel_get(const struct device *dev, enum sensor_channel chan,
					  struct sensor_value *val)
{
	struct max31790_fan_fault_data *data = dev->data;

	if ((enum sensor_channel_max31790)chan != SENSOR_CHAN_MAX31790_FAN_FAULT) {
		LOG_ERR("%s: requesting unsupported channel %i", dev->name, chan);
		return -ENOTSUP;
	}

	val->val1 = data->value;
	val->val2 = 0;
	return 0;
}

static DEVICE_API(sensor, max31790_fan_fault_api) = {
	.sample_fetch = max31790_fan_fault_sample_fetch,
	.channel_get = max31790_fan_fault_channel_get,
};

static int max31790_fan_fault_init(const struct device *dev)
{
	const struct max31790_fan_fault_config *config = dev->config;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}

	return 0;
}

#define MAX31790_FAN_FAULT_INIT(inst)                                                              \
	static const struct max31790_fan_fault_config max31790_fan_fault_##inst##_config = {       \
		.i2c = I2C_DT_SPEC_GET(DT_INST_PARENT(inst)),                                      \
	};                                                                                         \
                                                                                                   \
	static struct max31790_fan_fault_data max31790_fan_fault_##inst##_data;                    \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, max31790_fan_fault_init, NULL,                          \
			      &max31790_fan_fault_##inst##_data,                                   \
			      &max31790_fan_fault_##inst##_config, POST_KERNEL,                    \
			      CONFIG_SENSOR_INIT_PRIORITY, &max31790_fan_fault_api);

DT_INST_FOREACH_STATUS_OKAY(MAX31790_FAN_FAULT_INIT);
