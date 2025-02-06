/*
 * Copyright (c) 2024 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT invensense_icp101xx

#include <math.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/logging/log.h>

#include "icp101xx_drv.h"

LOG_MODULE_REGISTER(ICP101XX, CONFIG_SENSOR_LOG_LEVEL);

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#error "ICP101XX driver enabled without any compatible device in devicetree"
#endif

inline void inv_icp101xx_sleep_us(int us)
{
	k_sleep(K_USEC(us));
}

static int inv_io_hal_read_reg(void *ctx, uint8_t reg, uint8_t *rbuffer, uint32_t rlen)
{
	struct device *dev = (struct device *)ctx;
	const struct icp101xx_config *cfg = (const struct icp101xx_config *)dev->config;

	return i2c_read_dt(&cfg->i2c, (uint8_t *)rbuffer, rlen);
}

static int inv_io_hal_write_reg(void *ctx, uint8_t reg, const uint8_t *wbuffer, uint32_t wlen)
{
	struct device *dev = (struct device *)ctx;
	const struct icp101xx_config *cfg = (const struct icp101xx_config *)dev->config;

	return i2c_write_dt(&cfg->i2c, (uint8_t *)wbuffer, wlen);
}

static uint8_t get_timeout_ms(enum icp101xx_meas mode)
{
	switch (mode) {
	case ICP101XX_MEAS_LOW_POWER_T_FIRST:
	case ICP101XX_MEAS_LOW_POWER_P_FIRST:
		return 2;
	case ICP101XX_MEAS_NORMAL_T_FIRST:
	case ICP101XX_MEAS_NORMAL_P_FIRST:
		return 7;
	case ICP101XX_MEAS_LOW_NOISE_T_FIRST:
	case ICP101XX_MEAS_LOW_NOISE_P_FIRST:
		return 24;
	default:
	case ICP101XX_MEAS_ULTRA_LOW_NOISE_T_FIRST:
	case ICP101XX_MEAS_ULTRA_LOW_NOISE_P_FIRST:
		return 95;
	}
}

static uint8_t get_conversion_ms(enum icp101xx_meas mode)
{
	switch (mode) {
	case ICP101XX_MEAS_LOW_POWER_T_FIRST:
	case ICP101XX_MEAS_LOW_POWER_P_FIRST:
		return 1;
	case ICP101XX_MEAS_NORMAL_T_FIRST:
	case ICP101XX_MEAS_NORMAL_P_FIRST:
		return 5;
	case ICP101XX_MEAS_LOW_NOISE_T_FIRST:
	case ICP101XX_MEAS_LOW_NOISE_P_FIRST:
		return 20;
	default:
	case ICP101XX_MEAS_ULTRA_LOW_NOISE_T_FIRST:
	case ICP101XX_MEAS_ULTRA_LOW_NOISE_P_FIRST:
		return 80;
	}
}

#ifdef ICP101XX_DRV_USE_FLOATS
#define ATMOSPHERICAL_PRESSURE_KPA ((float)101.325)
#define TO_KELVIN(temp_C)          ((float)273.15 + temp_C)
/*
 * Constant in altitude formula:
 * M*g/R = (0,0289644 * 9,80665 / 8,31432)
 * with M the molar mass of air.
 * with g the gravitational force acceleration.
 * with R the universal gaz constant.
 */
#define HEIGHT_TO_PRESSURE_COEFF   ((float)0.03424)

/*
 * Constant in altitude formula:
 * R / (M*g) = 8,31432 / (0,0289644 * 9,80665)
 * with M the molar mass of air.
 * with g the gravitational force acceleration.
 * with R the universal gaz constant.
 */
#define PRESSURE_TO_HEIGHT_COEFF   ((float)29.27127)
/*
 * Constant for altitude formula:
 * logarithm of pressure at 0m
 * ln(101.325)
 */
#define LOG_ATMOSPHERICAL_PRESSURE ((float)4.61833)

float convertToHeight(float pressure_kp, float temperature_C)
{
	return PRESSURE_TO_HEIGHT_COEFF * TO_KELVIN(temperature_C) *
	       (LOG_ATMOSPHERICAL_PRESSURE - logf(pressure_kp));
}
#endif

static int icp101xx_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	int err = 0;
	struct icp101xx_data *data = (struct icp101xx_data *)dev->data;

	__ASSERT_NO_MSG(val != NULL);
	if (chan == SENSOR_CHAN_PRESS) {
		if (attr == SENSOR_ATTR_CONFIGURATION) {
			if ((val->val1 >= ICP101XX_MEAS_LOW_POWER_T_FIRST) &&
			    (val->val1 <= ICP101XX_MEAS_ULTRA_LOW_NOISE_P_FIRST)) {
				data->icp_device.measurement_mode = val->val1;
			} else {
				LOG_ERR("Not supported ATTR value");
				return -EINVAL;
			}

		} else {
			LOG_ERR("Not supported ATTR");
			return -EINVAL;
		}
	};
	return err;
}

