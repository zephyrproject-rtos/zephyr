/* Driver for MS5837 pressure sensor
 *
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT meas_ms5837

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

#include "ms5837.h"

LOG_MODULE_REGISTER(MS5837, CONFIG_SENSOR_LOG_LEVEL);

static int ms5837_get_measurement(const struct device *dev, uint32_t *val,
				  uint8_t cmd, const uint8_t delay)
{
	const struct ms5837_config *cfg = dev->config;
	uint8_t adc_read_cmd = MS5837_CMD_CONV_READ_ADC;
	int err;

	*val = 0U;

	err = i2c_write_dt(&cfg->i2c, &cmd, 1);
	if (err < 0) {
		return err;
	}

	k_msleep(delay);

	err = i2c_burst_read_dt(&cfg->i2c, adc_read_cmd, ((uint8_t *)val) + 1,
				3);
	if (err < 0) {
		return err;
	}

	*val = sys_be32_to_cpu(*val);

	return 0;
}

static void ms5837_compensate_30(const struct device *dev,
				 const int32_t adc_temperature,
				 const int32_t adc_pressure)
{
	struct ms5837_data *data = dev->data;
	int64_t dT;
	int64_t OFF;
	int64_t SENS;
	int64_t temp_sq;
	int32_t Ti;
	int32_t OFFi;
	int32_t SENSi;

	/* first order compensation as per datasheet
	 * (https://www.te.com/usa-en/product-CAT-BLPS0017.html) section
	 * PRESSURE AND TEMPERATURE CALCULATION
	 */

	dT = adc_temperature - ((int32_t)(data->t_ref) << 8);
	data->temperature = 2000 + (dT * data->tempsens) / (1ll << 23);
	OFF = ((int64_t)(data->off_t1) << 16) + (dT * data->tco) / (1ll << 7);
	SENS = ((int64_t)(data->sens_t1) << 15) + (dT * data->tcs) / (1ll << 8);

	/* Second order compensation as per datasheet
	 * (https://www.te.com/usa-en/product-CAT-BLPS0017.html) section
	 * SECOND ORDER TEMPERATURE COMPENSATION
	 */

	temp_sq = (int64_t)(data->temperature - 2000) * (data->temperature - 2000);
	if (data->temperature < 2000) {
		Ti = (3ll * dT * dT) / (1ll << 33);
		OFFi = (3ll * temp_sq) / (1ll << 1);
		SENSi = (5ll * temp_sq) / (1ll << 3);
		if (data->temperature < -1500) {
			temp_sq = (data->temperature + 1500) *
				  (data->temperature + 1500);
			OFFi += 7ll * temp_sq;
			SENSi += 4ll * temp_sq;
		}
	} else {
		Ti = (2ll * dT * dT) / (1ll << 37);
		OFFi = temp_sq / (1ll << 4);
		SENSi = 0;
	}

	OFF -= OFFi;
	SENS -= SENSi;

	data->temperature -= Ti;
	data->pressure =
	    (((SENS * adc_pressure) / (1ll << 21)) - OFF) / (1ll << 13);
}

/*
 * First and second order pressure and temperature calculations, as per the flowchart in the
 * MS5837-02B datasheet. (see "Pressure and Temperature Calculation", pages 6 and 7, REV a8 12/2019)
 */
