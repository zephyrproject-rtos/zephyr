/* veml7700.c - driver for high accuracy ALS */
/*
 * Copyright (c) 2022 innblue
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT vishay_veml7700

#include "veml7700.h"
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <math.h>
#include <init.h>
#include <kernel.h>
#include <sys/byteorder.h>
#include <sys/__assert.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(veml7700, CONFIG_SENSOR_LOG_LEVEL);

#define LX_STEP_10K 36UL
#define MUL_LX_GAIN2(X) (X * 1)
#define MUL_LX_GAIN1(X) (X * 2)
#define MUL_LX_GAIN1_4(X) (X * 8)
#define MUL_LX_GAIN1_8(X) (X * 16)

#define MUL_LX_IT800ms(X) (X * 1)
#define MUL_LX_IT400ms(X) (X * 2)
#define MUL_LX_IT200ms(X) (X * 4)
#define MUL_LX_IT100ms(X) (X * 8)
#define MUL_LX_IT50ms(X) (X * 16)
#define MUL_LX_IT25ms(X) (X * 32)

#define IT_CALIB_SHIFT 2
#define G_CALIB_SHIFT 1

static const enum veml7700_it calibration_it_values[] = { VEML7700_IT_25MS,  VEML7700_IT_50MS,
							  VEML7700_IT_100MS, VEML7700_IT_200MS,
							  VEML7700_IT_400MS, VEML7700_IT_800MS };
static const enum veml7700_gain calibration_gain_values[] = { VEML7700_GAIN_1_8, VEML7700_GAIN_1_4,
							      VEML7700_GAIN_1, VEML7700_GAIN_2 };

static int veml7700_reg_read(const struct device *dev, uint8_t reg, uint8_t *buf, size_t size)
{
	struct veml7700_data *data = (struct veml7700_data *)dev->data;
	const struct veml7700_config *cfg = (struct veml7700_config *)dev->config;

	return i2c_burst_read(data->i2c, cfg->i2c_addr, reg, buf, size);
}

static int veml7700_reg_write(const struct device *dev, uint8_t reg, uint8_t *val_u16)
{
	struct veml7700_data *data = (struct veml7700_data *)dev->data;
	const struct veml7700_config *cfg = (struct veml7700_config *)dev->config;

	uint8_t buf[3] = { reg, val_u16[0], val_u16[1] };
	return i2c_write(data->i2c, buf, sizeof(buf), cfg->i2c_addr);
}

static double veml7700_correction_formula(double veml_lux)
{
	return (6.0135 * powf(10, -13) * powf(veml_lux, 4) -
		9.3924 * powf(10, -9) * powf(veml_lux, 3) +
		8.1448 * powf(10, -5) * powf(veml_lux, 2) + 1.0023 * powf(10, 0) * veml_lux);
}

static double veml7700_get_mult_lux_k(struct veml7700_data *data)
{
	uint16_t k = 1;
	switch (data->als_gain) {
	case VEML7700_GAIN_1:
		k = MUL_LX_GAIN1(k);
		break;
	case VEML7700_GAIN_2:
		k = MUL_LX_GAIN2(k);
		break;
	case VEML7700_GAIN_1_4:
		k = MUL_LX_GAIN1_4(k);
		break;
	case VEML7700_GAIN_1_8:
		k = MUL_LX_GAIN1_8(k);
		break;
	}

	switch (data->als_it) {
	case VEML7700_IT_100MS:
		k = MUL_LX_IT100ms(k);
		break;
	case VEML7700_IT_200MS:
		k = MUL_LX_IT200ms(k);
		break;
	case VEML7700_IT_400MS:
		k = MUL_LX_IT400ms(k);
		break;
	case VEML7700_IT_800MS:
		k = MUL_LX_IT800ms(k);
		break;
	case VEML7700_IT_50MS:
		k = MUL_LX_IT50ms(k);
		break;
	case VEML7700_IT_25MS:
		k = MUL_LX_IT25ms(k);
		break;
	}

	return k * LX_STEP_10K / 10000.0;
}

static void veml7700_calc_als(struct veml7700_data *data, struct sensor_value *val)
{
	double lux = data->als * veml7700_get_mult_lux_k(data);

	if (data->als_gain != VEML7700_GAIN_2 && data->als_it != VEML7700_IT_800MS)
		lux = veml7700_correction_formula(lux);

	sensor_value_from_double(val, lux);
	LOG_DBG("VEML >> CTS: %u(0x%04X), Lux: %u.%u", data->als, data->als, val->val1, val->val2);
}

static uint16_t veml7700_calc_raw_from_lux(struct veml7700_data *data, double lux)
{
	return (uint16_t)(lux / veml7700_get_mult_lux_k(data));
}

static int veml7700_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct veml7700_data *data = (struct veml7700_data *)dev->data;

	uint16_t raw_val;
	int ret = veml7700_reg_read(dev, VEML7700_ALS_DATA, (uint8_t *)&raw_val, sizeof(uint16_t));
	data->als = sys_le16_to_cpu(raw_val);

	uint16_t reg0_val;
	ret = veml7700_reg_read(dev, VEML7700_ALS_CONFIG, (uint8_t *)&reg0_val, sizeof(reg0_val));
	if (ret < 0) {
		LOG_ERR("VEML >> %s >> Could not read reg0(%d)", __func__, ret);
	}
	data->als_gain = VEMEL7700_GET_GAIN(reg0_val);
	data->als_it = VEMEL7700_GET_IT(reg0_val);
	LOG_DBG("VEML >> %s >> Reg0:"
		"\t\t On: %s\n"
		"\t\t Int: %s\n"
		"\t\t GAIN: %u\n"
		"\t\t IT: %u\n",
		__func__, VEMEL7700_GET_SD(reg0_val) ? "off" : "on",
		VEMEL7700_GET_INT(reg0_val) ? "enable" : "disable", VEMEL7700_GET_GAIN(reg0_val),
		VEMEL7700_GET_IT(reg0_val));
	return ret;
}

static int veml7700_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct veml7700_data *data = (struct veml7700_data *)dev->data;

	if (SENSOR_CHAN_LIGHT != chan)
		return -EINVAL;

	veml7700_calc_als(data, val);

	return 0;
}

static int veml7700_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	struct veml7700_data *data = (struct veml7700_data *)dev->data;
	if (SENSOR_CHAN_LIGHT != chan)
		return -EINVAL;
	int ret;
	double thrs_lux = sensor_value_to_double(val);
	uint16_t thrs_val = veml7700_calc_raw_from_lux(data, thrs_lux);
	LOG_DBG("VEML >> Threshold value >> lux %0.5f | raw %u(0x%02X)", thrs_lux, thrs_val,
		thrs_val);
	switch (attr) {
	case (SENSOR_ATTR_LOWER_THRESH):
		ret = veml7700_reg_write(dev, VEML7700_ALS_THREHOLD_LOW, (uint8_t *)&thrs_val);
		break;
	case (SENSOR_ATTR_UPPER_THRESH):
		ret = veml7700_reg_write(dev, VEML7700_ALS_THREHOLD_HIGH, (uint8_t *)&thrs_val);
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}

/**
 * @note:
 * 	For a wide light detection range of more than seven decades
 * 	(from 0.007 lx to 120 klx), it is necessary to adjust the sensor.
 * 	Thisis done with the help of four gain steps and seven steps for the
 * 	integration time.
 * @param dev - VEML7700 device driver
 * @return 0 on success, -EINVAL on error
 */
