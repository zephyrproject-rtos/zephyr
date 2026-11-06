/*
 * Copyright (c) 2018 Bosch Sensortec GmbH
 * Copyright (c) 2022, Leonard Pollak
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

#include "bme680.h"

LOG_MODULE_REGISTER(bme680, CONFIG_SENSOR_LOG_LEVEL);

#if BME680_BUS_SPI
static inline bool bme680_is_on_spi(const struct device *dev)
{
	const struct bme680_config *config = dev->config;

	return config->bus_io == &bme680_bus_io_spi;
}
#endif

static inline int bme680_bus_check(const struct device *dev)
{
	const struct bme680_config *config = dev->config;

	return config->bus_io->check(&config->bus);
}

static inline int bme680_reg_read(const struct device *dev,
				  uint8_t start, void *buf, int size)
{
	const struct bme680_config *config = dev->config;

	return config->bus_io->read(dev, start, buf, size);
}

static inline int bme680_reg_write(const struct device *dev, uint8_t reg,
				   uint8_t val)
{
	const struct bme680_config *config = dev->config;

	return config->bus_io->write(dev, reg, val);
}

static void bme680_calc_temp(struct bme680_data *data, uint32_t adc_temp)
{
	int64_t var1, var2, var3;

	var1 = ((int32_t)adc_temp >> 3) - ((int32_t)data->par_t1 << 1);
	var2 = (var1 * (int32_t)data->par_t2) >> 11;
	var3 = ((var1 >> 1) * (var1 >> 1)) >> 12;
	var3 = ((var3) * ((int32_t)data->par_t3 << 4)) >> 14;
	data->t_fine = var2 + var3;
	data->calc_temp = ((data->t_fine * 5) + 128) >> 8;
}

static void bme680_calc_press(struct bme680_data *data, uint32_t adc_press)
{
	int32_t var1, var2, var3, calc_press;

	var1 = (((int32_t)data->t_fine) >> 1) - 64000;
	var2 = ((((var1 >> 2) * (var1 >> 2)) >> 11) *
		(int32_t)data->par_p6) >> 2;
	var2 = var2 + ((var1 * (int32_t)data->par_p5) << 1);
	var2 = (var2 >> 2) + ((int32_t)data->par_p4 << 16);
	var1 = (((((var1 >> 2) * (var1 >> 2)) >> 13) *
		 ((int32_t)data->par_p3 << 5)) >> 3)
	       + (((int32_t)data->par_p2 * var1) >> 1);
	var1 = var1 >> 18;
	var1 = ((32768 + var1) * (int32_t)data->par_p1) >> 15;
	calc_press = 1048576 - adc_press;
	calc_press = (calc_press - (var2 >> 12)) * ((uint32_t)3125);
	/* This max value is used to provide precedence to multiplication or
	 * division in the pressure calculation equation to achieve least
	 * loss of precision and avoiding overflows.
	 * i.e Comparing value, signed int 32bit (1 << 30)
	 */
	if (calc_press >= (int32_t)0x40000000) {
		calc_press = ((calc_press / var1) << 1);
	} else {
		calc_press = ((calc_press << 1) / var1);
	}
	var1 = ((int32_t)data->par_p9 *
		(int32_t)(((calc_press >> 3)
			 * (calc_press >> 3)) >> 13)) >> 12;
	var2 = ((int32_t)(calc_press >> 2) * (int32_t)data->par_p8) >> 13;
	var3 = ((int32_t)(calc_press >> 8) * (int32_t)(calc_press >> 8)
		* (int32_t)(calc_press >> 8)
		* (int32_t)data->par_p10) >> 17;

	data->calc_press = calc_press
			   + ((var1 + var2 + var3
			       + ((int32_t)data->par_p7 << 7)) >> 4);
}