static void ms5837_compensate_02(const struct device *dev,
				 const int32_t adc_temperature,
				 const int32_t adc_pressure)
{
	struct ms5837_data *data = dev->data;
	int64_t dT;
	int64_t OFF;
	int64_t SENS;
	int64_t temp_sq;
	int32_t Ti;
	int32_t OFFi;
	int32_t SENSi;

	dT = adc_temperature - ((int32_t)(data->t_ref) << 8);
	data->temperature = 2000 + (dT * data->tempsens) / (1ll << 23);
	OFF = ((int64_t)(data->off_t1) << 17) + (dT * data->tco) / (1ll << 6);
	SENS = ((int64_t)(data->sens_t1) << 16) + (dT * data->tcs) / (1ll << 7);

	temp_sq = (int64_t)(data->temperature - 2000) * (data->temperature - 2000);
	if (data->temperature < 2000) {
		Ti = (11ll * dT * dT) / (1ll << 35);
		OFFi = (31ll * temp_sq) / (1ll << 3);
		SENSi = (63ll * temp_sq) / (1ll << 5);
	} else {
		Ti = 0;
		OFFi = 0;
		SENSi = 0;
	}

	OFF -= OFFi;
	SENS -= SENSi;

	data->temperature -= Ti;
	data->pressure = (((SENS * adc_pressure) / (1ll << 21)) - OFF) / (1ll << 15);
}

static int ms5837_sample_fetch(const struct device *dev,
			       enum sensor_channel channel)
{
	struct ms5837_data *data = dev->data;
	int err;
	uint32_t adc_pressure;
	uint32_t adc_temperature;

	__ASSERT_NO_MSG(channel == SENSOR_CHAN_ALL);

	err = ms5837_get_measurement(dev, &adc_pressure, data->presure_conv_cmd,
				     data->presure_conv_delay);
	if (err < 0) {
		return err;
	}

	err = ms5837_get_measurement(dev, &adc_temperature,
				     data->temperature_conv_cmd,
				     data->temperature_conv_delay);
	if (err < 0) {
		return err;
	}

	data->comp_func(dev, adc_temperature, adc_pressure);

	return 0;
}

static int ms5837_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct ms5837_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		/* Internal temperature is in 100ths of deg C */
		val->val1 = data->temperature / 100;
		val->val2 = data->temperature % 100 * 10000;
		break;
	case SENSOR_CHAN_PRESS:
		/* Internal value is (mbar * 100), so factor to kPa is 1000 */
		val->val1 = data->pressure / 1000;
		val->val2 = data->pressure % 1000 * 1000;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int ms5837_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr,
			   const struct sensor_value *val)
{
	struct ms5837_data *data = dev->data;
	uint8_t p_conv_cmd;
	uint8_t t_conv_cmd;
	uint8_t conv_delay;

	if (attr == SENSOR_ATTR_OVERSAMPLING) {

		switch (val->val1) {
		case 8192:
			p_conv_cmd = MS5837_CMD_CONV_P_8192;
			t_conv_cmd = MS5837_CMD_CONV_T_8192;
			conv_delay = 19U;
			break;
		case 4096:
			p_conv_cmd = MS5837_CMD_CONV_P_4096;
			t_conv_cmd = MS5837_CMD_CONV_T_4096;
			conv_delay = 10U;
			break;
		case 2048:
			p_conv_cmd = MS5837_CMD_CONV_P_2048;
			t_conv_cmd = MS5837_CMD_CONV_T_2048;
			conv_delay = 5U;
			break;
		case 1024:
			p_conv_cmd = MS5837_CMD_CONV_P_1024;
			t_conv_cmd = MS5837_CMD_CONV_T_1024;
			conv_delay = 3U;
			break;
		case 512:
			p_conv_cmd = MS5837_CMD_CONV_P_512;
			t_conv_cmd = MS5837_CMD_CONV_T_512;
			conv_delay = 2U;
			break;
		case 256:
			p_conv_cmd = MS5837_CMD_CONV_P_256;
			t_conv_cmd = MS5837_CMD_CONV_T_256;
			conv_delay = 1U;
			break;
		default:
			LOG_ERR("invalid oversampling rate %d", val->val1);
			return -EINVAL;
		}

		if (chan == SENSOR_CHAN_ALL) {
			data->presure_conv_cmd = p_conv_cmd;
			data->presure_conv_delay = conv_delay;
			data->temperature_conv_cmd = t_conv_cmd;
			data->temperature_conv_delay = conv_delay;
			return 0;
		}

		if (chan == SENSOR_CHAN_PRESS) {
			data->presure_conv_cmd = p_conv_cmd;
			data->presure_conv_delay = conv_delay;
			return 0;
		}

		if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
			data->temperature_conv_cmd = t_conv_cmd;
			data->temperature_conv_delay = conv_delay;
			return 0;
		}

		return -ENOTSUP;
	}

	return -ENOTSUP;
}