static int veml7700_startup_calibration(const struct device *dev)
{
	struct veml7700_data *data = (struct veml7700_data *)dev->data;

	uint8_t step = 0;
	int ret;
	uint16_t reg0_val = 0;

	reg0_val = VEMEL7700_SET_GAIN(reg0_val, VEML7700_GAIN_1_8);
	reg0_val = VEMEL7700_SET_IT(reg0_val, VEML7700_IT_100MS);
	reg0_val = VEMEL7700_TURN_ON(reg0_val);

	ret = veml7700_reg_write(dev, VEML7700_ALS_CONFIG, (uint8_t *)&reg0_val);
	if (ret < 0) {
		LOG_ERR("VEML >> %s >> Could not write reg0(%d)", __func__, ret);
	}

	/********** DBG ******************/
	ret = veml7700_reg_read(dev, VEML7700_ALS_CONFIG, (uint8_t *)&reg0_val, sizeof(reg0_val));
	if (ret < 0) {
		LOG_ERR("VEML >> %s >> Could not read reg0(%d)", __func__, ret);
	}
	/********* END ******************/
	data->als_it = VEML7700_IT_100MS;
	data->als_gain = VEML7700_GAIN_1_8;
	k_sleep(K_MSEC(125));
	int8_t i = 1;
	int8_t g_val = i + G_CALIB_SHIFT;
	for (; i < ARRAY_SIZE(calibration_gain_values); i++) {
		ret = veml7700_sample_fetch(dev, 0);
		if (ret < 0)
			return -EINVAL;
		step++;
		LOG_DBG("VEML >> %s >> \n"
			"\t\tStep: %u - Raw val: %u\n"
			"\t\tG: %u - IT %u",
			__func__, step, data->als, data->als_gain, data->als_it);
		if (data->als > 100) {
			g_val = i + G_CALIB_SHIFT;
			break;
		} else {
			reg0_val = VEMEL7700_CLEAR_GAIN(reg0_val);
			reg0_val = VEMEL7700_SET_GAIN(reg0_val, calibration_gain_values[i]);
			reg0_val = VEMEL7700_SHUT_DOWN(reg0_val);
			ret = veml7700_reg_write(dev, VEML7700_ALS_CONFIG, (uint8_t *)&reg0_val);
			k_sleep(K_MSEC(15));
			reg0_val = VEMEL7700_TURN_ON(reg0_val);
			ret = veml7700_reg_write(dev, VEML7700_ALS_CONFIG, (uint8_t *)&reg0_val);
			LOG_DBG("VEML >> %s >> Reg to write 0x%04X", __func__, reg0_val);
			/********** DBG ******************/
			ret = veml7700_reg_read(dev, VEML7700_ALS_CONFIG, (uint8_t *)&reg0_val,
						sizeof(reg0_val));
			if (ret < 0) {
				LOG_ERR("VEML >> %s >> Could not read reg0(%d)", __func__, ret);
			}
			LOG_DBG("VEML >> %s >> Raw val 0x%04X => CPU val 0x%04X \n"
				"=>> GAIN: %u IT: %u",
				__func__, reg0_val, sys_le16_to_cpu(reg0_val),
				VEMEL7700_GET_GAIN(reg0_val), VEMEL7700_GET_IT(reg0_val));
			/********* END ******************/

			g_val = i + G_CALIB_SHIFT;
			data->als_gain = calibration_gain_values[i];
			k_sleep(K_MSEC(100));
		}
	}
	i = 0 + IT_CALIB_SHIFT;
	if (g_val == 4) {
		for (; i < ARRAY_SIZE(calibration_it_values); i++) {
			ret = veml7700_sample_fetch(dev, 0);
			if (ret < 0)
				return -EINVAL;
			step++;
			LOG_DBG("VEML >> %s >> \n"
				"\t\tStep: %u - Raw val: %u\n"
				"\t\tG: %u - IT %u",
				__func__, step, data->als, data->als_gain, data->als_it);
			if (data->als > 100) {
				break;
			} else {
				reg0_val = VEMEL7700_CLEAR_IT(reg0_val);
				reg0_val = VEMEL7700_SET_IT(reg0_val, calibration_it_values[i]);
				reg0_val = VEMEL7700_SHUT_DOWN(reg0_val);
				ret = veml7700_reg_write(dev, VEML7700_ALS_CONFIG,
							 (uint8_t *)&reg0_val);
				k_sleep(K_MSEC(15));
				reg0_val = VEMEL7700_TURN_ON(reg0_val);
				ret = veml7700_reg_write(dev, VEML7700_ALS_CONFIG,
							 (uint8_t *)&reg0_val);
				LOG_DBG("VEML >> %s >> Reg to write 0x%04X", __func__, reg0_val);
				k_sleep(K_MSEC(230 * (i - IT_CALIB_SHIFT)));
				/********** DBG ******************/
				ret = veml7700_reg_read(dev, VEML7700_ALS_CONFIG,
							(uint8_t *)&reg0_val, sizeof(reg0_val));
				if (ret < 0) {
					LOG_ERR("VEML >> %s >> Could not read reg0(%d)", __func__,
						ret);
				}
				LOG_DBG("VEML >> %s >> Raw val 0x%04X => CPU val 0x%04X \n"
					"=>> GAIN: %u IT: %u",
					__func__, reg0_val, sys_le16_to_cpu(reg0_val),
					VEMEL7700_GET_GAIN(reg0_val), VEMEL7700_GET_IT(reg0_val));
				/********* END ******************/
				data->als_it = calibration_it_values[i];
			}
		}

		if (data->als < 10000 && i != (ARRAY_SIZE(calibration_it_values) + 1)) {
			return 0;
		}
	}

	for (; i >= 0; i--) {
		ret = veml7700_sample_fetch(dev, 0);
		if (ret < 0)
			return -EINVAL;
		step++;
		LOG_DBG("VEML >> %s >> \n"
			"\t\tStep: %u - Raw val: %u\n"
			"\t\tG: %u - IT %u",
			__func__, step, data->als, data->als_gain, data->als_it);
		if (data->als < 10000) {
			break;
		} else {
			reg0_val = VEMEL7700_CLEAR_IT(reg0_val);
			reg0_val = VEMEL7700_SET_IT(reg0_val, calibration_it_values[i]);
			reg0_val = VEMEL7700_SHUT_DOWN(reg0_val);
			ret = veml7700_reg_write(dev, VEML7700_ALS_CONFIG, (uint8_t *)&reg0_val);
			k_sleep(K_MSEC(15));
			reg0_val = VEMEL7700_TURN_ON(reg0_val);
			ret = veml7700_reg_write(dev, VEML7700_ALS_CONFIG, (uint8_t *)&reg0_val);
			LOG_DBG("VEML >> %s >> Reg to write 0x%04X", __func__, reg0_val);
			k_sleep(K_MSEC(230 * (i - IT_CALIB_SHIFT)));
			/********** DBG ******************/
			ret = veml7700_reg_read(dev, VEML7700_ALS_CONFIG, (uint8_t *)&reg0_val,
						sizeof(reg0_val));
			if (ret < 0) {
				LOG_ERR("VEML >> %s >> Could not read reg0(%d)", __func__, ret);
			}
			LOG_DBG("VEML >> %s >> Raw val 0x%04X => CPU val 0x%04X \n"
				"=>> GAIN: %u IT: %u",
				__func__, reg0_val, sys_le16_to_cpu(reg0_val),
				VEMEL7700_GET_GAIN(reg0_val), VEMEL7700_GET_IT(reg0_val));
			/********* END ******************/
			data->als_it = calibration_it_values[i];
		}
	}
	step++;
	LOG_DBG("VEML >> %s >> \n"
		"\t\tStep: %u - Raw val: %u\n"
		"\t\tG: %u - IT %u",
		__func__, step, data->als, data->als_gain, data->als_it);
	return 0;
}

