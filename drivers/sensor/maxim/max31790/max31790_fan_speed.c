/*
 * Copyright (c) 2024 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT maxim_max31790_fan_speed

#include <zephyr/drivers/mfd/max31790.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "max31790_fan_speed.h"

#define FACTOR_RPM_TO_HZ           60
#define TACH_COUNT_FREQUENCY       (MAX31790_OSCILLATOR_FREQUENCY_IN_HZ / 4)
#define TACH_COUNTS_PER_REVOLUTION 2

LOG_MODULE_REGISTER(MAX31790_FAN_SPEED, CONFIG_SENSOR_LOG_LEVEL);

static int max31790_fan_speed_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct max31790_fan_speed_config *config = dev->config;
	struct max31790_fan_speed_data *data = dev->data;
	uint16_t tach_count;
	uint8_t fan_dynamics;
	uint8_t number_tach_periods_counted;
	uint8_t speed_range;
	uint8_t register_address;
	int result;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	register_address = MAX37190_REGISTER_TACHCOUNTMSB(config->channel_id);
	result = i2c_write_read_dt(&config->i2c, &register_address, sizeof(register_address),
				   &tach_count, sizeof(tach_count));
	tach_count = sys_be16_to_cpu(tach_count);
	if (result != 0) {
		return result;
	}

	result = i2c_reg_read_byte_dt(
		&config->i2c, MAX31790_REGISTER_FANDYNAMICS(config->channel_id), &fan_dynamics);
	if (result != 0) {
		return result;
	}

	tach_count = tach_count >> 5;
	speed_range = MAX31790_FANXDYNAMCIS_SPEED_RANGE_GET(fan_dynamics);

	switch (speed_range) {
	case 0:
		number_tach_periods_counted = 1;
		break;
	case 1:
		number_tach_periods_counted = 2;
		break;
	case 2:
		number_tach_periods_counted = 4;
		break;
	case 3:
		number_tach_periods_counted = 8;
		break;
	case 4:
		number_tach_periods_counted = 16;
		break;
	case 5:
		__fallthrough;
	case 6:
		__fallthrough;
	case 7:
		number_tach_periods_counted = 32;
		break;
	default:
		LOG_ERR("%s: invalid speed range %i", dev->name, speed_range);
		return -EINVAL;
	}

	if (tach_count == 0) {
		LOG_WRN("%s: tach count is zero", dev->name);
		data->rpm = UINT16_MAX;
	} else {
		LOG_DBG("%s: %i tach periods counted, %i tach count", dev->name,
			number_tach_periods_counted, tach_count);
		data->rpm = FACTOR_RPM_TO_HZ * TACH_COUNT_FREQUENCY * number_tach_periods_counted /
			    (tach_count * TACH_COUNTS_PER_REVOLUTION);
	}

	return 0;
}

static int max31790_fan_speed_channel_get(const struct device *dev, enum sensor_channel chan,
					  struct sensor_value *val)
{
	struct max31790_fan_speed_data *data = dev->data;

	if (chan != SENSOR_CHAN_RPM) {
		LOG_ERR("%s: requesting unsupported channel %i", dev->name, chan);
		return -ENOTSUP;
	}

	val->val1 = data->rpm;
	val->val2 = 0;
	return 0;
}

static const struct sensor_driver_api max31790_fan_speed_api = {
	.sample_fetch = max31790_fan_speed_sample_fetch,
	.channel_get = max31790_fan_speed_channel_get,
};

static int max31790_fan_speed_init(const struct device *dev)
{
	const struct max31790_fan_speed_config *config = dev->config;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}

	return 0;
}

#define MAX31790_FAN_SPEED_INIT(inst)                                                              \
	static const struct max31790_fan_speed_config max31790_fan_speed_##inst##_config = {       \
		.i2c = I2C_DT_SPEC_GET(DT_INST_PARENT(inst)),                                      \
		.channel_id = DT_INST_PROP(inst, channel) - 1,                                     \
	};                                                                                         \
                                                                                                   \
	static struct max31790_fan_speed_data max31790_fan_speed_##inst##_data;                    \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, max31790_fan_speed_init, NULL,                          \
			      &max31790_fan_speed_##inst##_data,                                   \
			      &max31790_fan_speed_##inst##_config, POST_KERNEL,                    \
			      CONFIG_SENSOR_INIT_PRIORITY, &max31790_fan_speed_api);

DT_INST_FOREACH_STATUS_OKAY(MAX31790_FAN_SPEED_INIT);