static int icp101xx_sample_fetch(const struct device *dev, const enum sensor_channel chan)
{
	struct icp101xx_data *data = (struct icp101xx_data *)dev->data;
	int rc = 0;
	uint64_t timeout;

	if (!((chan == SENSOR_CHAN_AMBIENT_TEMP) || (chan == SENSOR_CHAN_PRESS) ||
	      (chan == SENSOR_CHAN_ALTITUDE) || (chan == SENSOR_CHAN_ALL))) {
		return -ENOTSUP;
	}
	rc = inv_icp101xx_enable_sensor(&data->icp_device, 1);
	/* Compute timeout for the measure */
	timeout = k_uptime_get() + get_timeout_ms(data->icp_device.measurement_mode);
	/* Initial sleep waiting the sensor proceeds with the measure */
	k_sleep(K_MSEC(get_conversion_ms(data->icp_device.measurement_mode)));
	do {
		k_sleep(K_USEC(200));
		rc = inv_icp101xx_get_data(&data->icp_device, &(data->raw_pressure),
					   &(data->raw_temperature), &(data->pressure),
					   &(data->temperature));
	} while ((rc != 0) && (k_uptime_get() <= timeout));
	return rc;
}

static int icp101xx_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{

	struct icp101xx_data *data = (struct icp101xx_data *)dev->data;

	val->val1 = 0;
	val->val2 = 0;

	if (!((chan == SENSOR_CHAN_AMBIENT_TEMP) || (chan == SENSOR_CHAN_PRESS) ||
	      (chan == SENSOR_CHAN_ALTITUDE))) {
		return -ENOTSUP;
	}
	/* Zephyr expects kPa while ICP101xx returns Pa */
	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
#ifdef ICP101XX_DRV_USE_FLOATS
		sensor_value_from_float(val, data->temperature);
#else
		val->val1 = data->temperature >> 4;
		val->val2 = (data->temperature % 16) * 1000000 / 16;
#endif
	} else if (chan == SENSOR_CHAN_PRESS) {
#ifdef ICP101XX_DRV_USE_FLOATS
		sensor_value_from_float(val, data->pressure / 1000);
#else
		val->val1 = data->pressure / 1000;
		val->val2 = (data->pressure % 1000) * 1000;
#endif
#ifdef ICP101XX_DRV_USE_FLOATS
	} else if (chan == SENSOR_CHAN_ALTITUDE) {
		float altitude = convertToHeight(data->pressure / 1000, data->temperature);

		sensor_value_from_float(val, altitude);
#endif
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int icp101xx_init(const struct device *dev)
{
	int rc = 0;
	struct icp101xx_data *data = (struct icp101xx_data *)dev->data;
	const struct icp101xx_config *cfg = (const struct icp101xx_config *)dev->config;

	memset(&(data->icp_device), 0, sizeof(data->icp_device));

	data->icp_device.serif.context = (void *)dev;
	data->icp_device.serif.read_reg = inv_io_hal_read_reg;
	data->icp_device.serif.write_reg = inv_io_hal_write_reg;
	/* maximum number of bytes allowed per serial read */
	data->icp_device.serif.max_read = 2048;
	/* maximum number of bytes allowed per serial write */
	data->icp_device.serif.max_write = 2048;

	rc = inv_icp101xx_soft_reset(&data->icp_device);
	if (rc != 0) {
		LOG_ERR("Soft reset error %d", rc);
		return rc;
	}
	inv_icp101xx_init(&data->icp_device);
	if (rc != 0) {
		LOG_ERR("Init error %d", rc);
		return rc;
	}
	data->icp_device.measurement_mode = cfg->mode;

	/* successful init, return 0 */
	return 0;
}

static DEVICE_API(sensor, icp101xx_api_funcs) = {
	.sample_fetch = icp101xx_sample_fetch,
	.channel_get = icp101xx_channel_get,
	.attr_set = icp101xx_attr_set,
};

#define ICP101XX_DEFINE(inst)                                                                      \
	static struct icp101xx_data icp101xx_drv_##inst;                                           \
	static const struct icp101xx_config icp101xx_config_##inst = {                             \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.mode = DT_INST_ENUM_IDX(inst, mode),                                              \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, icp101xx_init, NULL, &icp101xx_drv_##inst,              \
			      &icp101xx_config_##inst, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,   \
			      &icp101xx_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(ICP101XX_DEFINE)
