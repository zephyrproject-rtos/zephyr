/* bme680.c - Driver for Bosch Sensortec's BME680 temperature, pressure,
 * humidity and gas sensor
 *
 * https://www.bosch-sensortec.com/bst/products/all_products/bme680
 */

/*
 * Copyright (c) 2018 Bosch Sensortec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bme680.h"
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <init.h>
#include <kernel.h>
#include <sys/byteorder.h>
#include <sys/__assert.h>
#include <drivers/sensor.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(bme680, CONFIG_SENSOR_LOG_LEVEL);

static int bme680_reg_read(struct bme680_data *data, u8_t start, u8_t *buf,
			   int size)
{
	return i2c_burst_read(data->i2c_master, data->i2c_slave_addr, start,
			      buf, size);
	return 0;
}

static int bme680_reg_write(struct bme680_data *data, u8_t reg, u8_t val)
{
	return i2c_reg_write_byte(data->i2c_master, data->i2c_slave_addr,
				  reg, val);
	return 0;
}

static void bme680_calc_temp(struct bme680_data *data, u32_t adc_temp)
{
	s64_t var1, var2, var3;

	var1 = ((s32_t)adc_temp >> 3) - ((s32_t)data->par_t1 << 1);
	var2 = (var1 * (s32_t)data->par_t2) >> 11;
	var3 = ((var1 >> 1) * (var1 >> 1)) >> 12;
	var3 = ((var3) * ((s32_t)data->par_t3 << 4)) >> 14;
	data->t_fine = var2 + var3;
	data->calc_temp = ((data->t_fine * 5) + 128) >> 8;
}

static void bme680_calc_press(struct bme680_data *data, u32_t adc_press)
{
	s32_t var1, var2, var3, calc_press;

	var1 = (((s32_t)data->t_fine) >> 1) - 64000;
	var2 = ((((var1 >> 2) * (var1 >> 2)) >> 11) *
		(s32_t)data->par_p6) >> 2;
	var2 = var2 + ((var1 * (s32_t)data->par_p5) << 1);
	var2 = (var2 >> 2) + ((s32_t)data->par_p4 << 16);
	var1 = (((((var1 >> 2) * (var1 >> 2)) >> 13) *
		 ((s32_t)data->par_p3 << 5)) >> 3)
	       + (((s32_t)data->par_p2 * var1) >> 1);
	var1 = var1 >> 18;
	var1 = ((32768 + var1) * (s32_t)data->par_p1) >> 15;
	calc_press = 1048576 - adc_press;
	calc_press = (calc_press - (var2 >> 12)) * ((u32_t)3125);
	/* This max value is used to provide precedence to multiplication or
	 * division in the pressure calculation equation to achieve least
	 * loss of precision and avoiding overflows.
	 * i.e Comparing value, signed int 32bit (1 << 30)
	 */
	if (calc_press >= (s32_t)0x40000000) {
		calc_press = ((calc_press / var1) << 1);
	} else {
		calc_press = ((calc_press << 1) / var1);
	}
	var1 = ((s32_t)data->par_p9 *
		(s32_t)(((calc_press >> 3)
			 * (calc_press >> 3)) >> 13)) >> 12;
	var2 = ((s32_t)(calc_press >> 2) * (s32_t)data->par_p8) >> 13;
	var3 = ((s32_t)(calc_press >> 8) * (s32_t)(calc_press >> 8)
		* (s32_t)(calc_press >> 8)
		* (s32_t)data->par_p10) >> 17;

	data->calc_press = calc_press
			   + ((var1 + var2 + var3
			       + ((s32_t)data->par_p7 << 7)) >> 4);
}

