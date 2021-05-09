/*
 * Copyright (c) 2019 Thomas Schmid <tom@lfence.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT meas_ms5607

#include <init.h>
#include <kernel.h>
#include <sys/byteorder.h>
#include <drivers/sensor.h>
#include <sys/__assert.h>

#include "ms5607.h"

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(ms5607);

static void ms5607_compensate(struct ms5607_data *data,
			      const int32_t adc_temperature,
			      const int32_t adc_pressure)
{
	int64_t dT;
	int64_t OFF;
	int64_t SENS;
	int64_t temp_sq;
	int64_t Ti;
	int64_t OFFi;
	int64_t SENSi;

	/* first order compensation as per datasheet
	 * (https://www.te.com/usa-en/product-CAT-BLPS0035.html) section
	 * PRESSURE AND TEMPERATURE CALCULATION
	 */

	dT = adc_temperature - ((uint32_t)(data->t_ref) << 8);
	data->temperature = 2000 + (dT * data->tempsens) / (1ll << 23);
	OFF = ((int64_t)(data->off_t1) << 17) + (dT * data->tco) / (1ll << 6);
	SENS = ((int64_t)(data->sens_t1) << 16) + (dT * data->tcs) / (1ll << 7);

	/* Second order compensation as per datasheet
	 * (https://www.te.com/usa-en/product-CAT-BLPS0035.html) section
	 * SECOND ORDER TEMPERATURE COMPENSATION
	 */

	temp_sq = (int64_t)(data->temperature - 2000) *
		  (int64_t)(data->temperature - 2000);
	if (data->temperature < 2000) {
		Ti = (dT * dT) / (1ll << 31);
		OFFi = (61ll * temp_sq) / (1ll << 4);
		SENSi = 2ll * temp_sq;
		if (data->temperature < -1500) {
			temp_sq = (int64_t)(data->temperature + 1500) *
				  (int64_t)(data->temperature + 1500);
			OFFi += 15ll * temp_sq;
			SENSi += 8ll * temp_sq;
		}
	} else {
		SENSi = 0;
		OFFi = 0;
		Ti = 0;
	}

	OFF -= OFFi;
	SENS -= SENSi;

	data->temperature -= Ti;
	data->pressure = (SENS * (int64_t)adc_pressure / (1ll << 21) - OFF) /
			 (1ll << 15);
}

static int ms5607_read_prom(const struct ms5607_data *data, uint8_t cmd,
			    uint16_t *val)
{
	int err;

	err = data->tf->read_prom(data, cmd, val);
	if (err < 0) {
		LOG_ERR("Error reading prom");
		return err;
	}

	return 0;
}

static int ms5607_get_measurement(const struct ms5607_data *data,
				  uint32_t *val,
				  uint8_t cmd,
				  uint8_t delay)
{
	int err;

	*val = 0U;

	err = data->tf->start_conversion(data, cmd);
	if (err < 0) {
		return err;
	}

	k_msleep(delay);

	err = data->tf->read_adc(data, val);
	if (err < 0) {
		return err;
	}

	return 0;
}

static int ms5607_sample_fetch(const struct device *dev,
			       enum sensor_channel channel)
{
	struct ms5607_data *data = dev->data;
	int err;
	uint32_t adc_pressure, adc_temperature;

	__ASSERT_NO_MSG(channel == SENSOR_CHAN_ALL);

	err = ms5607_get_measurement(data,
				     &adc_pressure,
				     data->pressure_conv_cmd,
				     data->pressure_conv_delay);
	if (err < 0) {
		return err;
	}

	err = ms5607_get_measurement(data,
				     &adc_temperature,
				     data->temperature_conv_cmd,
				     data->temperature_conv_delay);
	if (err < 0) {
		return err;
	}

	ms5607_compensate(data, adc_temperature, adc_pressure);
	return 0;
}

