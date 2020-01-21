/* Driver for Bosch BMP388 Digital pressure sensor */

/*
 * Copyright (c) 2020 Ioannis Konstantelias
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <drivers/sensor.h>
#include <drivers/gpio.h>
#include <sys/__assert.h>

#include <drivers/i2c.h>
#include <logging/log.h>

#include "bmp388.h"

LOG_MODULE_REGISTER(BMP388, CONFIG_SENSOR_LOG_LEVEL);

#define BMP388_CONCAT_BYTES(b1, b2) (((u16_t)b1 << 8) | ((u16_t)b2))


#if BMP388_PRESS_EN
/*
 * Compensation code taken from BMP388 datasheet, Section 9.3.
 * "Pressure compesation".
 */
static void bmp388_compensate_press(struct bmp388_data *data, u32_t adc_press)
{
	float part_data1, part_data2, part_data3, part_data4;
	float part_out1, part_out2;

	struct bmp388_converted_coef *conv_coef = &data->conv_coef;
	float comp_temp = data->comp_temp;
	float comp_temp_pow2 = comp_temp * comp_temp;
	float comp_temp_pow3 = comp_temp_pow2 * comp_temp;

	part_data1 = conv_coef->par_p6 * comp_temp;
	part_data2 = conv_coef->par_p7 * comp_temp_pow2;
	part_data3 = conv_coef->par_p8 * comp_temp_pow3;
	part_out1 = conv_coef->par_p5 + part_data1 + part_data2 + part_data3;

	part_data1 = conv_coef->par_p2 * comp_temp;
	part_data2 = conv_coef->par_p3 * comp_temp_pow2;
	part_data3 = conv_coef->par_p4 * comp_temp_pow3;
	part_out2 = (float)adc_press *
		    (conv_coef->par_p1 + part_data1 + part_data2 + part_data3);

	part_data1 = (float)adc_press * (float)adc_press;
	part_data2 = conv_coef->par_p9 + conv_coef->par_p10 * comp_temp;
	part_data3 = part_data1 * part_data2;
	part_data4 = part_data3 +
		     ((float)adc_press * (float)adc_press * (float)adc_press) *
		     conv_coef->par_p11;

	data->comp_press = part_out1 + part_out2 + part_data4;
}
#endif /* BMP388_PRESS_EN */

#if BMP388_TEMP_EN
/*
 * Compensation code taken from BMP388 datasheet, Section 9.2.
 * "Temperature compesation".
 */
static void bmp388_compensate_temp(struct bmp388_data *data, u32_t adc_temp)
{
	float part_data1, part_data2;

	part_data1 = (float)(adc_temp - data->conv_coef.par_t1);
	part_data2 = (float)(part_data1 * data->conv_coef.par_t2);

	data->comp_temp = part_data2 + (part_data1 * part_data1) *
			  data->conv_coef.par_t3;
}
#endif /* BMP388_TEMP_EN */

/*
 * Calculate time in milliseconds before a measurement is ready according to
 * Section 3.9.2 "Measurement rate in forced mode and normal mode"
 */
static int calc_measurement_time(void)
{
	int t = 234U +
		(!!BMP388_PRESS_EN) *
		(392U + (1U << BMP388_PRESS_OVER) * 2000U) +
		(!!BMP388_TEMP_EN) *
		(313U + (1U << (BMP388_TEMP_OVER >> 3U)) * 2000U);

	return t / 1000U + ((t % 1000) > 0);
}

/*
 * Check the error register for possible sensor error conditions
 */
static int bmp388_check_error(struct device *dev)
{
	struct bmp388_data *data = dev->driver_data;
	int ret;
	u8_t err;

	ret = data->reg_read(dev, BMP388_REG_ERR, &err, 1);
	if (ret < 0) {
		return ret;
	}

	return err;
}