static void bme680_calc_humidity(struct bme680_data *data, uint16_t adc_humidity)
{
	int32_t var1, var2_1, var2_2, var2, var3, var4, var5, var6;
	int32_t temp_scaled, calc_hum;

	temp_scaled = (((int32_t)data->t_fine * 5) + 128) >> 8;
	var1 = (int32_t)(adc_humidity - ((int32_t)((int32_t)data->par_h1 * 16))) -
	       (((temp_scaled * (int32_t)data->par_h3)
		 / ((int32_t)100)) >> 1);
	var2_1 = (int32_t)data->par_h2;
	var2_2 = ((temp_scaled * (int32_t)data->par_h4) / (int32_t)100)
		 + (((temp_scaled * ((temp_scaled * (int32_t)data->par_h5)
				     / ((int32_t)100))) >> 6) / ((int32_t)100))
		 +  (int32_t)(1 << 14);
	var2 = (var2_1 * var2_2) >> 10;
	var3 = var1 * var2;
	var4 = (int32_t)data->par_h6 << 7;
	var4 = ((var4) + ((temp_scaled * (int32_t)data->par_h7) /
			  ((int32_t)100))) >> 4;
	var5 = ((var3 >> 14) * (var3 >> 14)) >> 10;
	var6 = (var4 * var5) >> 1;
	calc_hum = (((var3 + var6) >> 10) * ((int32_t)1000)) >> 12;

	if (calc_hum > 100000) { /* Cap at 100%rH */
		calc_hum = 100000;
	} else if (calc_hum < 0) {
		calc_hum = 0;
	}

	data->calc_humidity = calc_hum;
}

static void bme680_calc_gas_resistance(struct bme680_data *data, uint8_t gas_range,
				       uint16_t adc_gas_res)
{
	int64_t var1, var3;
	uint64_t var2;

	static const uint32_t look_up1[16] = { 2147483647, 2147483647, 2147483647,
			       2147483647, 2147483647, 2126008810, 2147483647,
			       2130303777, 2147483647, 2147483647, 2143188679,
			       2136746228, 2147483647, 2126008810, 2147483647,
			       2147483647 };

	static const uint32_t look_up2[16] = { 4096000000, 2048000000, 1024000000,
			       512000000, 255744255, 127110228, 64000000,
			       32258064, 16016016, 8000000, 4000000, 2000000,
			       1000000, 500000, 250000, 125000 };

	var1 = (int64_t)((1340 + (5 * (int64_t)data->range_sw_err)) *
		       ((int64_t)look_up1[gas_range])) >> 16;
	var2 = (((int64_t)((int64_t)adc_gas_res << 15) - (int64_t)(16777216)) + var1);
	var3 = (((int64_t)look_up2[gas_range] * (int64_t)var1) >> 9);
	data->calc_gas_resistance = (uint32_t)((var3 + ((int64_t)var2 >> 1))
					    / (int64_t)var2);
}

static void bme680_calc_gas_resistance_high(struct bme680_data *data, uint8_t gas_range,
					    uint16_t adc_gas_res)
{
	uint32_t den = 2560U + 3U * adc_gas_res;
	uint64_t num = 1000000ULL * (262144U >> gas_range);

	data->calc_gas_resistance = (uint32_t)((num + den / 2U) / den);
}

static void bme690_calc_temp(struct bme680_data *data, uint32_t adc_temp)
{
	const struct bme690_comp_par *par = &data->bme690;
	int64_t cf = (int64_t)adc_temp - ((int64_t)par->t1 << 8);
	int64_t t_lin;

	t_lin = cf * par->t2 * (1LL << 18) + cf * cf * par->t3;
	data->t_fine = (int32_t)(t_lin / (1LL << 32));
	data->calc_temp = (int32_t)(((int64_t)data->t_fine * 100 + (1 << 15)) >> 16);
}

static void bme690_calc_press(struct bme680_data *data, uint32_t adc_press)
{
	const struct bme690_comp_par *par = &data->bme690;
	int64_t t1 = data->t_fine;
	int64_t t2 = (t1 * t1) >> 16;
	int64_t t3 = (t2 * t1) >> 16;
	int64_t ap = adc_press;
	int64_t ap2 = (ap * ap) >> 16;
	int64_t off, sens, quad, cub, press;

	off = (int64_t)par->p1 * (1LL << 34) + (int64_t)par->p2 * t1 * (1LL << 9) +
	      (int64_t)par->p3 * t2 * (1LL << 7) + (int64_t)par->p4 * t3;
	sens = ((int64_t)par->p5 - 16384) * (1LL << 33) +
	       ((int64_t)par->p6 - 16384) * t1 * (1LL << 8) + (int64_t)par->p7 * t2 * (1LL << 5) +
	       (int64_t)par->p8 * t3;
	quad = ap * ((ap * ((int64_t)par->p9 * (1LL << 16) + (int64_t)par->p10 * t1)) >> 32);
	cub = ((ap2 * ap) >> 17) * par->p11;

	press = off * 2 + (sens >> 21) * ap + quad + cub;
	press = (press + (1LL << 31)) >> 32;

	data->calc_press = (uint32_t)CLAMP(press, 0, UINT32_MAX);
}

