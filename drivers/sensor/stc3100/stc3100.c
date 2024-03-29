/*
 * Copyright (c) 2024 Benedikt Schmidt <benediktibk@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stc3100

#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "stc3100.h"

LOG_MODULE_REGISTER(STC3100, CONFIG_SENSOR_LOG_LEVEL);

#define STC3100REGISTER_MODE           0x00
#define STC3100REGISTER_CTRL           0x01
#define STC3100REGISTER_CHARGELOW      0x02
#define STC3100REGISTER_CURRENTLOW     0x06
#define STC3100REGISTER_TEMPERATURELOW 0x0A

#define STC3100_MODE_GG_RUN_POS 4
#define STC3100_CTRL_GG_RST_POS 1

static int stc3100_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct stc3100_config *config = dev->config;
	struct stc3100_data *data = dev->data;
	int result;
	uint8_t write_buffer;
	uint8_t read_buffer[2];
	int16_t raw_value;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	write_buffer = STC3100REGISTER_CHARGELOW;
	result = i2c_write_read_dt(&config->i2c, &write_buffer, sizeof(write_buffer), read_buffer,
				   sizeof(read_buffer));
	if (result != 0) {
		LOG_ERR("%s: unable to read charge register", dev->name);
		return result;
	}

	raw_value = sys_get_le16(read_buffer);
	data->cumulative_charge = raw_value * 6700 / config->sense_resistor / (1000 * 1000);

	write_buffer = STC3100REGISTER_CURRENTLOW;
	result = i2c_write_read_dt(&config->i2c, &write_buffer, sizeof(write_buffer), read_buffer,
				   sizeof(read_buffer));
	if (result != 0) {
		LOG_ERR("%s: unable to read current register", dev->name);
		return result;
	}

	raw_value = sys_get_le16(read_buffer);
	data->current = raw_value * 11770 / config->sense_resistor / (1000 * 1000);

	write_buffer = STC3100REGISTER_TEMPERATURELOW;
	result = i2c_write_read_dt(&config->i2c, &write_buffer, sizeof(write_buffer), read_buffer,
				   sizeof(read_buffer));
	if (result != 0) {
		LOG_ERR("%s: unable to read temperature register", dev->name);
		return result;
	}

	raw_value = sys_get_le16(read_buffer);
	data->temperature = raw_value * 125 * 1000;

	return 0;
}

static int stc3100_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	const struct stc3100_config *config = dev->config;
	struct stc3100_data *data = dev->data;
	int32_t state_of_charge;

	switch (chan) {
	case SENSOR_CHAN_GAUGE_AVG_CURRENT:
		val->val1 = data->current / (1000 * 1000);
		val->val2 = data->current - val->val1 * 1000 * 1000;
		break;
	case SENSOR_CHAN_GAUGE_TEMP:
		val->val1 = data->temperature / (1000 * 1000);
		val->val2 = data->temperature - val->val1 * 1000 * 1000;
		break;
	case SENSOR_CHAN_GAUGE_STATE_OF_CHARGE:
		state_of_charge = data->cumulative_charge * 100 * 1000 / config->nominal_capacity;
		val->val1 = state_of_charge / (1000 * 1000);
		val->val2 = state_of_charge - val->val1 * 1000 * 1000;
		break;
	default:
		LOG_ERR("%s: requesting unsupported channel %i", dev->name, chan);
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api stc3100_api = {
	.sample_fetch = stc3100_sample_fetch,
	.channel_get = stc3100_channel_get,
};

static int stc3100_init(const struct device *dev)
{
	const struct stc3100_config *config = dev->config;
	uint8_t mode_register_value = BIT(STC3100_MODE_GG_RUN_POS);
	uint8_t ctrl_register_value = BIT(STC3100_CTRL_GG_RST_POS);
	int result;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}

	LOG_DBG("reset battery charge accumulator");
	result = i2c_reg_write_byte_dt(&config->i2c, STC3100REGISTER_CTRL, ctrl_register_value);
	if (result != 0) {
		LOG_ERR("%s: unable to reset battery charge accumulator", dev->name);
		return result;
	}

	LOG_DBG("start battery charge accumulator");
	result = i2c_reg_write_byte_dt(&config->i2c, STC3100REGISTER_MODE, mode_register_value);
	if (result != 0) {
		LOG_ERR("%s: unable to start battery charge accumulator", dev->name);
		return result;
	}

	return 0;
}

#define STC3100_INIT(inst)                                                                         \
	static const struct stc3100_config stc3100_##inst##_config = {                             \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.sense_resistor = DT_INST_PROP(inst, sense_resistor),                              \
		.nominal_capacity = DT_INST_PROP(inst, nominal_capacity),                          \
	};                                                                                         \
                                                                                                   \
	static struct stc3100_data stc3100_##inst##_data;                                          \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, stc3100_init, NULL, &stc3100_##inst##_data,             \
				     &stc3100_##inst##_config, POST_KERNEL,                        \
				     CONFIG_SENSOR_INIT_PRIORITY, &stc3100_api);

DT_INST_FOREACH_STATUS_OKAY(STC3100_INIT);