static void bme680_calc_humidity(struct bme680_data *data, u16_t adc_humidity)
{
	s32_t var1, var2_1, var2_2, var2, var3, var4, var5, var6;
	s32_t temp_scaled, calc_hum;

	temp_scaled = (((s32_t)data->t_fine * 5) + 128) >> 8;
	var1 = (s32_t)(adc_humidity - ((s32_t)((s32_t)data->par_h1 * 16))) -
	       (((temp_scaled * (s32_t)data->par_h3)
		 / ((s32_t)100)) >> 1);
	var2_1 = (s32_t)data->par_h2;
	var2_2 = ((temp_scaled * (s32_t)data->par_h4) / (s32_t)100)
		 + (((temp_scaled * ((temp_scaled * (s32_t)data->par_h5)
				     / ((s32_t)100))) >> 6) / ((s32_t)100))
		 +  (s32_t)(1 << 14);
	var2 = (var2_1 * var2_2) >> 10;
	var3 = var1 * var2;
	var4 = (s32_t)data->par_h6 << 7;
	var4 = ((var4) + ((temp_scaled * (s32_t)data->par_h7) /
			  ((s32_t)100))) >> 4;
	var5 = ((var3 >> 14) * (var3 >> 14)) >> 10;
	var6 = (var4 * var5) >> 1;
	calc_hum = (((var3 + var6) >> 10) * ((s32_t)1000)) >> 12;

	if (calc_hum > 100000) { /* Cap at 100%rH */
		calc_hum = 100000;
	} else if (calc_hum < 0) {
		calc_hum = 0;
	}

	data->calc_humidity = calc_hum;
}

static void bme680_calc_gas_resistance(struct bme680_data *data, u8_t gas_range,
				       u16_t adc_gas_res)
{
	s64_t var1, var3;
	u64_t var2;

	static const u32_t look_up1[16] = { 2147483647, 2147483647, 2147483647,
			       2147483647, 2147483647, 2126008810, 2147483647,
			       2130303777, 2147483647, 2147483647, 2143188679,
			       2136746228, 2147483647, 2126008810, 2147483647,
			       2147483647 };

	static const u32_t look_up2[16] = { 4096000000, 2048000000, 1024000000,
			       512000000, 255744255, 127110228, 64000000,
			       32258064, 16016016, 8000000, 4000000, 2000000,
			       1000000, 500000, 250000, 125000 };

	var1 = (s64_t)((1340 + (5 * (s64_t)data->range_sw_err)) *
		       ((s64_t)look_up1[gas_range])) >> 16;
	var2 = (((s64_t)((s64_t)adc_gas_res << 15) - (s64_t)(16777216)) + var1);
	var3 = (((s64_t)look_up2[gas_range] * (s64_t)var1) >> 9);
	data->calc_gas_resistance = (u32_t)((var3 + ((s64_t)var2 >> 1))
					    / (s64_t)var2);
}

static u8_t bme680_calc_res_heat(struct bme680_data *data, u16_t heatr_temp)
{
	u8_t heatr_res;
	s32_t var1, var2, var3, var4, var5;
	s32_t heatr_res_x100;
	s32_t amb_temp = 25;    /* Assume ambient temperature to be 25 deg C */

	if (heatr_temp > 400) { /* Cap temperature */
		heatr_temp = 400;
	}

	var1 = ((amb_temp * data->par_gh3) / 1000) * 256;
	var2 = (data->par_gh1 + 784) * (((((data->par_gh2 + 154009)
					   * heatr_temp * 5) / 100)
					 + 3276800) / 10);
	var3 = var1 + (var2 / 2);
	var4 = (var3 / (data->res_heat_range + 4));
	var5 = (131 * data->res_heat_val) + 65536;
	heatr_res_x100 = ((var4 / var5) - 250) * 34;
	heatr_res = (heatr_res_x100 + 50) / 100;

	return heatr_res;
}

static u8_t bme680_calc_gas_wait(u16_t dur)
{
	u8_t factor = 0, durval;

	if (dur >= 0xfc0) {
		durval = 0xff; /* Max duration*/
	} else {
		while (dur > 0x3F) {
			dur = dur / 4;
			factor += 1;
		}
		durval = dur + (factor * 64);
	}

	return durval;
}