static int ms5607_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	const struct ms5607_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		val->val1 = data->temperature / 100;
		val->val2 = data->temperature % 100 * 10000;
		break;
	case SENSOR_CHAN_PRESS:
		val->val1 = data->pressure / 100;
		val->val2 = data->pressure % 100 * 10000;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int ms5607_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr,
			   const struct sensor_value *val)
{
	struct ms5607_data *data = dev->data;
	uint8_t p_conv_cmd, t_conv_cmd, conv_delay;

	if (attr != SENSOR_ATTR_OVERSAMPLING) {
		return -ENOTSUP;
	}

	switch (val->val1) {
	case 4096:
		p_conv_cmd = MS5607_CMD_CONV_P_4096;
		t_conv_cmd = MS5607_CMD_CONV_T_4096;
		conv_delay = 9U;
		break;
	case 2048:
		p_conv_cmd = MS5607_CMD_CONV_P_2048;
		t_conv_cmd = MS5607_CMD_CONV_T_2048;
		conv_delay = 5U;
		break;
	case 1024:
		p_conv_cmd = MS5607_CMD_CONV_P_1024;
		t_conv_cmd = MS5607_CMD_CONV_T_1024;
		conv_delay = 3U;
		break;
	case 512:
		p_conv_cmd = MS5607_CMD_CONV_P_512;
		t_conv_cmd = MS5607_CMD_CONV_T_512;
		conv_delay = 2U;
		break;
	case 256:
		p_conv_cmd = MS5607_CMD_CONV_P_256;
		t_conv_cmd = MS5607_CMD_CONV_T_256;
		conv_delay = 1U;
		break;
	default:
		LOG_ERR("invalid oversampling rate %d", val->val1);
		return -EINVAL;
	}

	switch (chan) {
	case SENSOR_CHAN_ALL:
		data->pressure_conv_cmd = p_conv_cmd;
		data->temperature_conv_cmd = t_conv_cmd;
		data->temperature_conv_delay = conv_delay;
		data->pressure_conv_delay = conv_delay;
		break;
	case SENSOR_CHAN_PRESS:
		data->pressure_conv_cmd = p_conv_cmd;
		data->pressure_conv_delay = conv_delay;
		break;
	case SENSOR_CHAN_AMBIENT_TEMP:
		data->temperature_conv_cmd = t_conv_cmd;
		data->temperature_conv_delay = conv_delay;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct ms5607_config ms5607_config = {
	.ms5607_device_name = DT_INST_BUS_LABEL(0),
};

static int ms5607_init(const struct device *dev)
{
	const struct ms5607_config *const config = dev->config;
	struct ms5607_data *data = dev->data;
	struct sensor_value val;
	int err;

	data->ms5607_device = device_get_binding(config->ms5607_device_name);
	if (!data->ms5607_device) {
		LOG_ERR("master not found: %s", config->ms5607_device_name);
		return -EINVAL;
	}

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	ms5607_spi_init(dev);
#else
	BUILD_ASSERT(1, "I2c interface not implemented yet");
#endif

	data->pressure = 0;
	data->temperature = 0;


	val.val1 = MS5607_PRES_OVER_DEFAULT;
	err = ms5607_attr_set(dev, SENSOR_CHAN_PRESS, SENSOR_ATTR_OVERSAMPLING,
			      &val);
	if (err < 0) {
		return err;
	}

	val.val1 = MS5607_TEMP_OVER_DEFAULT;
	err = ms5607_attr_set(dev, SENSOR_CHAN_AMBIENT_TEMP,
			      SENSOR_ATTR_OVERSAMPLING, &val);
	if (err < 0) {
		return err;
	}

	err = data->tf->reset(data);
	if (err < 0) {
		return err;
	}

	k_sleep(K_MSEC(2));

	err = ms5607_read_prom(data, MS5607_CMD_CONV_READ_OFF_T1,
			       &data->off_t1);
	if (err < 0) {
		return err;
	}

	LOG_DBG("OFF_T1: %d", data->off_t1);

	err = ms5607_read_prom(data, MS5607_CMD_CONV_READ_SENSE_T1,
			       &data->sens_t1);
	if (err < 0) {
		return err;
	}

	LOG_DBG("SENSE_T1: %d", data->sens_t1);

	err = ms5607_read_prom(data, MS5607_CMD_CONV_READ_T_REF, &data->t_ref);
	if (err < 0) {
		return err;
	}

	LOG_DBG("T_REF: %d", data->t_ref);

	err = ms5607_read_prom(data, MS5607_CMD_CONV_READ_TCO, &data->tco);
	if (err < 0) {
		return err;
	}

	LOG_DBG("TCO: %d", data->tco);

	err = ms5607_read_prom(data, MS5607_CMD_CONV_READ_TCS, &data->tcs);
	if (err < 0) {
		return err;
	}

	LOG_DBG("TCS: %d", data->tcs);

	err = ms5607_read_prom(data, MS5607_CMD_CONV_READ_TEMPSENS,
			       &data->tempsens);
	if (err < 0) {
		return err;
	}

	LOG_DBG("TEMPSENS: %d", data->tempsens);

	return 0;
}

static const struct sensor_driver_api ms5607_api_funcs = {
	.attr_set = ms5607_attr_set,
	.sample_fetch = ms5607_sample_fetch,
	.channel_get = ms5607_channel_get,
};

static struct ms5607_data ms5607_data;

DEVICE_DT_INST_DEFINE(0,
		    ms5607_init,
		    device_pm_control_nop,
		    &ms5607_data,
		    &ms5607_config,
		    POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY,
		    &ms5607_api_funcs);
