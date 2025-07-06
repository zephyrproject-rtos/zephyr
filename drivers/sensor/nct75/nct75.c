/*
 * Copyright (c) 2024 Benedikt Schmidt <benediktibk@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT onnn_nct75

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(NCT75, CONFIG_SENSOR_LOG_LEVEL);

#define NCT75REGISTER_STOREDTEMPERATUREVALUE 0x00
#define NCT75REGISTER_CONFIGURATION          0x01
#define NCT75REGISTER_ONESHOT                0x04

#define NCT75_CONFIGURATION_ONESHOTMODE_POS 5

#define NCT75_TEMPERATURE_CONVERSION_TIME_US      48500
#define NCT75_TEMPERATURE_CONVERSION_WAIT_TIME_US (NCT75_TEMPERATURE_CONVERSION_TIME_US + 1000)

struct nct75_config {
	struct i2c_dt_spec i2c;
};

struct nct75_data {
	/* temperature in 1e-6 °C */
	int64_t value;
};

static int nct75_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct nct75_config *config = dev->config;
	struct nct75_data *data = dev->data;
	int result;
	uint8_t write_buffer = 0;
	uint16_t read_buffer;
	int16_t raw_value;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	result = i2c_reg_write_byte_dt(&config->i2c, NCT75REGISTER_ONESHOT, 0);
	if (result != 0) {
		LOG_ERR("%s: unable to trigger temperature one shot measurement", dev->name);
		return result;
	}

	k_sleep(K_USEC(NCT75_TEMPERATURE_CONVERSION_WAIT_TIME_US));

	write_buffer = NCT75REGISTER_STOREDTEMPERATUREVALUE;
	result = i2c_write_read_dt(&config->i2c, &write_buffer, sizeof(write_buffer), &read_buffer,
				   sizeof(read_buffer));
	if (result != 0) {
		LOG_ERR("%s: unable to read temperature", dev->name);
		return result;
	}

	raw_value = arithmetic_shift_right(sys_be16_to_cpu(read_buffer), 4);
	data->value = (raw_value * 1000 * 1000) * 100 / 0x640;

	return 0;
}

static int nct75_channel_get(const struct device *dev, enum sensor_channel chan,
			     struct sensor_value *val)
{
	struct nct75_data *data = dev->data;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		LOG_ERR("%s: requesting unsupported channel %i", dev->name, chan);
		return -ENOTSUP;
	}

	val->val1 = data->value / (1000 * 1000);
	val->val2 = data->value - val->val1 * 1000 * 1000;
	return 0;
}

static DEVICE_API(sensor, nct75_api) = {
	.sample_fetch = nct75_sample_fetch,
	.channel_get = nct75_channel_get,
};

static int nct75_init(const struct device *dev)
{
	const struct nct75_config *config = dev->config;
	uint8_t configuration_register_value = BIT(NCT75_CONFIGURATION_ONESHOTMODE_POS);
	int result;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}

	result = i2c_reg_write_byte_dt(&config->i2c, NCT75REGISTER_CONFIGURATION,
				       configuration_register_value);
	if (result != 0) {
		LOG_ERR("%s: unable to configure temperature sensor", dev->name);
		return result;
	}

	return 0;
}

#define NCT75_INIT(inst)                                                                           \
	static const struct nct75_config nct75_##inst##_config = {                                 \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
                                                                                                   \
	static struct nct75_data nct75_##inst##_data;                                              \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, nct75_init, NULL, &nct75_##inst##_data,                 \
				     &nct75_##inst##_config, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &nct75_api);

DT_INST_FOREACH_STATUS_OKAY(NCT75_INIT);
