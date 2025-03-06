/* bme680.c - Driver for Bosch Sensortec's BME680 temperature, pressure,
 * humidity and gas sensor
 *
 * https://www.bosch-sensortec.com/bst/products/all_products/bme680
 */

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

struct bme_data_regs {
	uint8_t pressure[3];
	uint8_t temperature[3];
	uint8_t humidity[2];
	uint8_t padding[3];
	uint8_t gas[2];
} __packed;

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

static int bme680_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct bme680_data *data = dev->data;
	struct bme_data_regs data_regs;
	uint8_t gas_range;
	uint32_t adc_temp, adc_press;
	uint16_t adc_hum, adc_gas_res;
	uint8_t status;
	int cnt = 0;
	int ret;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	/* Trigger the measurement */
	ret = bme680_reg_write(dev, BME680_REG_CTRL_MEAS, BME680_CTRL_MEAS_VAL);
	if (ret < 0) {
		return ret;
	}

	do {
		/* Wait for a maximum of 250ms for data.
		 * Initial delay after boot has been measured at 170ms.
		 * Subequent triggers are < 1ms.
		 */
		if (cnt++ > 250) {
			return -EAGAIN;
		}
		k_sleep(K_MSEC(1));
		ret = bme680_reg_read(dev, BME680_REG_MEAS_STATUS, &status, 1);
		if (ret < 0) {
			return ret;
		}
	} while (!(status & BME680_MSK_NEW_DATA));
	LOG_DBG("New data after %d ms", cnt);

	ret = bme680_reg_read(dev, BME680_REG_FIELD0, &data_regs, sizeof(data_regs));
	if (ret < 0) {
		return ret;
	}

	adc_press = sys_get_be24(data_regs.pressure) >> 4;
	adc_temp = sys_get_be24(data_regs.temperature) >> 4;
	adc_hum = sys_get_be16(data_regs.humidity);
	adc_gas_res = sys_get_be16(data_regs.gas) >> 6;
	data->heatr_stab = data_regs.gas[1] & BME680_MSK_HEATR_STAB;
	gas_range = data_regs.gas[1] & BME680_MSK_GAS_RANGE;

	bme680_calc_temp(data, adc_temp);
	bme680_calc_press(data, adc_press);
	bme680_calc_humidity(data, adc_hum);
	bme680_calc_gas_resistance(data, gas_range, adc_gas_res);
	return 0;
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

	err = bme680_reg_write(dev, BME680_REG_CTRL_GAS_1, BME680_CTRL_GAS_1_VAL);
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
	int err;

	err = bme680_bus_check(dev);
	if (err < 0) {
		LOG_ERR("Bus not ready for '%s'", dev->name);
		return err;
	}

	return pm_device_driver_init(dev, bme680_pm_control);
}

static DEVICE_API(sensor, bme680_api_funcs) = {
	.sample_fetch = bme680_sample_fetch,
	.channel_get = bme680_channel_get,
};

/* Initializes a struct bme680_config for an instance on a SPI bus. */
#define BME680_CONFIG_SPI(inst)				\
	{						\
		.bus.spi = SPI_DT_SPEC_INST_GET(	\
			inst, BME680_SPI_OPERATION, 0),	\
		.bus_io = &bme680_bus_io_spi,		\
	}

/* Initializes a struct bme680_config for an instance on an I2C bus. */
#define BME680_CONFIG_I2C(inst)			       \
	{					       \
		.bus.i2c = I2C_DT_SPEC_INST_GET(inst), \
		.bus_io = &bme680_bus_io_i2c,	       \
	}

/*
 * Main instantiation macro, which selects the correct bus-specific
 * instantiation macros for the instance.
 */
#define BME680_DEFINE(inst)						\
	static struct bme680_data bme680_data_##inst;			\
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