static void bme690_calc_humidity(struct bme680_data *data, uint16_t adc_humidity)
{
	const struct bme690_comp_par *par = &data->bme690;
	int64_t tc = (((int64_t)data->t_fine * 5) >> 6) - 76800;
	int64_t hoff, fac, hsens, hum;

	hoff = ((int64_t)adc_humidity << 14) - (int64_t)par->h1 * (1LL << 20) -
	       (int64_t)par->h2 * tc;
	fac = (1LL << 26) + (int64_t)par->h4 * tc + (((int64_t)par->h4 * par->h3 * tc * tc) >> 26);
	hsens = (((hoff * par->h5) >> 10) * fac) >> 30;
	hum = hsens - ((((hsens * hsens) >> 16) * par->h6) >> 19);
	hum = (hum * 1000 + (1 << 15)) >> 16;

	data->calc_humidity = (uint32_t)CLAMP(hum, 0, 100000);
}

static uint8_t bme680_calc_res_heat(struct bme680_data *data, uint16_t heatr_temp)
{
	uint8_t heatr_res;
	int32_t var1, var2, var3, var4, var5;
	int32_t heatr_res_x100;
	int32_t amb_temp = 25;    /* Assume ambient temperature to be 25 deg C */

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

static uint8_t bme680_calc_gas_wait(uint16_t dur)
{
	uint8_t factor = 0, durval;

	if (dur >= 0xfc0) {
		durval = 0xff; /* Max duration*/
	} else {
		while (dur > 0x3F) {
			dur = dur / 4;
			factor += 1;
		}
		const uint16_t max_duration = dur + (factor * 64);

		durval = CLAMP(max_duration, 0, 0xff);
	}

	return durval;
}

static int bme680_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct bme_data_regs  data_regs;
	uint8_t status;
	int cnt = 0;
	int ret;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	/* Trigger the measurement */
	ret = bme680_reg_write(dev, BME680_REG_CTRL_MEAS, BME680_CTRL_MEAS_VAL);
	if (ret < 0) {
		return ret;
	}

	do {
		/* Wait for at most the full measurement duration for data.
		 * Initial delay after boot has been measured at 170ms with the
		 * low power heater profile. Subsequent triggers are < 1ms.
		 */
		if (cnt++ > BME680_MEAS_TIMEOUT_MS) {
			return -EAGAIN;
		}
		k_sleep(K_MSEC(1));
		ret = bme680_reg_read(dev, BME680_REG_MEAS_STATUS, &status, 1);
		if (ret < 0) {
			return ret;
		}
	} while (!(status & BME680_MSK_NEW_DATA));
	LOG_DBG("New data after %d ms", cnt);

	ret = bme680_reg_read(dev, BME680_REG_FIELD0, (uint8_t *)&data_regs,
			      sizeof(data_regs));
	if (ret < 0) {
		return ret;
	}

	bme680_compensate_raw(dev, &data_regs, NULL);

	return 0;
}

void bme680_compensate_raw(const struct device *dev,
			   const struct bme_data_regs  *regs,
			   struct bme680_reading *reading)
{
	struct bme680_data *data = dev->data;
	bool is_bme690 = data->variant == BME680_VARIANT_BME690;
	const uint8_t *gas = (data->variant == BME680_VARIANT_GAS_LOW) ? regs->gas : regs->gas_high;
	uint32_t adc_temp, adc_press;
	uint16_t adc_hum, adc_gas_res;
	uint8_t gas_range;

	adc_press = sys_get_be24(regs->pressure);
	adc_temp = sys_get_be24(regs->temperature);
	if (!is_bme690) {
		adc_press >>= 4;
		adc_temp >>= 4;
	}
	adc_hum     = sys_get_be16(regs->humidity);
	adc_gas_res = sys_get_be16(gas) >> 6;
	data->heatr_stab = gas[1] & BME680_MSK_HEATR_STAB;
	data->gas_valid = ((gas[1] & BME680_MSK_GAS_VALID) != 0U) && (data->heatr_stab != 0U);
	gas_range = gas[1] & BME680_MSK_GAS_RANGE;

	LOG_DBG("Raw ADC: temp=%u press=%u hum=%u gas=%u range=%u status=0x%02x", adc_temp,
		adc_press, adc_hum, adc_gas_res, gas_range, gas[1]);

	if (is_bme690) {
		bme690_calc_temp(data, adc_temp);
		bme690_calc_press(data, adc_press);
		bme690_calc_humidity(data, adc_hum);
	} else {
		bme680_calc_temp(data, adc_temp);
		bme680_calc_press(data, adc_press);
		bme680_calc_humidity(data, adc_hum);
	}

