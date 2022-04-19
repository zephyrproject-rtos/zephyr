/* veml7700.c - Driver for Vishay VEML7700 High-Accuracy Ambient Light Sensor
 *
 * https://www.vishay.com/docs/84286/veml7700.pdf
 */

/*
 * Copyright (c) 2022 Nikolaus Huber
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT vishay_veml7700

#include "veml7700.h"
#include <zephyr/drivers/i2c.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/sensor.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(veml7700, CONFIG_SENSOR_LOG_LEVEL);

static int veml7700_write(const struct veml7700_config *cfg, uint8_t command, uint16_t data)
{
	uint8_t cmd[3];

	cmd[0] = command;
	sys_put_le16(data, &cmd[1]);

	return i2c_write_dt(&cfg->bus, cmd, sizeof(cmd));
}

static int veml7700_read(const struct veml7700_config *cfg, uint8_t command, uint16_t *data)
{
	int ret = i2c_burst_read_dt(&cfg->bus, command, (uint8_t *)data, sizeof(*data));

	if (ret < 0) {
		return -ret;
	}

	*data = sys_le16_to_cpu(*data);

	return 0;
}

static int veml7700_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct veml7700_data *data = dev->data;
	const struct veml7700_config *cfg = dev->config;
	uint16_t val;

	if (!(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_LIGHT)) {
		return -ENOTSUP;
	}

	int ret = veml7700_read(cfg, VEML7700_ALS, &val);

	if (ret < 0) {
		return ret;
	}

	data->als_reading = val;

	return 0;
}

static int veml7700_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	struct veml7700_data *data = dev->data;

	if (chan != SENSOR_CHAN_LIGHT) {
		return -ENOTSUP;
	}

	double lux = data->als_reading * data->factor1 * data->factor2;

	/* See application note
	 * https://www.vishay.com/docs/84323/designingveml7700.pdf (page 10)
	 */
	lux = lux * (1.0023f + lux * (8.1488e-5f + lux *
				      (-9.3924e-9f + lux * 6.0135e-13f)));

	return sensor_value_from_double(val, lux);
}

static const struct sensor_driver_api veml7700_driver_api = {
	.sample_fetch = veml7700_sample_fetch,
	.channel_get = veml7700_channel_get,
};


static int veml7700_init(const struct device *dev)
{
	const struct veml7700_config *config = dev->config;
	struct veml7700_data *data = dev->data;

	if (!device_is_ready(config->bus.bus)) {
		LOG_ERR("I2C master %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	/* Configure sensor */
	int16_t cmd = 0x00;

	/* Assign conversion factors here, since settings cannot change during
	 * runtime
	 */
	switch (config->gain) {
	case GAIN_X1:
		data->factor1 = 1.f;
		cmd |= 0b00 << VEML7700_GAIN_SHIFT;
		break;
	case GAIN_X2:
		data->factor1 = 0.5f;
		cmd |= 0b01 << VEML7700_GAIN_SHIFT;
		break;
	case GAIN_X1_8:
		data->factor1 = 8.f;
		cmd |= 0b10 << VEML7700_GAIN_SHIFT;
		break;
	case GAIN_X1_4:
		data->factor1 = 4.f;
		cmd |= 0b11 << VEML7700_GAIN_SHIFT;
		break;
	}

	switch (config->it) {
	case IT_100MS:
		data->factor2 = 0.0576f;
		cmd |= 0b0000 << VEML7700_IT_SHIFT;
		break;
	case IT_200MS:
		data->factor2 = 0.0288f;
		cmd |= 0b0001 << VEML7700_IT_SHIFT;
		break;
	case IT_400MS:
		data->factor2 = 0.0144f;
		cmd |= 0b0010 << VEML7700_IT_SHIFT;
		break;
	case IT_800MS:
		data->factor2 = 0.0072f;
		cmd |= 0b0011 << VEML7700_IT_SHIFT;
		break;
	case IT_25MS:
		data->factor2 = 0.2304f;
		cmd |= 0b1100 << VEML7700_IT_SHIFT;
		break;
	case IT_50MS:
		data->factor2 = 0.1152f;
		cmd |= 0b1000 << VEML7700_IT_SHIFT;
		break;
	}

	switch (config->persistence) {
	case PERSISTENCE_1:
		cmd |= 0b00 << VEML7700_PERS_SHIFT;
		break;
	case PERSISTENCE_2:
		cmd |= 0b01 << VEML7700_PERS_SHIFT;
		break;
	case PERSISTENCE_4:
		cmd |= 0b10 << VEML7700_PERS_SHIFT;
		break;
	case PERSISTENCE_8:
		cmd |= 0b11 << VEML7700_PERS_SHIFT;
		break;
	}

	int ret = veml7700_write(config, VEML7700_ALS_CONF_0, cmd);

	if (ret < 0) {
		return ret;
	}

	if (config->psm_enabled) {
		cmd = 0x01 | (config->psm_mode << 1);
		ret = veml7700_write(config, VEML7700_PSM, cmd);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

#define DEFINE_VEML7700(_num) \
	static struct veml7700_data veml7700_driver_data_##_num; \
	static const struct veml7700_config veml7700_driver_config_##_num = { \
		.bus = I2C_DT_SPEC_INST_GET(_num), \
		.gain = DT_INST_ENUM_IDX(_num, gain), \
		.it = DT_INST_ENUM_IDX(_num, als_it), \
		.persistence = DT_INST_ENUM_IDX(_num, persistence), \
		.psm_enabled = DT_INST_PROP(_num, enable_psm), \
		.psm_mode = DT_INST_ENUM_IDX(_num, psm_mode), \
	}; \
	DEVICE_DT_INST_DEFINE(_num, veml7700_init, NULL, \
				&veml7700_driver_data_##_num, &veml7700_driver_config_##_num, \
				POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, \
				&veml7700_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_VEML7700)
