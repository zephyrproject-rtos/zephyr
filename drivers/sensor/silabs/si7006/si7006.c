/*
 * Copyright (c) 2019 Electronut Labs
 * Copyright (c) 2023 Trent Piepho <tpiepho@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/sys/util.h>
#include "si7006.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(si7006, CONFIG_SENSOR_LOG_LEVEL);

struct si7006_data {
	uint16_t temperature;
	uint16_t humidity;
};

struct si7006_config {
	struct i2c_dt_spec i2c;
	const struct device *vin_supply;
	/** Use "read temp" vs "read old temp" command, the latter only with SiLabs sensors. */
	uint8_t read_temp_cmd;
};

/**
 * @brief function to get relative humidity
 *
 * @return int 0 on success
 */
static int si7006_get_humidity(const struct device *dev)
{
	struct si7006_data *si_data = dev->data;
	const struct si7006_config *config = dev->config;
	int retval;
	uint16_t hum;

	retval = i2c_burst_read_dt(&config->i2c, SI7006_MEAS_REL_HUMIDITY_MASTER_MODE,
				   (uint8_t *)&hum, sizeof(hum));

	if (retval == 0) {
		si_data->humidity = sys_be16_to_cpu(hum) & ~3;
	} else {
		LOG_ERR("read register err: %d", retval);
	}

	return retval;
}

/**
 * @brief function to get temperature
 *
 * Note that for Si7006 type sensors, si7006_get_humidity must be called before
 * calling si7006_get_temperature, as the get old temperature command is used.
 *
 * @return int 0 on success
 */

static int si7006_get_temperature(const struct device *dev)
{
	struct si7006_data *si_data = dev->data;
	const struct si7006_config *config = dev->config;
	uint16_t temp;
	int retval;

	retval = i2c_burst_read_dt(&config->i2c, config->read_temp_cmd,
				   (uint8_t *)&temp, sizeof(temp));

	if (retval == 0) {
		si_data->temperature = sys_be16_to_cpu(temp) & ~3;
	} else {
		LOG_ERR("read register err: %d", retval);
	}

	return retval;
}

/**
 * @brief fetch a sample from the sensor
 *
 * @return 0
 */
static int si7006_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	int retval;

	retval = si7006_get_humidity(dev);
	if (retval == 0) {
		retval = si7006_get_temperature(dev);
	}

	return retval;
}

/**
 * @brief sensor value get
 *
 * @return -ENOTSUP for unsupported channels
 */
static int si7006_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct si7006_data *si_data = dev->data;

	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		/* Raw formula: (temp * 175.72) / 65536 - 46.85
		 * To use integer math, scale the 175.72 factor by 128 and move the offset to
		 * inside the division.  This gives us:
		 *
		 * (temp * 175.72 * 128 - 46.86 * 128 * 65536) / (65536 * 128)
		 * The constants can be calculated now:
		 * (temp * 22492 - 393006285) / 2^23
		 *
		 * There is a very small amount of round-off error in the factor of 22492.  To
		 * compenstate, a constant of 5246 is used to center the error about 0, thus
		 * reducing the overall MSE.
		 */

		/* Temperature value times two to the 23rd power, i.e. temp_23 = temp << 23 */
		const int32_t temp_23 = si_data->temperature * 22492 - (393006285 - 5246);
		/* Integer component of temperature */
		int32_t temp_int = temp_23 >> 23;
		/* Fractional component of temperature */
		int32_t temp_frac = temp_23 & BIT_MASK(23);

		/* Deal with the split twos-complement / BCD format oddness with negatives */
		if (temp_23 < 0) {
			temp_int += 1;
			temp_frac -= BIT(23);
		}
		val->val1 = temp_int;
		/* Remove a constant factor of 64 from (temp_frac * 1000000) >> 23 */
		val->val2 = (temp_frac * 15625ULL) >> 17;

		LOG_DBG("temperature %u = val1:%d, val2:%d", si_data->temperature,
			val->val1, val->val2);

		return 0;
	} else if (chan == SENSOR_CHAN_HUMIDITY) {
		/* Humidity times two to the 16th power.  Offset of -6 not applied yet. */
		const uint32_t rh_16 = si_data->humidity * 125U;
		/* Integer component of humidity */
		const int16_t rh_int = rh_16 >> 16;
		/* Fraction component of humidity */
		const uint16_t rh_frac = rh_16 & BIT_MASK(16);

		val->val1 = rh_int - 6; /* Apply offset now */
		/* Remove a constant factor of 64 from (rh_frac * 1000000) >> 16 */
		val->val2 = (rh_frac * 15625) >> 10;

		/* Deal with the split twos-complement / BCD format oddness with negatives */
		if (val->val1 < 0) {
			val->val1 += 1;
			val->val2 -= 1000000;
		}

		LOG_DBG("humidity %u = val1:%d, val2:%d", si_data->humidity, val->val1, val->val2);

		return 0;
	} else {
		return -ENOTSUP;
	}
}

static DEVICE_API(sensor, si7006_api) = {
	.sample_fetch = &si7006_sample_fetch,
	.channel_get = &si7006_channel_get,
};

/**
 * @brief initialize the sensor
 *
 * @return 0 for success
 */

static int si7006_init(const struct device *dev)
{
	const struct si7006_config *config = dev->config;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	if (IS_ENABLED(CONFIG_REGULATOR) && config->vin_supply) {
		regulator_enable(config->vin_supply);

		/* As stated by the Si7006 spec - Maximum powerup time is 80ms */
		k_msleep(80);
	}

	LOG_DBG("si7006 init ok");

	return 0;
}

#define SI7006_DEFINE(inst, name, temp_cmd)						\
	static struct si7006_data si7006_data_##name##_##inst;				\
											\
	static const struct si7006_config si7006_config_##name##_##inst = {		\
		.i2c = I2C_DT_SPEC_INST_GET(inst),					\
		.vin_supply = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(inst, vin_supply)),	\
		.read_temp_cmd = temp_cmd,						\
	};										\
											\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, si7006_init, NULL,				\
				     &si7006_data_##name##_##inst,			\
				     &si7006_config_##name##_##inst,			\
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,		\
				     &si7006_api);

#define DT_DRV_COMPAT silabs_si7006
DT_INST_FOREACH_STATUS_OKAY_VARGS(SI7006_DEFINE, DT_DRV_COMPAT, SI7006_READ_OLD_TEMP);

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT sensirion_sht21
DT_INST_FOREACH_STATUS_OKAY_VARGS(SI7006_DEFINE, DT_DRV_COMPAT, SI7006_MEAS_TEMP_MASTER_MODE);