static int bme680_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct bme680_data *data = dev->driver_data;
	u8_t buff[BME680_LEN_FIELD] = { 0 };
	u8_t gas_range;
	u32_t adc_temp, adc_press;
	u16_t adc_hum, adc_gas_res;
	int size = BME680_LEN_FIELD;
	int ret;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	ret = bme680_reg_read(data, BME680_REG_FIELD0, buff, size);
	if (ret < 0) {
		return ret;
	}

	data->new_data = buff[0] & BME680_MSK_NEW_DATA;
	data->heatr_stab = buff[14] & BME680_MSK_HEATR_STAB;

	adc_press = (u32_t)(((u32_t)buff[2] << 12) | ((u32_t)buff[3] << 4)
			    | ((u32_t)buff[4] >> 4));
	adc_temp = (u32_t)(((u32_t)buff[5] << 12) | ((u32_t)buff[6] << 4)
			   | ((u32_t)buff[7] >> 4));
	adc_hum = (u16_t)(((u32_t)buff[8] << 8) | (u32_t)buff[9]);
	adc_gas_res = (u16_t)((u32_t)buff[13] << 2 | (((u32_t)buff[14]) >> 6));
	gas_range = buff[14] & BME680_MSK_GAS_RANGE;

	if (data->new_data) {
		bme680_calc_temp(data, adc_temp);
		bme680_calc_press(data, adc_press);
		bme680_calc_humidity(data, adc_hum);
		bme680_calc_gas_resistance(data, gas_range, adc_gas_res);
	}

	/* Trigger the next measurement */
	ret = bme680_reg_write(data, BME680_REG_CTRL_MEAS,
			       BME680_CTRL_MEAS_VAL);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int bme680_channel_get(struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct bme680_data *data = dev->driver_data;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		/*
		 * data->calc_temp has a resolution of 0.01 degC.
		 * So 5123 equals 51.23 degC.
		 */
		val->val1 = data->calc_temp / 100;
		val->val2 = data->calc_temp % 100 * 10000;
		break;
	case SENSOR_CHAN_PRESS:
		/*
		 * data->calc_press has a resolution of 1 Pa.
		 * So 96321 equals 96.321 kPa.
		 */
		val->val1 = data->calc_press / 1000;
		val->val2 = (data->calc_press % 1000) * 1000;
		break;
	case SENSOR_CHAN_HUMIDITY:
		/*
		 * data->calc_humidity has a resolution of 0.001 %RH.
		 * So 46333 equals 46.333 %RH.
		 */
		val->val1 = data->calc_humidity / 1000;
		val->val2 = (data->calc_humidity % 1000) * 1000;
		break;
	case SENSOR_CHAN_GAS_RES:
		/*
		 * data->calc_gas_resistance has a resolution of 1 ohm.
		 * So 100000 equals 100000 ohms.
		 */
		val->val1 = data->calc_gas_resistance;
		val->val2 = 0;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int bme680_read_compensation(struct bme680_data *data)
{
	u8_t buff[BME680_LEN_COEFF_ALL];
	int err = 0;

	err = bme680_reg_read(data, BME680_REG_COEFF1, buff, BME680_LEN_COEFF1);
	if (err < 0) {
		return err;
	}

	err = bme680_reg_read(data, BME680_REG_COEFF2, &buff[BME680_LEN_COEFF1],
			      16);
	if (err < 0) {
		return err;
	}

	err = bme680_reg_read(data, BME680_REG_COEFF3,
			      &buff[BME680_LEN_COEFF1 + BME680_LEN_COEFF2],
			      BME680_LEN_COEFF3);
	if (err < 0) {
		return err;
	}

	/* Temperature related coefficients */
	data->par_t1 = (u16_t)(BME680_CONCAT_BYTES(buff[32], buff[31]));
	data->par_t2 = (s16_t)(BME680_CONCAT_BYTES(buff[1], buff[0]));
	data->par_t3 = (u8_t)(buff[2]);

	/* Pressure related coefficients */
	data->par_p1 = (u16_t)(BME680_CONCAT_BYTES(buff[5], buff[4]));
	data->par_p2 = (s16_t)(BME680_CONCAT_BYTES(buff[7], buff[6]));
	data->par_p3 = (s8_t)buff[8];
	data->par_p4 = (s16_t)(BME680_CONCAT_BYTES(buff[11], buff[10]));
	data->par_p5 = (s16_t)(BME680_CONCAT_BYTES(buff[13], buff[12]));
	data->par_p6 = (s8_t)(buff[15]);
	data->par_p7 = (s8_t)(buff[14]);
	data->par_p8 = (s16_t)(BME680_CONCAT_BYTES(buff[19], buff[18]));
	data->par_p9 = (s16_t)(BME680_CONCAT_BYTES(buff[21], buff[20]));
	data->par_p10 = (u8_t)(buff[22]);

	/* Humidity related coefficients */
	data->par_h1 = (u16_t)(((u16_t)buff[25] << 4) | (buff[24] & 0x0f));
	data->par_h2 = (u16_t)(((u16_t)buff[23] << 4) | ((buff[24]) >> 4));
	data->par_h3 = (s8_t)buff[26];
	data->par_h4 = (s8_t)buff[27];
	data->par_h5 = (s8_t)buff[28];
	data->par_h6 = (u8_t)buff[29];
	data->par_h7 = (s8_t)buff[30];

	/* Gas heater related coefficients */
	data->par_gh1 = (s8_t)buff[35];
	data->par_gh2 = (s16_t)(BME680_CONCAT_BYTES(buff[34], buff[33]));
	data->par_gh3 = (s8_t)buff[36];

	data->res_heat_val = (s8_t)buff[37];
	data->res_heat_range = ((buff[39] & BME680_MSK_RH_RANGE) >> 4);
	data->range_sw_err = ((s8_t)(buff[41] & BME680_MSK_RANGE_SW_ERR)) / 16;

	return 0;
}

static int bme680_chip_init(struct device *dev)
{
	struct bme680_data *data = (struct bme680_data *)dev->driver_data;
	int err;

	err = bme680_reg_read(data, BME680_REG_CHIP_ID, &data->chip_id, 1);
	if (err < 0) {
		return err;
	}

	if (data->chip_id == BME680_CHIP_ID) {
		LOG_ERR("BME680 chip detected");
	} else {
		LOG_ERR("Bad BME680 chip id 0x%x", data->chip_id);
		return -ENOTSUP;
	}

	err = bme680_read_compensation(data);
	if (err < 0) {
		return err;
	}

	err = bme680_reg_write(data, BME680_REG_CTRL_HUM, BME680_HUMIDITY_OVER);
	if (err < 0) {
		return err;
	}

	err = bme680_reg_write(data, BME680_REG_CONFIG, BME680_CONFIG_VAL);
	if (err < 0) {
		return err;
	}

	err = bme680_reg_write(data, BME680_REG_CTRL_GAS_1,
			       BME680_CTRL_GAS_1_VAL);
	if (err < 0) {
		return err;
	}

	err = bme680_reg_write(data, BME680_REG_RES_HEAT0,
			       bme680_calc_res_heat(data, BME680_HEATR_TEMP));
	if (err < 0) {
		return err;
	}

	err = bme680_reg_write(data, BME680_REG_GAS_WAIT0,
			       bme680_calc_gas_wait(BME680_HEATR_DUR_MS));
	if (err < 0) {
		return err;
	}

	err = bme680_reg_write(data, BME680_REG_CTRL_MEAS,
			       BME680_CTRL_MEAS_VAL);
	if (err < 0) {
		return err;
	}

	return 0;
}

static int bme680_init(struct device *dev)
{
	struct bme680_data *data = dev->driver_data;

	data->i2c_master = device_get_binding(
		DT_INST_0_BOSCH_BME680_BUS_NAME);
	if (!data->i2c_master) {
		LOG_ERR("I2C master not found: %s",
			    DT_INST_0_BOSCH_BME680_BUS_NAME);
		return -EINVAL;
	}

	data->i2c_slave_addr = DT_INST_0_BOSCH_BME680_BASE_ADDRESS;

	if (bme680_chip_init(dev) < 0) {
		return -EINVAL;
	}

	return 0;
}

static const struct sensor_driver_api bme680_api_funcs = {
	.sample_fetch = bme680_sample_fetch,
	.channel_get = bme680_channel_get,
};

static struct bme680_data bme680_data;

DEVICE_AND_API_INIT(bme680, DT_INST_0_BOSCH_BME680_LABEL, bme680_init, &bme680_data,
		    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &bme680_api_funcs);
