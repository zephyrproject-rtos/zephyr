/*
 * Copyright 2026 Ahmed Ashraf NourEldeen <a.programmer55559@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_tc74

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tc74, CONFIG_SENSOR_LOG_LEVEL);

/* register definitions per Microchip TC74 Datasheet Section 4.0 */
#define TC74_TEMP_REG   0x00
#define TC74_CONFIG_REG 0x01

#define TC74_CONFIG_DATA_RDY BIT(6)

struct tc74_config {
	struct i2c_dt_spec i2c_spec;
};

struct tc74_data {
	int8_t temp_sample;
};

static int tc74_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct tc74_config *config = dev->config;
	struct tc74_data *data = dev->data;
	int ret;
	uint8_t cfg_reg = TC74_CONFIG_REG;
	uint8_t temp_reg = TC74_TEMP_REG;
	uint8_t cfg_val;
	uint8_t raw_temp;

	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_AMBIENT_TEMP)) {
		return -ENOTSUP;
	}

	/* Read config register to verify DATA_RDY bit */
	ret = i2c_write_read_dt(&config->i2c_spec, &cfg_reg, sizeof(cfg_reg), &cfg_val,
				sizeof(cfg_val));
	if (ret < 0) {
		return ret;
	}

	if (!(cfg_val & TC74_CONFIG_DATA_RDY)) {
		LOG_WRN("Temperature data not ready yet");
		return -EBUSY;
	}

	/* Fetch ambient temperature sample */
	ret = i2c_write_read_dt(&config->i2c_spec, &temp_reg, sizeof(temp_reg), &raw_temp,
				sizeof(raw_temp));
	if (ret < 0) {
		return ret;
	}

	data->temp_sample = (int8_t)raw_temp;
	LOG_DBG("Raw sample: %d", data->temp_sample);

	return 0;
}

static int tc74_channel_get(const struct device *dev, enum sensor_channel chan,
			    struct sensor_value *val)
{
	struct tc74_data *data = dev->data;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	val->val1 = data->temp_sample;
	val->val2 = 0;

	return 0;
}

static DEVICE_API(sensor, tc74_api) = {
	.sample_fetch = &tc74_sample_fetch,
	.channel_get = &tc74_channel_get,
};

static int tc74_init(const struct device *dev)
{
	const struct tc74_config *config = dev->config;

	if (!i2c_is_ready_dt(&config->i2c_spec)) {
		LOG_ERR_DEVICE_NOT_READY(config->i2c_spec.bus);
		return -ENODEV;
	}

	return 0;
}

#define TC74_INIT(n)                                                                               \
	static struct tc74_data tc74_data_##n;                                                     \
	static const struct tc74_config tc74_config_##n = {                                        \
		.i2c_spec = I2C_DT_SPEC_INST_GET(n),                                               \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(n, &tc74_init, NULL, &tc74_data_##n, &tc74_config_##n,        \
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &tc74_api);

DT_INST_FOREACH_STATUS_OKAY(TC74_INIT)
