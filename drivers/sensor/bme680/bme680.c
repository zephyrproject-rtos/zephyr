/* bme680.c - Bosch BME680 temperature/humidity/pressure/gas sensor driver */

/*
 * Copyright (c) 2018 Laird
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <sensor.h>
#include <init.h>
#include <gpio.h>
#include <misc/byteorder.h>
#include <misc/__assert.h>

#ifdef CONFIG_BME680_DEV_TYPE_I2C
#include <i2c.h>
#endif
#include <logging/log.h>

#include "bme680.h"

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
LOG_MODULE_REGISTER(BME680);

static int bme680_reg_read(struct BME680_data *data, u8_t start,
			   u8_t *buf, int size)
{
#ifdef CONFIG_BME680_DEV_TYPE_I2C
	return i2c_burst_read(data->i2c_master,
			      data->i2c_slave_addr, start, buf, size);
#endif
	return 0;
}

static int bme680_reg_write(struct BME680_data *data, u8_t reg, u8_t val)
{
#ifdef CONFIG_BME680_DEV_TYPE_I2C
	return i2c_reg_write_byte(data->i2c_master, data->i2c_slave_addr,
				  reg, val);
#endif
	return 0;
}

/*
 * Compensation code taken from Bosch BME680 driver
 * https://github.com/BoschSensortec/BME680_driver/
 *
 * License agreement as follows:
 *
 * Copyright (C) 2017 - 2018 Bosch Sensortec GmbH
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * Neither the name of the copyright holder nor the names of the
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER
 * OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
 *
 * The information provided is believed to be accurate and reliable.
 * The copyright holder assumes no responsibility
 * for the consequences of use
 * of such information nor for any infringement of patents or
 * other rights of third parties which may result from its use.
 * No license is granted by implication or otherwise under any patent or
 * patent rights of the copyright holder.
 *
 */
static void BME680_compensate_temp(struct BME680_data *data, u32_t adc_temp)
{
	s64_t var1, var2, var3;

	var1 = ((s32_t)adc_temp >> 3) - ((s32_t)data->par_t1 << 1);
	var2 = (var1 * (s32_t)data->par_t2) >> 11;
	var3 = ((var1 >> 1) * (var1 >> 1)) >> 12;
	var3 = ((var3) * ((s32_t)data->par_t3 << 4)) >> 14;
	data->t_fine = (s32_t)(var2 + var3);
	data->comp_temp = (s16_t)(((data->t_fine * 5) + 128) >> 8);
}

static void BME680_compensate_press(struct BME680_data *data, u32_t adc_press)
{
	s32_t var1;
	s32_t var2;
	s32_t var3;
	s32_t p;

	var1 = (((s32_t)data->t_fine) >> 1) - 64000;
	var2 = ((((var1 >> 2) * (var1 >> 2)) >> 11)
		* (s32_t)data->par_p6) >> 2;
	var2 = var2 + ((var1 * (s32_t)data->par_p5) << 1);
	var2 = (var2 >> 2) + ((s32_t)data->par_p4 << 16);
	var1 = (((((var1 >> 2) * (var1 >> 2)) >> 13)
		 * ((s32_t)data->par_p3 << 5)) >> 3)
	       + (((s32_t)data->par_p2 * var1) >> 1);
	var1 = var1 >> 18;
	var1 = ((32768 + var1) * (s32_t)data->par_p1) >> 15;
	p = 1048576 - adc_press;
	p = (s32_t)((p - (var2 >> 12)) * ((u32_t)3125));

	if (p >= BME680_MAX_OVERFLOW_VAL) {
		p = ((p / var1) << 1);
	} else if (var1 == 0) {
		data->comp_press = 0;
		return;
	} else {
		p = ((p << 1) / var1);
	}

	var1 = ((s32_t)data->par_p9 * (s32_t)(((p >> 3)
					       * (p >> 3)) >> 13)) >> 12;
	var2 = ((s32_t)(p >> 2) * (s32_t)data->par_p8) >> 13;
	var3 = ((s32_t)(p >> 8) * (s32_t)(p >> 8)
		* (s32_t)(p >> 8) * (s32_t)data->par_p10) >> 17;

	p += ((var1 + var2 + var3 + ((s32_t)data->par_p7 << 7)) >> 4);
	data->comp_press = (u32_t)p;
}