	if (data->variant == BME680_VARIANT_GAS_LOW) {
		bme680_calc_gas_resistance(data, gas_range, adc_gas_res);
	} else {
		bme680_calc_gas_resistance_high(data, gas_range, adc_gas_res);
	}

	if (reading != NULL) {

		reading->comp_temp     = data->calc_temp;
		reading->comp_press    = data->calc_press;
		reading->comp_humidity = data->calc_humidity;
		reading->comp_gas      = data->calc_gas_resistance;
		reading->gas_valid = data->gas_valid;
	}
}

static int bme680_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct bme680_data *data = dev->data;

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
		if (!data->gas_valid) {
			return -ENODATA;
		}
		/*
		 * data->calc_gas_resistance has a resolution of 1 ohm.
		 * So 100000 equals 100000 ohms.
		 */
		val->val1 = data->calc_gas_resistance;
		val->val2 = 0;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static void bme680_parse_compensation(struct bme680_data *data, const uint8_t *buff)
{
	/* Temperature related coefficients */
	data->par_t1 = (uint16_t)(BME680_CONCAT_BYTES(buff[32], buff[31]));
	data->par_t2 = (int16_t)(BME680_CONCAT_BYTES(buff[1], buff[0]));
	data->par_t3 = (uint8_t)(buff[2]);

	/* Pressure related coefficients */
	data->par_p1 = (uint16_t)(BME680_CONCAT_BYTES(buff[5], buff[4]));
	data->par_p2 = (int16_t)(BME680_CONCAT_BYTES(buff[7], buff[6]));
	data->par_p3 = (int8_t)buff[8];
	data->par_p4 = (int16_t)(BME680_CONCAT_BYTES(buff[11], buff[10]));
	data->par_p5 = (int16_t)(BME680_CONCAT_BYTES(buff[13], buff[12]));
	data->par_p6 = (int8_t)(buff[15]);
	data->par_p7 = (int8_t)(buff[14]);
	data->par_p8 = (int16_t)(BME680_CONCAT_BYTES(buff[19], buff[18]));
	data->par_p9 = (int16_t)(BME680_CONCAT_BYTES(buff[21], buff[20]));
	data->par_p10 = (uint8_t)(buff[22]);

	/* Humidity related coefficients */
	data->par_h1 = (uint16_t)(((uint16_t)buff[25] << 4) | (buff[24] & 0x0f));
	data->par_h2 = (uint16_t)(((uint16_t)buff[23] << 4) | ((buff[24]) >> 4));
	data->par_h3 = (int8_t)buff[26];
	data->par_h4 = (int8_t)buff[27];
	data->par_h5 = (int8_t)buff[28];
	data->par_h6 = (uint8_t)buff[29];
	data->par_h7 = (int8_t)buff[30];
}

static void bme690_parse_compensation(struct bme690_comp_par *par, const uint8_t *buff)
{
	par->t1 = BME680_CONCAT_BYTES(buff[32], buff[31]);
	par->t2 = BME680_CONCAT_BYTES(buff[1], buff[0]);
	par->t3 = (int8_t)buff[2];

	par->p1 = BME680_CONCAT_BYTES(buff[11], buff[10]);
	par->p2 = BME680_CONCAT_BYTES(buff[13], buff[12]);
	par->p3 = (int8_t)buff[14];
	par->p4 = (int8_t)buff[15];
	par->p5 = (int16_t)BME680_CONCAT_BYTES(buff[5], buff[4]);
	par->p6 = (int16_t)BME680_CONCAT_BYTES(buff[7], buff[6]);
	par->p7 = (int8_t)buff[8];
	par->p8 = (int8_t)buff[9];
	par->p9 = (int16_t)BME680_CONCAT_BYTES(buff[19], buff[18]);
	par->p10 = (int8_t)buff[20];
	par->p11 = (int8_t)buff[21];

	par->h1 = (int16_t)sign_extend(((uint32_t)buff[25] << 4) | (buff[24] & 0x0fU), 11);
	par->h2 = (int8_t)buff[26];
	par->h3 = buff[28];
	par->h4 = (int8_t)buff[27];
	par->h5 = (int16_t)sign_extend(((uint32_t)buff[23] << 4) | (buff[24] >> 4), 11);
	par->h6 = buff[29];
}

