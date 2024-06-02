/*
 * Copyright (c) 2019 Thomas Schmid <tom@lfence.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT meas_ms56xx

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/__assert.h>

#include "ms56xx.h"

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ms56xx);

static void ms56xx_compensate(struct ms56xx_data *data,
			      const struct ms56xx_chip_calc_coefficients *const coefficients,
			      const int32_t adc_temperature, const int32_t adc_pressure)
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

	dT = adc_temperature - ((int32_t)(data->t_ref) << 8);
	data->temperature = 2000 + (dT * data->tempsens) / (1ll << 23);
	OFF = ((int64_t)(data->off_t1) << coefficients->offset_temp_c2_factor_exp) +
	      (dT * data->tco) / (1ll << coefficients->offset_temp_c4_d2_div_exp);
	SENS = ((int64_t)(data->sens_t1) << coefficients->sens_c1_factor_exp) +
	       (dT * data->tcs) / (1ll << coefficients->sens_c3_dt_div_exp);

	/* Second order compensation as per datasheet
	 * (https://www.te.com/usa-en/product-CAT-BLPS0035.html) section
	 * SECOND ORDER TEMPERATURE COMPENSATION
	 */

	temp_sq = (int64_t)(data->temperature - 2000) * (int64_t)(data->temperature - 2000);
	if (data->temperature < 2000) {
		Ti = (dT * dT) / (1ll << 31);
		OFFi = (coefficients->low_temp_off2_factor * temp_sq) /
		       (1ll << coefficients->low_temp_off2_div_exp);
		SENSi = (coefficients->low_temp_sens2_factor * temp_sq) /
			(1ll << coefficients->low_temp_sens2_div_exp);
		if (data->temperature < -1500) {
			temp_sq = (int64_t)(data->temperature + 1500) *
				  (int64_t)(data->temperature + 1500);
			OFFi += coefficients->very_low_temp_off2_addend * temp_sq;
			SENSi += (coefficients->very_low_temp_sens2_addend * temp_sq) /
				 coefficients->very_low_temp_sens2_div;
		}
	} else {
		SENSi = 0;
		OFFi = 0;
		Ti = 0;
	}

	OFF -= OFFi;
	SENS -= SENSi;

	data->temperature -= Ti;
	data->pressure = (SENS * (int64_t)adc_pressure / (1ll << 21) - OFF) / (1ll << 15);
}

static int ms56xx_read_prom(const struct ms56xx_config *config, uint8_t cmd, uint16_t *val)
{
	int err;

	err = config->tf->read_prom(config, cmd, val);
	if (err < 0) {
		LOG_ERR("Error reading prom");
		return err;
	}

	return 0;
}

static int ms56xx_get_measurement(const struct ms56xx_config *config, uint32_t *val, uint8_t cmd,
				  uint8_t delay)
{
	int err;

	*val = 0U;

	err = config->tf->start_conversion(config, cmd);
	if (err < 0) {
		return err;
	}

	k_msleep(delay);

	err = config->tf->read_adc(config, val);
	if (err < 0) {
		return err;
	}

	return 0;
}

static int ms56xx_sample_fetch(const struct device *dev, enum sensor_channel channel)
{
	const struct ms56xx_config *config = dev->config;
	struct ms56xx_data *data = dev->data;
	int err;
	uint32_t adc_pressure, adc_temperature;

	__ASSERT_NO_MSG(channel == SENSOR_CHAN_ALL);

	err = ms56xx_get_measurement(config, &adc_pressure, data->pressure_conv_cmd,
				     data->pressure_conv_delay);
	if (err < 0) {
		return err;
	}

	err = ms56xx_get_measurement(config, &adc_temperature, data->temperature_conv_cmd,
				     data->temperature_conv_delay);
	if (err < 0) {
		return err;
	}

	ms56xx_compensate(data, &config->calc_coefficients, adc_temperature, adc_pressure);
	return 0;
}

static int ms56xx_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	const struct ms56xx_data *data = dev->data;

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
		return -ENOTSUP;
	}

	return 0;
}