static void BME680_compensate_humidity(struct BME680_data *data,
				       u16_t adc_humidity)
{
	s32_t var1;
	s32_t var2;
	s32_t var3;
	s32_t var4;
	s32_t var5;
	s32_t var6;
	s32_t temp_scaled;

	temp_scaled = (((s32_t)data->t_fine * 5) + 128) >> 8;
	var1 = (s32_t)(adc_humidity - ((s32_t)((s32_t)data->par_h1 * 16)))
	       - (((temp_scaled * (s32_t)data->par_h3) / ((s32_t)100)) >> 1);
	var2 = ((s32_t)data->par_h2 * (((temp_scaled * (s32_t)data->par_h4)
					/ ((s32_t)100)) + (((temp_scaled
					* ((temp_scaled * (s32_t)data->par_h5)
					/ ((s32_t)100))) >> 6) / ((s32_t)100))
					+ (s32_t)(1 << 14))) >> 10;
	var3 = var1 * var2;
	var4 = (s32_t) data->par_h6 << 7;
	var4 = ((var4) + ((temp_scaled * (s32_t)data->par_h7)
			  / ((s32_t)100))) >> 4;
	var5 = ((var3 >> 14) * (var3 >> 14)) >> 10;
	var6 = (var4 * var5) >> 1;
	data->comp_humidity = (((var3 + var6) >> 10) * ((s32_t)1000)) >> 12;

	if (data->comp_humidity > 100000) { /* Cap at 100%rH */
		data->comp_humidity = 100000;
	} else if (data->comp_humidity < 0) {
		data->comp_humidity = 0;
	}
}

#ifdef CONFIG_BME680_ENABLE_GAS_SENSOR
static void BME680_compensate_gas_resist(struct BME680_data *data,
					 u16_t adc_gas, u8_t gas_range)
{
	s64_t var1;
	u64_t var2;
	s64_t var3;

	/* Look up table 1 for the possible gas range values */
	u32_t lookupTable1[16] = { (u32_t)(2147483647), (u32_t)(2147483647),
				   (u32_t)(2147483647), (u32_t)(2147483647),
				   (u32_t)(2147483647), (u32_t)(2126008810),
				   (u32_t)(2147483647), (u32_t)(2130303777),
				   (u32_t)(2147483647), (u32_t)(2147483647),
				   (u32_t)(2143188679), (u32_t)(2136746228),
				   (u32_t)(2147483647), (u32_t)(2126008810),
				   (u32_t)(2147483647), (u32_t)(2147483647) };

	/* Look up table 2 for the possible gas range values */
	u32_t lookupTable2[16] = { (u32_t)(4096000000), (u32_t)(2048000000),
				   (u32_t)(1024000000), (u32_t)(512000000),
				   (u32_t)(255744255), (u32_t)(127110228),
				   (u32_t)(64000000), (u32_t)(32258064),
				   (u32_t)(16016016), (u32_t)(8000000),
				   (u32_t)(4000000), (u32_t)(2000000),
				   (u32_t)(1000000), (u32_t)(500000),
				   (u32_t)(250000), (u32_t)(125000) };

	var1 = (s64_t) ((1340 + (5 * (s64_t) data->range_sw_err)) *
			((s64_t)lookupTable1[gas_range])) >> 16;
	var2 = (((s64_t)((s64_t) adc_gas << 15) - (s64_t)(16777216)) + var1);
	var3 = (((s64_t)lookupTable2[gas_range] * (s64_t)var1) >> 9);
	data->comp_gas = (u32_t)((var3 + ((s64_t)var2 >> 1)) / (s64_t)var2);
}