static int bme680_read_compensation(const struct device *dev)
{
	struct bme680_data *data = dev->data;
	uint8_t buff[BME680_LEN_COEFF_ALL];
	int err = 0;

	if (data->has_read_compensation) {
		return 0;
	}

	err = bme680_reg_read(dev, BME680_REG_COEFF1, buff, BME680_LEN_COEFF1);
	if (err < 0) {
		return err;
	}

	err = bme680_reg_read(dev, BME680_REG_COEFF2, &buff[BME680_LEN_COEFF1],
			      BME680_LEN_COEFF2);
	if (err < 0) {
		return err;
	}

	err = bme680_reg_read(dev, BME680_REG_COEFF3,
			      &buff[BME680_LEN_COEFF1 + BME680_LEN_COEFF2],
			      BME680_LEN_COEFF3);
	if (err < 0) {
		return err;
	}

	if (data->variant == BME680_VARIANT_BME690) {
		bme690_parse_compensation(&data->bme690, buff);
	} else {
		bme680_parse_compensation(data, buff);
	}

	/* Gas heater related coefficients */
	data->par_gh1 = (int8_t)buff[35];
	data->par_gh2 = (int16_t)(BME680_CONCAT_BYTES(buff[34], buff[33]));
	data->par_gh3 = (int8_t)buff[36];

	data->res_heat_val = (int8_t)buff[37];
	data->res_heat_range = ((buff[39] & BME680_MSK_RH_RANGE) >> 4);
	data->range_sw_err = ((int8_t)(buff[41] & BME680_MSK_RANGE_SW_ERR)) / 16;

	data->has_read_compensation = true;
	return 0;
}

static int bme680_power_up(const struct device *dev)
{
	struct bme680_data *data = dev->data;
	int err;

#if BME680_BUS_SPI
	if (bme680_is_on_spi(dev)) {
		uint8_t mem_page;

		err = bme680_reg_read(dev, BME680_REG_STATUS, &mem_page, 1);
		if (err < 0) {
			return err;
		}

		data->mem_page = (mem_page & BME680_SPI_MEM_PAGE_MSK) >> BME680_SPI_MEM_PAGE_POS;
	}
#endif

	err = bme680_reg_read(dev, BME680_REG_CHIP_ID, &data->chip_id, 1);
	if (err < 0) {
		return err;
	}

	if (data->chip_id == BME680_CHIP_ID) {
		LOG_DBG("BME680 chip detected");
	} else {
		LOG_ERR("Bad BME680 chip id: 0x%x", data->chip_id);
		return -ENOTSUP;
	}

	err = bme680_reg_read(dev, BME680_REG_VARIANT_ID, &data->variant, 1);
	if (err < 0) {
		return err;
	}

	if ((data->variant != BME680_VARIANT_GAS_HIGH) &&
	    (data->variant != BME680_VARIANT_BME690)) {
		data->variant = BME680_VARIANT_GAS_LOW;
	}

	LOG_DBG("Variant id: 0x%x", data->variant);

	err = bme680_read_compensation(dev);
	if (err < 0) {
		return err;
	}

	err = bme680_reg_write(dev, BME680_REG_CTRL_HUM, BME680_HUMIDITY_OVER);
	if (err < 0) {
		return err;
	}

	err = bme680_reg_write(dev, BME680_REG_CONFIG, BME680_CONFIG_VAL);
	if (err < 0) {
		return err;
	}

	err = bme680_reg_write(dev, BME680_REG_CTRL_GAS_1,
			       (data->variant == BME680_VARIANT_GAS_LOW)
				       ? BME680_CTRL_GAS_1_RUN_GAS_LOW
				       : BME680_CTRL_GAS_1_RUN_GAS_HIGH);
	if (err < 0) {
		return err;
	}

	err = bme680_reg_write(dev, BME680_REG_RES_HEAT0,
			       bme680_calc_res_heat(data, BME680_HEATR_TEMP));
	if (err < 0) {
		return err;
	}

	err = bme680_reg_write(dev, BME680_REG_GAS_WAIT0,
			       bme680_calc_gas_wait(BME680_HEATR_DUR_MS));
	if (err < 0) {
		return err;
	}

	return bme680_reg_write(dev, BME680_REG_CTRL_MEAS, BME680_CTRL_MEAS_VAL);
}

static int bme680_pm_control(const struct device *dev, enum pm_device_action action)
{
	int rc = 0;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
	case PM_DEVICE_ACTION_RESUME:
	case PM_DEVICE_ACTION_TURN_OFF:
		break;
	case PM_DEVICE_ACTION_TURN_ON:
		rc = bme680_power_up(dev);
		break;
	default:
		return -ENOTSUP;
	}

	return rc;
}