static int veml7700_init(const struct device *dev)
{
	struct veml7700_data *data = (struct veml7700_data *)dev->data;
	const struct veml7700_config *cfg = (struct veml7700_config *)dev->config;

	int err = 0;

	/** bus init */
	data->i2c = device_get_binding(cfg->i2c_name);
	if (NULL == data->i2c) {
		LOG_ERR("VEML >> I2C master %s >> Not found", cfg->i2c_name);
		return -EINVAL;
	}

	LOG_DBG("VEML >> I2C >> Device ready");

#ifdef CONFIG_VEML7700_INT_ENABLE
	err = veml7700_interrupt_init(dev);
	if (err < 0) {
		LOG_DBG("VEML >> GPIO >> Interrupt configuration failed (%d)", err);
		return err;
	}
	LOG_DBG("VEML >> GPIO >> Interrupt configured");
#endif
	/**
	 * For a wide light detection range of more than seven decades
	 * (from 0.007 lx to 120 klx), it is necessary to adjust the sensor.
	 * Thisis done with the help of four gain steps and seven steps for
	 * the integration time.
	 * To deal with these steps, they are numberedas needed for
	 * the application software.
	 */
	LOG_DBG("VEML >> Starting calibration process");

#ifdef CONFIG_VEML7700_STARTUP_CALIBRATION_ENABLE
	err = veml7700_startup_calibration(dev);
	if (err < 0)
		LOG_WRN("VEML >> Calibration process failed (%d)", err);
#endif

	return 0;
}

static const struct sensor_driver_api veml7700_api_funcs = {
	.sample_fetch = veml7700_sample_fetch,
	.channel_get = veml7700_channel_get,
	.attr_set = veml7700_attr_set,
};

static struct veml7700_data veml7700_data;

static const struct veml7700_config veml7700_config = {
	.i2c_name = DT_INST_BUS_LABEL(0),
	.i2c_addr = DT_INST_REG_ADDR(0),
#ifdef CONFIG_VEML7700_INT_ENABLE
	.int_gpio = DT_INST_GPIO_LABEL(0, int_gpios),
	.int_gpio_pin = DT_INST_GPIO_PIN(0, int_gpios),
	.int_gpio_flags = DT_INST_GPIO_FLAGS(0, int_gpios)
#endif
};

DEVICE_DT_INST_DEFINE(0, veml7700_init, NULL, &veml7700_data, &veml7700_config, POST_KERNEL,
		      CONFIG_SENSOR_INIT_PRIORITY, &veml7700_api_funcs);