static u8_t BME680_calc_heater_res(struct BME680_data *data,
				   u16_t adc_temp)
{
	s32_t var1;
	s32_t var2;
	s32_t var3;
	s32_t var4;
	s32_t var5;
	s32_t heatr_res_x100;

	if (adc_temp > 400) { /* Cap temperature */
		adc_temp = 400;
	}

	var1 = (((s32_t)(data->comp_temp / 100) * data->par_gh3) / 1000) * 256;
	var2 = (data->par_gh1 + 784)
	       * (((((data->par_gh2 + 154009) * adc_temp * 5) / 100)
		   + 3276800) / 10);
	var3 = var1 + (var2 / 2);
	var4 = (var3 / (data->res_heat_range + 4));
	var5 = (131 * data->res_heat_val) + 65536;
	heatr_res_x100 = (s32_t)(((var4 / var5) - 250) * 34);
	return (u8_t)((heatr_res_x100 + 50) / 100);
}

static u8_t BME680_calc_heater_dur(u16_t dur)
{
	u8_t factor = 0;
	u8_t durval;

	if (dur >= 0xfc0) {
		durval = 0xff; /* Max duration*/
	} else {
		while (dur > 0x3F) {
			dur = dur / 4;
			factor += 1;
		}
		durval = (u8_t) (dur + (factor * 64));
	}

	return durval;
}
#endif

static int BME680_set_mode(struct BME680_data *data)
{
	int err;
	u8_t tmp;
	u8_t pow;
	u8_t timeout = 0;

	do {
		err = bme680_reg_read(data, BME680_CONF_T_P_MODE_ADDR,
				      &tmp, 1);
		if (err < 0) {
			return err;
		}
		pow = (tmp & BME680_MODE_MSK);

		if (pow != BME680_SLEEP_MODE) {
			tmp = tmp & (~BME680_MODE_MSK); /* Set to sleep */
			err = bme680_reg_write(data,
					       BME680_CONF_T_P_MODE_ADDR, tmp);
			k_sleep(BME680_POLL_PERIOD_MS);

			if (timeout > BME680_POLL_TIMEOUT_CHECK) {
				//Communication has timed out
				return BME680_TIMEOUT_ERROR_CODE;
			}
			++timeout;
		}
	} while (pow != BME680_SLEEP_MODE);

	if (data->power_mode != BME680_SLEEP_MODE) {
		tmp = (tmp & ~BME680_MODE_MSK)
		      | (data->power_mode & BME680_MODE_MSK);
		err = bme680_reg_write(data, BME680_CONF_T_P_MODE_ADDR, tmp);
	}

	return 0;
}

static int BME680_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct BME680_data *data = dev->driver_data;
	u8_t buf[BME680_FIELD_LENGTH];
	u32_t adc_temp;
	u32_t adc_press;
	u16_t adc_humidity;
	int ret;
	u8_t timeout = 0;

#ifdef CONFIG_BME680_ENABLE_GAS_SENSOR
	u8_t gas_range;
	u16_t adc_gas_res;
#endif

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);
	ret = BME680_set_mode(dev->driver_data);
	if (ret < 0) {
		return ret;
	}
	buf[0] = 0;
	while (1) {
		k_sleep(BME680_POLL_PERIOD_MS);
		ret = bme680_reg_read(data, BME680_FIELD0_ADDR,
				      buf, BME680_FIELD_LENGTH);
		if (ret < 0) {
			return ret;
		}

		if (buf[0] & BME680_NEW_DATA_MSK) {
			adc_press = (u32_t)(((u32_t)buf[2] << 12)
					    | ((u32_t)buf[3] << 4)
					    | ((u32_t)buf[4] >> 4));
			adc_temp = (u32_t)(((u32_t)buf[5] << 12)
					   | ((u32_t)buf[6] << 4)
					   | ((u32_t)buf[7] >> 4));
			adc_humidity = (u16_t)(((u32_t)buf[8] << 8)
					       | (u32_t)buf[9]);
			BME680_compensate_temp(data, adc_temp);
			BME680_compensate_press(data, adc_press);
			BME680_compensate_humidity(data, adc_humidity);

#ifdef CONFIG_BME680_ENABLE_GAS_SENSOR
			adc_gas_res = (u16_t)((u32_t)buf[13] * 4
					      | (((u32_t)buf[14]) / 64));
			gas_range = (buf[14] & BME680_GAS_RANGE_MSK);
			ret = bme680_reg_read(data, BME680_GAS_R_LSB, buf, 1);
			if (ret < 0) {
				return ret;
			}

			if ((buf[0] & BME680_GAS_VALID_MASK)) {
				BME680_compensate_gas_resist(data,
							     adc_gas_res,
							     gas_range);
			}
#endif
			break;
		}
		else {
			if (timeout > BME680_POLL_TIMEOUT_CHECK) {
				//Communication has timed out
				return BME680_TIMEOUT_ERROR_CODE;
			}
			++timeout;
		}
	}

	return 0;
}