static int bme680_init(const struct device *dev)
{
#if BME680_BUS_SPI || defined(CONFIG_SENSOR_ASYNC_API)
	struct bme680_data *data = dev->data;
#endif
	int err;

	err = bme680_bus_check(dev);
	if (err < 0) {
		LOG_ERR_DEVICE_NOT_READY(dev);
		return err;
	}

#ifdef CONFIG_SENSOR_ASYNC_API
	data->dev = dev;
	mpsc_init(&data->io_q);
#endif

	return pm_device_driver_init(dev, bme680_pm_control);
}

static DEVICE_API(sensor, bme680_api_funcs) = {
	.sample_fetch = bme680_sample_fetch,
	.channel_get = bme680_channel_get,
#ifdef CONFIG_SENSOR_ASYNC_API
	.submit = bme680_submit,
	.get_decoder = bme680_get_decoder,
#endif
};


#ifdef CONFIG_SENSOR_ASYNC_API
/* Per-instance SPI iodev + RTIO context. */
#define BME680_RTIO_DEFINE_SPI(inst)					       \
	SPI_DT_IODEV_DEFINE(bme680_iodev_##inst, DT_DRV_INST(inst),	       \
			    BME680_SPI_OPERATION);			       \
	RTIO_DEFINE(bme680_rtio_ctx_##inst, 8, 8);

/* Per-instance I2C iodev + RTIO context. */
#define BME680_RTIO_DEFINE_I2C(inst)					       \
	I2C_DT_IODEV_DEFINE(bme680_iodev_##inst, DT_DRV_INST(inst));	       \
	RTIO_DEFINE(bme680_rtio_ctx_##inst, 8, 8);

#define BME680_RTIO_DEFINE(inst)					       \
	COND_CODE_1(DT_INST_ON_BUS(inst, spi),				       \
		    (BME680_RTIO_DEFINE_SPI(inst)),			       \
		    (BME680_RTIO_DEFINE_I2C(inst)))

#define BME680_ASYNC_CONFIG(inst)					       \
	.r	   = &bme680_rtio_ctx_##inst,				       \
	.bus_iodev = &bme680_iodev_##inst,				       \
	.is_spi	   = DT_INST_ON_BUS(inst, spi),
#else
#define BME680_RTIO_DEFINE(inst)
#define BME680_ASYNC_CONFIG(inst)
#endif /* CONFIG_SENSOR_ASYNC_API */

/* Initializes a struct bme680_config for an instance on a SPI bus. */
#define BME680_CONFIG_SPI(inst)						       \
	{								       \
		.bus.spi = SPI_DT_SPEC_INST_GET(			       \
			inst, BME680_SPI_OPERATION),			       \
		.bus_io = &bme680_bus_io_spi,				       \
		BME680_ASYNC_CONFIG(inst)        \
	}

/* Initializes a struct bme680_config for an instance on an I2C bus. */
#define BME680_CONFIG_I2C(inst)						       \
	{								       \
		.bus.i2c = I2C_DT_SPEC_INST_GET(inst),			       \
		.bus_io = &bme680_bus_io_i2c,				       \
		BME680_ASYNC_CONFIG(inst)        \
	}
/*
 * Main instantiation macro, which selects the correct bus-specific
 * instantiation macros for the instance.
 */
#define BME680_DEFINE(inst)						\
	static struct bme680_data bme680_data_##inst;			\
	BME680_RTIO_DEFINE(inst)                    \
	static const struct bme680_config bme680_config_##inst =	\
		COND_CODE_1(DT_INST_ON_BUS(inst, spi),			\
			    (BME680_CONFIG_SPI(inst)),			\
			    (BME680_CONFIG_I2C(inst)));			\
	PM_DEVICE_DT_INST_DEFINE(inst, bme680_pm_control);              \
	SENSOR_DEVICE_DT_INST_DEFINE(inst,				\
			 bme680_init,					\
			 PM_DEVICE_DT_INST_GET(inst),			\
			 &bme680_data_##inst,				\
			 &bme680_config_##inst,				\
			 POST_KERNEL,					\
			 CONFIG_SENSOR_INIT_PRIORITY,			\
			 &bme680_api_funcs);

/* Create the struct device for every status "okay" node in the devicetree. */
DT_INST_FOREACH_STATUS_OKAY(BME680_DEFINE)
