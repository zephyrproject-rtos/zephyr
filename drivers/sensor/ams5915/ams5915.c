/*
 * Copyright (c) 2024 BoRob Engineering
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT analogmicro_ams5915

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>

#include <zephyr/init.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/logging/log.h>

#include <math.h>

LOG_MODULE_REGISTER(AMS5915, CONFIG_SENSOR_LOG_LEVEL);

/* used constants according datasheet */
#define AMS5915_DIGPOUT_P_MAX    14745
#define AMS5915_DIGPOUT_P_MIN    1638
#define AMS5915_AMBIENT_TEMP_MAX 85.0f

struct ams5915_limits {
	float press_min; /* lower pressure limit in kilopascal */
	float press_max; /* upper pressure limit in kilopascal */
};

struct ams5915_config {
	const struct i2c_dt_spec bus;
	struct ams5915_limits limits;
};

struct ams5915_data {
	float temp_c;	/* temperature in degree Celsius */
	float press_kilopascal; /* pressure in kilopascal */
};

static void ams5915_adc2temp(const struct device *dev, uint16_t adc_temp)
{
	struct ams5915_data *data = dev->data;

	// get degree Celsius
	float temp = (float)(adc_temp * 200) / 2048.0f - 50.0f;

	if (temp > AMS5915_AMBIENT_TEMP_MAX) {
		LOG_ERR("Temperature out of range!");

		/* no valid temperature */
		temp = NAN;
	}

	data->temp_c = temp;
}

static void ams5915_adc2press(const struct device *dev, uint16_t adc_press)
{
	struct ams5915_data *data = dev->data;
	const struct ams5915_config *config = dev->config;

	/* get range counts per milli Bar */
	float Sensp = (AMS5915_DIGPOUT_P_MAX - AMS5915_DIGPOUT_P_MIN) /
		      (config->limits.press_max * 0.1f - config->limits.press_min * 0.1f);

	/* get milli bar */
	Sensp = (float)(adc_press - AMS5915_DIGPOUT_P_MIN) / Sensp + config->limits.press_min * 0.1f;

	/* get kilopascal */
	Sensp = 10.0f * Sensp;

	if ((Sensp >= config->limits.press_min) && (Sensp <= config->limits.press_max)) {
		data->press_kilopascal = Sensp;
	} else {
		LOG_ERR("Pressure out of range!");

		// pressure outside sepcified limits
		data->press_kilopascal = NAN;
	}
}

static int ams5915_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct ams5915_data *data = dev->data;
	const struct ams5915_config *config = dev->config;

	uint16_t adc_press = 0;
	uint16_t adc_temp = 0;
	uint8_t buf[sizeof(adc_press) + sizeof(adc_temp)] = {0};
	int ret = 0;

	__ASSERT_NO_MSG((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_DIE_TEMP) ||
			chan == SENSOR_CHAN_PRESS);

	if ((config->limits.press_max == NAN || config->limits.press_min == NAN) &&
	    ((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_PRESS))) {
		LOG_ERR("Invalid Sensor Attribute!");
		return -EINVAL;
	}

	/* read data from sensor */
	switch (chan) {
	case SENSOR_CHAN_ALL:
	case SENSOR_CHAN_DIE_TEMP:
		/* need to read four bytes */
		ret = i2c_read_dt(&config->bus, buf, sizeof(adc_press) + sizeof(adc_temp));
		break;

	case SENSOR_CHAN_PRESS:
		/* only two bytes need to be read */
		ret = i2c_read_dt(&config->bus, buf, sizeof(adc_press));
		break;

	default:
		LOG_ERR("Unsupported Sensor Channel!");
		return -ENOTSUP;
		break;
	}

	/* check result */
	if (ret < 0) {
		LOG_ERR("Read failed!");
		return -EIO;
	}

	/* extract raw adc values from receive buffer */
	adc_press = (uint16_t)(buf[0] & 0x3F) << 8 | buf[1];
	adc_temp = (uint16_t)(buf[2]) << 3 | (buf[3] & 0xE0) >> 5;

	/* scale adc values to pressure/temperature */
	switch (chan) {
	case SENSOR_CHAN_ALL:
		ams5915_adc2temp(dev, adc_temp);
		ams5915_adc2press(dev, adc_press);
		break;

	case SENSOR_CHAN_DIE_TEMP:
		ams5915_adc2temp(dev, adc_temp);
		break;

	case SENSOR_CHAN_PRESS:
		ams5915_adc2press(dev, adc_press);
		break;

	default:
		LOG_ERR("Unsupported Sensor Channel!");
		return -ENOTSUP;
		break;
	}

	/* check results */
	if ((data->press_kilopascal == NAN) || (data->temp_c == NAN)) {
		return -ERANGE;
	}

	return 0;
}

static int ams5915_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct ams5915_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		if (data->temp_c == NAN) {
			LOG_ERR("Temperature invalid!");
			return -ERANGE;
		}
		sensor_value_from_float(val, data->temp_c);
		break;

	case SENSOR_CHAN_PRESS:
		if (data->press_kilopascal == NAN) {
			LOG_ERR("Pressure invalid!");
			return -ERANGE;
		}
		sensor_value_from_float(val, data->press_kilopascal);
		break;

	default:
		LOG_ERR("Unsupported Sensor Channel!");
		return -ENOTSUP;
		break;
	}

	return 0;
}

static const struct sensor_driver_api ams5915_driver_api = {.sample_fetch = ams5915_sample_fetch,
							    .channel_get = ams5915_channel_get};

int ams5915_init(const struct device *dev)
{
	const struct ams5915_config *config = dev->config;
	struct ams5915_data *data = dev->data;

	if (!i2c_is_ready_dt(&config->bus)) {
		LOG_ERR("I2C bus %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	// invalidate data
	data->press_kilopascal = NAN;
	data->temp_c = NAN;

	return 0;
}

#define AMS5915_INST(inst)                                                                         \
	static struct ams5915_data ams5915_data_##inst;                                            \
	static const struct ams5915_config ams5915_config_##inst = {                               \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.limits.press_min = DT_INST_PROP(inst, lower_limit_press),                              \
		.limits.press_max = DT_INST_PROP(inst, upper_limit_press)};                             \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, ams5915_init, NULL, &ams5915_data_##inst,               \
				     &ams5915_config_##inst, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &ams5915_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AMS5915_INST)