static int BME680_channel_get(struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct BME680_data *data = dev->driver_data;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		/*
		 * data->comp_temp has a resolution of 0.01 degC.  So
		 * 5123 equals 51.23 degC.
		 */
		val->val1 = data->comp_temp / 100;
		val->val2 = data->comp_temp % 100;
		break;
	case SENSOR_CHAN_PRESS:
		/*
		 * data->comp_press is in pascals. Convert to KPa
		 * by dividing by 1000.  E.g. 100057 Pa = 100.057 KPa
		 */
		val->val1 = data->comp_press / 1000;
		val->val2 = data->comp_press % 1000;
		break;
	case SENSOR_CHAN_HUMIDITY:
		/*
		 * data->comp_humidity is in percent
		 */
		val->val1 = data->comp_humidity / 1000;
		val->val2 = data->comp_humidity % 1000;
		break;
#ifdef CONFIG_BME680_ENABLE_GAS_SENSOR
	case SENSOR_CHAN_GAS:
		/*
		 * data->comp_gas is in ohms
		 */
		val->val1 = data->comp_gas;
		val->val2 = 0;
		break;
#endif
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct sensor_driver_api BME680_api_funcs = {
	.sample_fetch = BME680_sample_fetch,
	.channel_get = BME680_channel_get,
};

static int BME680_read_compensation(struct BME680_data *data)
{
	u8_t buf[BME680_COEFF_SIZE];
	int err = 0;

	err = bme680_reg_read(data, BME680_COEFF_ADDR1,
			      (u8_t *)buf, BME680_COEFF_ADDR1_LEN);

	if (err < 0) {
		return err;
	}

	err = bme680_reg_read(data, BME680_COEFF_ADDR2,
			      (u8_t *)&buf[BME680_COEFF_ADDR1_LEN],
			      BME680_COEFF_ADDR2_LEN);

	if (err < 0) {
		return err;
	}

	data->par_h1 = sys_le16_to_cpu((u16_t)buf[BME680_H1_LSB_REG]
				       | ((u16_t)buf[BME680_H1_MSB_REG]
					  << BME680_HUM_REG_SHIFT_VAL));
	data->par_h2 = sys_le16_to_cpu((u16_t)buf[BME680_H2_LSB_REG]
				       | ((u16_t)buf[BME680_H2_MSB_REG]
					  << BME680_HUM_REG_SHIFT_VAL));
	data->par_h3 = buf[BME680_H3_REG];
	data->par_h4 = buf[BME680_H4_REG];
	data->par_h5 = buf[BME680_H5_REG];
	data->par_h6 = buf[BME680_H6_REG];
	data->par_h7 = buf[BME680_H7_REG];
	data->par_gh1 = buf[BME680_GH1_REG];
	data->par_gh2 = sys_le16_to_cpu((u16_t)buf[BME680_GH2_LSB_REG]
					| ((u16_t)buf[BME680_GH2_MSB_REG]
					   << 8));
	data->par_gh3 = buf[BME680_GH3_REG];
	data->par_t1 = sys_le16_to_cpu((u16_t)buf[BME680_T1_LSB_REG]
				       | ((u16_t)buf[BME680_T1_MSB_REG]
					  << 8));
	data->par_t2 = sys_le16_to_cpu((u16_t)buf[BME680_T2_LSB_REG]
				       | ((u16_t)buf[BME680_T2_MSB_REG]
					  << 8));
	data->par_t3 = buf[BME680_T3_REG];
	data->par_p1 = sys_le16_to_cpu((u16_t)buf[BME680_P1_LSB_REG]
				       | ((u16_t)buf[BME680_P1_MSB_REG]
					  << 8));
	data->par_p2 = sys_le16_to_cpu((u16_t)buf[BME680_P2_LSB_REG]
				       | ((u16_t)buf[BME680_P2_MSB_REG]
					  << 8));
	data->par_p3 = buf[BME680_P3_REG];
	data->par_p4 = sys_le16_to_cpu((u16_t)buf[BME680_P4_LSB_REG]
				       | ((u16_t)buf[BME680_P4_MSB_REG]
					  << 8));
	data->par_p5 = sys_le16_to_cpu((u16_t)buf[BME680_P5_LSB_REG]
				       | ((u16_t)buf[BME680_P5_MSB_REG]
					  << 8));
	data->par_p6 = buf[BME680_P6_REG];
	data->par_p7 = buf[BME680_P7_REG];
	data->par_p8 = sys_le16_to_cpu((u16_t)buf[BME680_P8_LSB_REG]
				       | ((u16_t)buf[BME680_P8_MSB_REG]
					  << 8));
	data->par_p9 = sys_le16_to_cpu((u16_t)buf[BME680_P9_LSB_REG]
				       | ((u16_t)buf[BME680_P9_MSB_REG]
					  << 8));
	data->par_p10 = buf[BME680_P10_REG];

	err = bme680_reg_read(data, BME680_ADDR_RES_HEAT_RANGE_ADDR,
			      (u8_t *)buf, 1);

	if (err < 0) {
		return err;
	}

	data->res_heat_range = ((buf[0] & BME680_RHRANGE_MSK) / 16);

	err = bme680_reg_read(data, BME680_ADDR_RES_HEAT_VAL_ADDR,
			      (u8_t *)buf, 1);

	if (err < 0) {
		return err;
	}

	data->res_heat_val = buf[0];

	err = bme680_reg_read(data, BME680_ADDR_RANGE_SW_ERR_ADDR,
			      (u8_t *)buf, 1);

	if (err < 0) {
		return err;
	}

	data->range_sw_err = ((s8_t)buf[0] & (s8_t)BME680_RSERROR_MSK) / 16;

	return 0;
}

static void BME680_soft_reset(struct BME680_data *data)
{
	bme680_reg_write(data, BME680_SOFT_RESET_ADDR, BME680_SOFT_RESET_CMD);
	k_sleep(BME680_RESET_PERIOD);
}

#ifdef CONFIG_BME680_ENABLE_GAS_SENSOR
static s8_t set_gas_config(struct BME680_data *data)
{
	s8_t err;

	err = bme680_reg_write(data, BME680_RES_HEAT0_ADDR,
			       BME680_calc_heater_res(data, data->heatr_temp));
	if (err < 0) {
		return err;
	}

	err = bme680_reg_write(data, BME680_GAS_WAIT0_ADDR,
			       BME680_calc_heater_dur(data->heatr_dur));
	if (err < 0) {
		return err;
	}

	data->nb_conv = 0;

	return err;
}
#endif

static int BME680_chip_init(struct device *dev)
{
	struct BME680_data *data = (struct BME680_data *) dev->driver_data;
	int err;

	/* Soft reset before initialisation */
	BME680_soft_reset(data);

	err = bme680_reg_read(data, BME680_REG_ID, &data->chip_id, 1);
	if (err < 0) {
		return err;
	}

	if (data->chip_id == BME680_CHIP_ID) {
		LOG_DBG("BME680 chip detected");
	} else {
		LOG_DBG("bad chip id 0x%x", data->chip_id);
		return -ENOTSUP;
	}

	err = BME680_read_compensation(data);
	if (err < 0) {
		return err;
	}

	/* Put sensor into sleep mode */
	data->power_mode = BME680_SLEEP_MODE;
	err = BME680_set_mode(data);
	if (err < 0) {
		return err;
	}

	/* Configure sensor settings */
	err = bme680_reg_write(data, BME680_CONF_T_P_MODE_ADDR,
			       (BME680_TEMP_OVER | BME680_PRESS_OVER));
	err = bme680_reg_write(data, BME680_CONF_OS_H_ADDR,
			       BME680_HUMIDITY_OVER);
	err = bme680_reg_write(data, BME680_CONF_ODR_FILT_ADDR,
			       BME680_FILTER);

#ifdef CONFIG_BME680_ENABLE_GAS_SENSOR
	/* Configure gas sensor settings, a temperature reading is needed */
	data->power_mode = BME680_FORCED_MODE;
	err = BME680_set_mode(data);
	if (err < 0) {
		return err;
	}

	u32_t adc_temp;
	u8_t buf[8] = { 0 };
	u8_t timeout = 0;

	while (1) {
		/* Retrieve ambient sensor temperature */
		k_sleep(BME680_POLL_PERIOD_MS);

		err = bme680_reg_read(data, BME680_FIELD0_ADDR,
				      buf, sizeof(buf));
		if (err < 0) {
			return err;
		}

		if (buf[0] & BME680_NEW_DATA_MSK) {
			data->power_mode = BME680_SLEEP_MODE;
			err = BME680_set_mode(data);
			if (err < 0) {
				return err;
			}

			adc_temp = (u32_t)(((u32_t)buf[5] << 12)
					   | ((u32_t)buf[6] << 4)
					   | ((u32_t)buf[7] >> 4));
			BME680_compensate_temp(data, adc_temp);

			/* Load the heater configuration to the sensor */
#ifdef CONFIG_BME680_ENABLE_GAS_SENSOR_HEATER
			data->heatr_temp = CONFIG_BME680_GAS_HEATER_TEMPERATUE;
			data->heatr_dur = CONFIG_BME680_GAS_HEATER_DURATION;
#else
			data->heatr_temp = 0;
			data->heatr_dur = 0;
#endif

			err = set_gas_config(data);

			err = bme680_reg_read(data,
					      BME680_CONF_HEAT_CTRL_ADDR, buf, 1);
			if (err < 0) {
				return err;
			}
#ifdef CONFIG_BME680_ENABLE_GAS_SENSOR_HEATER
			buf[0] &= (0xff ^ BME680_DISABLE_HEATER);
#else
			buf[0] |= BME680_DISABLE_HEATER;
#endif
			err = bme680_reg_write(data,
					       BME680_CONF_HEAT_CTRL_ADDR, buf[0]);

			/* Enable the gas sensor */
			err = bme680_reg_write(data,
					       BME680_CONF_ODR_RUN_GAS_NBC_ADDR,
					       BME680_RUN_GAS_MSK);
			break;
		}
		else {
			if (timeout > BME680_POLL_TIMEOUT_CHECK) {
				//Communication has timed out
				return BME680_TIMEOUT_ERROR_CODE;
			}
			++timeout;
		}
	}
#endif

	/* Use sensor in forced mode */
	data->power_mode = BME680_FORCED_MODE;

	return 0;
}

int BME680_init(struct device *dev)
{
	struct BME680_data *data = dev->driver_data;

#ifdef CONFIG_BME680_DEV_TYPE_I2C
	data->i2c_master = device_get_binding(
		CONFIG_BME680_I2C_MASTER_DEV_NAME);
	if (!data->i2c_master) {
		LOG_DBG("i2c master not found: %s",
			CONFIG_BME680_I2C_MASTER_DEV_NAME);
		return -EINVAL;
	}

	data->i2c_slave_addr = BME680_I2C_ADDR;
#endif

	return BME680_chip_init(dev);
}

static struct BME680_data BME680_data;

DEVICE_AND_API_INIT(BME680, CONFIG_BME680_DEV_NAME, BME680_init, &BME680_data,
		    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &BME680_api_funcs);