static int bmp388_opmode_set(struct device *dev, u8_t new_opmode)
{
	struct bmp388_data *data = dev->driver_data;
	int ret;

	u8_t pwr_ctrl, curr_opmode;

	/* Get current operating mode */
	ret = data->reg_read(dev, BMP388_REG_PWR_CTRL, &pwr_ctrl, 1);
	if (ret < 0) {
		return ret;
	}

	curr_opmode = pwr_ctrl & 0x30;

	if (curr_opmode == new_opmode) {
		/* Do nothing */
		return 0;
	}

	if (curr_opmode != BMP388_PWR_SLEEP) {
		/* Put device to sleep */
		ret = data->reg_write(dev, BMP388_REG_PWR_CTRL,
				      pwr_ctrl | BMP388_PWR_SLEEP);
		if (ret < 0) {
			LOG_ERR("Cannot set Sleep Mode");
			return ret;
		}

		/* Wait for some time */
		k_sleep(5);
	}

	if (new_opmode == BMP388_PWR_FORCED) {
		ret = data->reg_write(dev, BMP388_REG_PWR_CTRL,
				      pwr_ctrl | BMP388_PWR_FORCED);
		if (ret < 0) {
			LOG_ERR("Cannot set Forced Mode");
			return ret;
		}
	} else if (new_opmode == BMP388_PWR_NORMAL) {
		ret = data->reg_write(dev, BMP388_REG_PWR_CTRL,
				      pwr_ctrl | BMP388_PWR_NORMAL);
		if (ret < 0) {
			LOG_ERR("Cannot set Normal Mode");
			return ret;
		}

		switch (bmp388_check_error(dev)) {
		case BMP388_ERR_CONF:
			LOG_ERR("Cannot set Normal Mode, configuration error");
			return -EINVAL;
		case BMP388_ERR_CMD:
			LOG_ERR("Cannot set Normal Mode, command error");
			return -EINVAL;
		case BMP388_ERR_FATAL:
			LOG_ERR("Cannot set Normal Mode, fatal error");
			return -EINVAL;
		default:
			break;
		}
	}

	return 0;
}

static int bmp388_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct bmp388_data *data = dev->driver_data;
	u8_t buf[6];
	int ret;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	/* Set forced mode */
	ret = bmp388_opmode_set(dev, BMP388_PWR_FORCED);
	if (ret < 0) {
		return ret;
	}

	/* Wait for the measurement to complete */
	k_sleep(data->meas_time);

	/* Read the measurements */
	ret = data->reg_read(dev, BMP388_REG_PRESS_DATA_0, buf, 6);
	if (ret < 0) {
		return ret;
	}

#if BMP388_PRESS_EN
	bmp388_compensate_press(data,
				(buf[2] << 16) | (buf[1] << 8) | (buf[0]));
#endif  /* BMP388_PRESS_EN */

#if BMP388_TEMP_EN
	bmp388_compensate_temp(data, (buf[5] << 16) | (buf[4] << 8) | (buf[3]));
#endif  /* BMP388_TEMP_EN */

	return 0;
}

static int bmp388_channel_get(struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct bmp388_data *data = dev->driver_data;
	s32_t tmp;

	switch (chan) {
#if BMP388_PRESS_EN
	case SENSOR_CHAN_PRESS:
		/*
		 * data->comp_press is a float and represents Pa.
		 * Output value of 1029564 Pa represents 102.9564 kPa.
		 */
		tmp = data->comp_press * 10;

		val->val1 = (tmp / 10U) / 1000U;
		val->val2 = (tmp % (10000U)) * 100U;
		break;
#endif          /* BMP388_TEMP_EN */
#if BMP388_TEMP_EN
	case SENSOR_CHAN_AMBIENT_TEMP:
		/*
		 * data->comp_temp is a float and represents oC.
		 * Assign a resolution of 10^(-6) degC.
		 */
		tmp = data->comp_temp * 1000000;
		val->val1 = tmp / 1000000;
		val->val2 = tmp % 1000000;
		break;
#endif          /* BMP388_TEMP_EN */
	default:
		return -EINVAL;
	}

	return 0;
}

static int bmp388_odr_set(struct device *dev, const struct sensor_value *freq)
{
	struct bmp388_data *data = dev->driver_data;
	int ret;

	static const struct sensor_value odr_map[] = {
		{ 200, 0 }, { 100, 0 }, { 50, 0 }, { 25, 0 }, { 25, 2 },
		{ 25, 4 }, { 25, 8 }, { 25, 16 }, { 25, 32 }, { 25, 64 },
		{ 25, 128 }, { 25, 256 }, { 25, 512 }, { 25, 1024 },
		{ 25, 2048 }, { 25, 4096 }, { 25, 8196 }, { 25, 16384 },
	};

	int odr;

	for (odr = 0; odr < ARRAY_SIZE(odr_map); odr++) {
		if (freq->val1 == odr_map[odr].val1 &&
		    freq->val2 == odr_map[odr].val2) {
			break;
		}
	}

	if (odr == ARRAY_SIZE(odr_map)) {
		LOG_DBG("bad frequency");
		return -EINVAL;
	}

	/* Set ODR register */
	ret = data->reg_write(dev, BMP388_REG_ODR, odr);
	if (ret < 0) {
		LOG_ERR("BMP388 cannot set ODR");
		return ret;
	}

	return 0;
}