static int ms56xx_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
{
	struct ms56xx_data *data = dev->data;
	uint8_t p_conv_cmd, t_conv_cmd, conv_delay;

	if (attr != SENSOR_ATTR_OVERSAMPLING) {
		return -ENOTSUP;
	}

	switch (val->val1) {
	case 4096:
		p_conv_cmd = MS56XX_CMD_CONV_P_4096;
		t_conv_cmd = MS56XX_CMD_CONV_T_4096;
		conv_delay = 9U;
		break;
	case 2048:
		p_conv_cmd = MS56XX_CMD_CONV_P_2048;
		t_conv_cmd = MS56XX_CMD_CONV_T_2048;
		conv_delay = 5U;
		break;
	case 1024:
		p_conv_cmd = MS56XX_CMD_CONV_P_1024;
		t_conv_cmd = MS56XX_CMD_CONV_T_1024;
		conv_delay = 3U;
		break;
	case 512:
		p_conv_cmd = MS56XX_CMD_CONV_P_512;
		t_conv_cmd = MS56XX_CMD_CONV_T_512;
		conv_delay = 2U;
		break;
	case 256:
		p_conv_cmd = MS56XX_CMD_CONV_P_256;
		t_conv_cmd = MS56XX_CMD_CONV_T_256;
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

static int ms56xx_init(const struct device *dev)
{
	const struct ms56xx_config *const config = dev->config;
	struct ms56xx_data *data = dev->data;
	struct sensor_value val;
	int err;

	err = config->tf->bus_check(config);
	if (err < 0) {
		return err;
	}

	data->pressure = 0;
	data->temperature = 0;

	val.val1 = MS56XX_PRES_OVER_DEFAULT;
	err = ms56xx_attr_set(dev, SENSOR_CHAN_PRESS, SENSOR_ATTR_OVERSAMPLING, &val);
	if (err < 0) {
		return err;
	}

	val.val1 = MS56XX_TEMP_OVER_DEFAULT;
	err = ms56xx_attr_set(dev, SENSOR_CHAN_AMBIENT_TEMP, SENSOR_ATTR_OVERSAMPLING, &val);
	if (err < 0) {
		return err;
	}

	err = config->tf->reset(config);
	if (err < 0) {
		return err;
	}

	k_sleep(K_MSEC(2));

	err = ms56xx_read_prom(config, MS56XX_CMD_CONV_READ_OFF_T1, &data->off_t1);
	if (err < 0) {
		return err;
	}

	LOG_DBG("OFF_T1: %d", data->off_t1);

	err = ms56xx_read_prom(config, MS56XX_CMD_CONV_READ_SENSE_T1, &data->sens_t1);
	if (err < 0) {
		return err;
	}

	LOG_DBG("SENSE_T1: %d", data->sens_t1);

	err = ms56xx_read_prom(config, MS56XX_CMD_CONV_READ_T_REF, &data->t_ref);
	if (err < 0) {
		return err;
	}

	LOG_DBG("T_REF: %d", data->t_ref);

	err = ms56xx_read_prom(config, MS56XX_CMD_CONV_READ_TCO, &data->tco);
	if (err < 0) {
		return err;
	}

	LOG_DBG("TCO: %d", data->tco);

	err = ms56xx_read_prom(config, MS56XX_CMD_CONV_READ_TCS, &data->tcs);
	if (err < 0) {
		return err;
	}

	LOG_DBG("TCS: %d", data->tcs);

	err = ms56xx_read_prom(config, MS56XX_CMD_CONV_READ_TEMPSENS, &data->tempsens);
	if (err < 0) {
		return err;
	}

	LOG_DBG("TEMPSENS: %d", data->tempsens);

	return 0;
}

static const struct sensor_driver_api ms56xx_api_funcs = {
	.attr_set = ms56xx_attr_set,
	.sample_fetch = ms56xx_sample_fetch,
	.channel_get = ms56xx_channel_get,
};

#define MS56XX_SPI_OPERATION                                                                       \
	(SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_TRANSFER_MSB)

/* Initializes a struct ms56xx_config for an instance on a SPI bus. */
#define MS56XX_CONFIG_SPI(inst)                                                                    \
	{                                                                                          \
		.tf = &ms56xx_spi_transfer_function,                                               \
		.bus_cfg.spi = SPI_DT_SPEC_INST_GET(inst, MS56XX_SPI_OPERATION, 0),                \
		.calc_coefficients = COND_CODE_1(                                                  \
			IS_EQ(DT_INST_PROP(inst, chip), MS56XX_5607_CHIP),                         \
			(MS56XX_CONFIG_07_COEFFICIENTS), (MS56XX_CONFIG_11_COEFFICIENTS))          \
	}

/* Initializes a struct ms56xx_config for an instance on a I2C bus. */
#define MS56XX_CONFIG_I2C(inst)                                                                    \
	{                                                                                          \
		.tf = &ms56xx_i2c_transfer_function, .bus_cfg.i2c = I2C_DT_SPEC_INST_GET(inst),    \
		.calc_coefficients = COND_CODE_1(                                                  \
			IS_EQ(DT_INST_PROP(inst, chip), MS56XX_5607_CHIP),                         \
			(MS56XX_CONFIG_07_COEFFICIENTS), (MS56XX_CONFIG_11_COEFFICIENTS))          \
	}

/* Initializes calculation coefficients based on chip version. */

#define MS56XX_CONFIG_07_COEFFICIENTS                                                              \
	(const struct ms56xx_chip_calc_coefficients)                                               \
	{                                                                                          \
		.offset_temp_c2_factor_exp = MS5607_OFFSET_TEMP_C2_FACTOR_EXP,                     \
		.offset_temp_c4_d2_div_exp = MS5607_OFFSET_TEMP_C4_D2_DIV_EXP,                     \
		.sens_c1_factor_exp = MS5607_SENS_C1_FACTOR_EXP,                                   \
		.sens_c3_dt_div_exp = MS5607_SENS_C3_DT_DIV,                                       \
		.low_temp_off2_factor = MS5607_LOW_TEMP_OFF2_FACTOR,                               \
		.low_temp_off2_div_exp = MS5607_LOW_TEMP_OFF2_DIV_EXP,                             \
		.low_temp_sens2_factor = MS5607_LOW_TEMP_SENS2_FACTOR,                             \
		.low_temp_sens2_div_exp = MS5607_LOW_TEMP_SENS2_DIV_EXP,                           \
		.very_low_temp_off2_addend = MS5607_VERY_LOW_TEMP_OFF2_ADDEND,                     \
		.very_low_temp_sens2_addend = MS5607_VERY_LOW_TEMP_SENS2_ADDEND,                   \
		.very_low_temp_sens2_div = MS5607_VERY_LOW_TEMP_SENS2_DIV                          \
	}

#define MS56XX_CONFIG_11_COEFFICIENTS                                                              \
	(const struct ms56xx_chip_calc_coefficients)                                               \
	{                                                                                          \
		.offset_temp_c2_factor_exp = MS5611_OFFSET_TEMP_C2_FACTOR_EXP,                     \
		.offset_temp_c4_d2_div_exp = MS5611_OFFSET_TEMP_C4_D2_DIV_EXP,                     \
		.sens_c1_factor_exp = MS5611_SENS_C1_FACTOR_EXP,                                   \
		.sens_c3_dt_div_exp = MS5611_SENS_C3_DT_DIV,                                       \
		.low_temp_off2_factor = MS5611_LOW_TEMP_OFF2_FACTOR,                               \
		.low_temp_off2_div_exp = MS5611_LOW_TEMP_OFF2_DIV_EXP,                             \
		.low_temp_sens2_factor = MS5611_LOW_TEMP_SENS2_FACTOR,                             \
		.low_temp_sens2_div_exp = MS5611_LOW_TEMP_SENS2_DIV_EXP,                           \
		.very_low_temp_off2_addend = MS5611_VERY_LOW_TEMP_OFF2_ADDEND,                     \
		.very_low_temp_sens2_addend = MS5611_VERY_LOW_TEMP_SENS2_ADDEND,                   \
		.very_low_temp_sens2_div = MS5611_VERY_LOW_TEMP_SENS2_DIV                          \
	}

/*
 * Main instantiation macro, which selects the correct bus-specific
 * instantiation macros for the instance.
 */
#define MS56XX_DEFINE(inst)                                                                        \
	static struct ms56xx_data ms56xx_data_##inst;                                              \
	static const struct ms56xx_config ms56xx_config_##inst = COND_CODE_1(                      \
		DT_INST_ON_BUS(inst, spi), (MS56XX_CONFIG_SPI(inst)), (MS56XX_CONFIG_I2C(inst)));  \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, ms56xx_init, NULL, &ms56xx_data_##inst,                 \
				     &ms56xx_config_##inst, POST_KERNEL,                           \
				     CONFIG_SENSOR_INIT_PRIORITY, &ms56xx_api_funcs);

/* Create the struct device for every status "okay" node in the devicetree. */
DT_INST_FOREACH_STATUS_OKAY(MS56XX_DEFINE)