static const struct sensor_driver_api ms5837_api_funcs = {
	.attr_set = ms5837_attr_set,
	.sample_fetch = ms5837_sample_fetch,
	.channel_get = ms5837_channel_get,
};

static int ms5837_read_prom(const struct device *dev, const uint8_t cmd,
			    uint16_t *val)
{
	const struct ms5837_config *cfg = dev->config;
	int err;

	err = i2c_burst_read_dt(&cfg->i2c, cmd, (uint8_t *)val, 2);
	if (err < 0) {
		return err;
	}

	*val = sys_be16_to_cpu(*val);

	return 0;
}

static int ms5837_init(const struct device *dev)
{
	struct ms5837_data *data = dev->data;
	const struct ms5837_config *cfg = dev->config;
	int err;
	uint8_t cmd;

	data->pressure = 0;
	data->temperature = 0;

	data->presure_conv_cmd = MS5837_CMD_CONV_P_256;
	data->presure_conv_delay = 1U;
	data->temperature_conv_cmd = MS5837_CMD_CONV_T_256;
	data->temperature_conv_delay = 1U;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	cmd = MS5837_CMD_RESET;
	err = i2c_write_dt(&cfg->i2c, &cmd, 1);
	if (err < 0) {
		return err;
	}

	err = ms5837_read_prom(dev, MS5837_CMD_CONV_READ_CRC, &data->factory);
	if (err < 0) {
		LOG_ERR("couldn't read device info");
		return err;
	}

	err = ms5837_read_prom(dev, MS5837_CMD_CONV_READ_SENS_T1, &data->sens_t1);
	if (err < 0) {
		return err;
	}

	err = ms5837_read_prom(dev, MS5837_CMD_CONV_READ_OFF_T1, &data->off_t1);
	if (err < 0) {
		return err;
	}

	err = ms5837_read_prom(dev, MS5837_CMD_CONV_READ_TCS, &data->tcs);
	if (err < 0) {
		return err;
	}

	err = ms5837_read_prom(dev, MS5837_CMD_CONV_READ_TCO, &data->tco);
	if (err < 0) {
		return err;
	}

	err = ms5837_read_prom(dev, MS5837_CMD_CONV_READ_T_REF, &data->t_ref);
	if (err < 0) {
		return err;
	}

	err = ms5837_read_prom(dev, MS5837_CMD_CONV_READ_TEMPSENS,
			       &data->tempsens);
	if (err < 0) {
		return err;
	}

	const int type_id = (data->factory >> 5) & 0x7f;

	switch (type_id) {
	case  MS5837_02BA01:
	case MS5837_02BA21:
		data->comp_func = ms5837_compensate_02;
		break;
	case MS5837_30BA26:
		data->comp_func = ms5837_compensate_30;
		break;
	default:
		LOG_WRN(" unrecognized type: '%2x', defaulting to MS5837-30", type_id);
		data->comp_func = ms5837_compensate_30;
		break;
	}

	return 0;
}

#define MS5837_DEFINE(inst)								\
	static struct ms5837_data ms5837_data_##inst;					\
											\
	static const struct ms5837_config ms5837_config_##inst = {			\
		.i2c = I2C_DT_SPEC_INST_GET(inst),					\
	};										\
											\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, ms5837_init, NULL,				\
			      &ms5837_data_##inst, &ms5837_config_##inst, POST_KERNEL,	\
			      CONFIG_SENSOR_INIT_PRIORITY, &ms5837_api_funcs);		\

DT_INST_FOREACH_STATUS_OKAY(MS5837_DEFINE)