static int bmp388_osr_set(struct device *dev, const struct sensor_value *osr)
{
	struct bmp388_data *data = dev->driver_data;
	int ret;
	int osr_p, osr_t;
	static const u8_t osr_map[] = { 1, 2, 4, 8, 16, 32 };


	for (osr_p = 0; osr_p < ARRAY_SIZE(osr_map); osr_p++) {
		if ((u8_t)osr->val1 == osr_map[osr_p]) {
			break;
		}
	}

	if (osr_p == ARRAY_SIZE(osr_map)) {
		LOG_DBG("bad oversampling option for pressure");
		return -EINVAL;
	}

	for (osr_t = 0; osr_t < ARRAY_SIZE(osr_map); osr_t++) {
		if ((u8_t)osr->val2 == osr_map[osr_t]) {
			break;
		}
	}

	if (osr_p == ARRAY_SIZE(osr_map)) {
		LOG_DBG("bad oversampling option for temperature");
		return -EINVAL;
	}

	/* Set OSR register */
	ret = data->reg_write(dev, BMP388_REG_OSR, osr_p | (osr_t << 3));
	if (ret < 0) {
		LOG_ERR("BMP388 cannot set OSR");
		return ret;
	}

	return 0;
}

static int bmp388_attr_set(struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr,
			   const struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_ALL) {
		LOG_WRN("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return bmp388_odr_set(dev, val);
	case SENSOR_ATTR_OVERSAMPLING:
		return bmp388_osr_set(dev, val);
	default:
		LOG_DBG("operation not supported.");
		return -ENOTSUP;
	}

	return 0;
}


static const struct sensor_driver_api bmp388_api_funcs = {
	.attr_set = bmp388_attr_set,
	.sample_fetch = bmp388_sample_fetch,
	.channel_get = bmp388_channel_get,
};

/*
 * Read calibration coefficients and convert them to floating point numbers as
 * described in Section 9.1. "Calibration coefficient".
 */
static int bmp388_read_compensation(struct device *dev)
{
	struct bmp388_data *data = dev->driver_data;

	u8_t buf[21];
	int err = 0;

	err = data->reg_read(dev, BMP388_NVM_TRIMMING_COEF,
			     (u8_t *)buf, sizeof(buf));
	if (err < 0) {
		return err;
	}

	data->tr_coef.par_t1 = (u16_t)BMP388_CONCAT_BYTES(buf[1], buf[0]);
	data->conv_coef.par_t1 = (float)data->tr_coef.par_t1 / 0.00390625f;

	data->tr_coef.par_t2 = (u16_t)BMP388_CONCAT_BYTES(buf[3], buf[2]);
	data->conv_coef.par_t2 = (float)data->tr_coef.par_t2 / 1073741824.f;

	data->tr_coef.par_t3 = (s8_t)buf[4];
	data->conv_coef.par_t3 = (float)data->tr_coef.par_t3 /
				 281474976710656.f;

	data->tr_coef.par_p1 = (s16_t)BMP388_CONCAT_BYTES(buf[6], buf[5]);
	data->conv_coef.par_p1 = (float)(data->tr_coef.par_p1 - 16384) /
				 1048576.f;

	data->tr_coef.par_p2 = (s16_t)BMP388_CONCAT_BYTES(buf[8], buf[7]);
	data->conv_coef.par_p2 = (float)(data->tr_coef.par_p2 - 16384) /
				 536870912.f;

	data->tr_coef.par_p3 = (s8_t)buf[9];
	data->conv_coef.par_p3 = (float)data->tr_coef.par_p3 / 4294967296.f;

	data->tr_coef.par_p4 = (s8_t)buf[10];
	data->conv_coef.par_p4 = (float)data->tr_coef.par_p4 / 137438953472.f;

	data->tr_coef.par_p5 = (u16_t)BMP388_CONCAT_BYTES(buf[12], buf[11]);
	data->conv_coef.par_p5 = (float)data->tr_coef.par_p5 / 0.125f;

	data->tr_coef.par_p6 = (u16_t)BMP388_CONCAT_BYTES(buf[14], buf[13]);
	data->conv_coef.par_p6 = (float)data->tr_coef.par_p6 / 64.f;

	data->tr_coef.par_p7 = (s8_t)buf[15];
	data->conv_coef.par_p7 = (float)data->tr_coef.par_p7 / 256.f;

	data->tr_coef.par_p8 = (s8_t)buf[16];
	data->conv_coef.par_p8 = (float)data->tr_coef.par_p8 / 32768.f;

	data->tr_coef.par_p9 = (s16_t)BMP388_CONCAT_BYTES(buf[18], buf[17]);
	data->conv_coef.par_p9 = (float)data->tr_coef.par_p9 /
				 281474976710656.f;

	data->tr_coef.par_p10 = (s8_t)buf[19];
	data->conv_coef.par_p10 = (float)data->tr_coef.par_p10 /
				  281474976710656.f;

	data->tr_coef.par_p11 = (s8_t)buf[20];
	data->conv_coef.par_p11 = (float)data->tr_coef.par_p11 /
				  36893488147419103232.f;

	return 0;
}

static int bmp388_soft_reset(struct device *dev)
{
	struct bmp388_data *data = dev->driver_data;
	int ret;
	u8_t status;

	ret = data->reg_read(dev, BMP388_REG_STATUS, &status, 1);
	if (!(status & BMP388_STATUS_CMD_RDY) || ret < 0) {
		return -EINVAL;
	}

	ret = data->reg_write(dev, BMP388_REG_CMD, BMP388_SOFT_RESET);
	if (ret < 0) {
		return ret;
	}

	k_sleep(2);

	if (bmp388_check_error(dev) == BMP388_ERR_CMD) {
		return -EINVAL;
	}

	return 0;
}

static int bmp388_chip_init(struct device *dev)
{
	struct bmp388_data *data = (struct bmp388_data *) dev->driver_data;
	int ret;

	/* Confirm the correctness of the chip by it's ID */
	ret = data->reg_read(dev, BMP388_REG_CHIP_ID, &data->chip_id, 1);
	if (ret < 0) {
		return ret;
	}

	if (data->chip_id == BMP388_CHIP_ID) {
		LOG_DBG("BMP388 chip detected");
	} else {
		LOG_DBG("bad chip id 0x%x", data->chip_id);
		return -ENOTSUP;
	}

	/* Perform a soft reset */
	ret = bmp388_soft_reset(dev);
	if (ret < 0) {
		LOG_ERR("BMP388 cannot be reseted");
		return ret;
	}

	/* Read trimming coefficients used for conversion of the out data */
	ret = bmp388_read_compensation(dev);
	if (ret < 0) {
		LOG_ERR("BMP388 cannot read calibration data");
		return ret;
	}

	/* Set sensor mode */
	ret = data->reg_write(dev, BMP388_REG_PWR_CTRL,
			      BMP388_PRESS_EN | BMP388_TEMP_EN);
	if (ret < 0) {
		return ret;
	}

	/* Set ODR */
	ret = data->reg_write(dev, BMP388_REG_ODR, BMP388_ODR);
	if (ret < 0) {
		LOG_ERR("BMP388 cannot set ODR");
		return ret;
	}

	/* Set IIR filter */
	ret = data->reg_write(dev, BMP388_REG_CONFIG, BMP388_FILTER);
	if (ret < 0) {
		LOG_ERR("BMP388 cannot set IIR filter");
		return ret;
	}

	/* Set pressure & temperature oversampling */
	ret = data->reg_write(dev, BMP388_REG_OSR,
			      BMP388_PRESS_OVER | BMP388_TEMP_OVER);
	if (ret < 0) {
		LOG_ERR("BMP388 cannot set OSR");
		return ret;
	}

	/* Set forced mode */
	ret = bmp388_opmode_set(dev, BMP388_PWR_FORCED);
	if (ret < 0) {
		LOG_ERR("BMP388 cannot set operating mode");
		return ret;
	}

	data->meas_time = calc_measurement_time();
	LOG_DBG("Measurement time: %d milliseconds", data->meas_time);

	return 0;
}

int bmp388_bus_init(struct device *dev)
{
	const struct bmp388_config *const config = dev->config->config_info;
	struct bmp388_data *data = dev->driver_data;

	data->bus = device_get_binding(config->master_dev_name);
	if (!data->bus) {
		LOG_DBG("i2c master not found: %s",
			DT_INST_0_BOSCH_BMP388_BUS_NAME);
		return -EINVAL;
	}

	config->bus_init(dev);

	return 0;
}

int bmp388_init(struct device *dev)
{
	if (bmp388_bus_init(dev) < 0) {
		return -EINVAL;
	}

	if (bmp388_chip_init(dev) < 0) {
		return -EINVAL;
	}

	return 0;
}

static struct bmp388_data bmp388_data;

static const struct bmp388_config bmp388_config = {
	.master_dev_name = DT_INST_0_BOSCH_BMP388_BUS_NAME,
#if defined(DT_BOSCH_BMP388_BUS_I2C)
	.bus_init = bmp388_i2c_init,
	.i2c_slave_addr = DT_INST_0_BOSCH_BMP388_BASE_ADDRESS,
#else
#error "BUS MACRO NOT DEFINED IN DTS"
#endif
};

DEVICE_AND_API_INIT(bmp388, DT_INST_0_BOSCH_BMP388_LABEL, bmp388_init,
		    &bmp388_data, &bmp388_config, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &bmp388_api_funcs);
