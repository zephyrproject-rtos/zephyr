/*
 * Copyright (c) 2018 Bosch Sensortec GmbH
 * Copyright (c) 2022, Leonard Pollak
 * Copyright (c) 2025 Nordic Semiconductors ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* bme680.c - Driver for Bosch Sensortec's BME680 temperature, pressure,
 * humidity and gas sensor
 *
 * https://www.bosch-sensortec.com/products/environmental-sensors/gas-sensors/bme680/
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

#include "bme680.h"
#include "bme680_decoder.h"

LOG_MODULE_REGISTER(BME680, CONFIG_SENSOR_LOG_LEVEL);

static uint8_t bme680_calc_res_heat(struct bme680_data *data, uint16_t heatr_temp)
{
	uint8_t heatr_res;
	int32_t var1, var2, var3, var4, var5;
	int32_t heatr_res_x100;
	int32_t amb_temp = 25;    /* Assume ambient temperature to be 25 deg C */

	if (heatr_temp > 400) { /* Cap temperature */
		heatr_temp = 400;
	}

	var1 = ((amb_temp * data->comp_param.par_gh3) / 1000) * 256;
	var2 = (data->comp_param.par_gh1 + 784) * (((((data->comp_param.par_gh2 + 154009)
					   * heatr_temp * 5) / 100)
					 + 3276800) / 10);
	var3 = var1 + (var2 / 2);
	var4 = (var3 / (data->comp_param.res_heat_range + 4));
	var5 = (131 * data->comp_param.res_heat_val) + 65536;
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
	struct bme680_raw_data raw_data;
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
		if (cnt++ > BME680_DELAY_TIMEOUT) {
			return -EAGAIN;
		}
		k_sleep(K_MSEC(1));
		ret = bme680_reg_read(dev, BME680_REG_MEAS_STATUS, &status, 1);
		if (ret < 0) {
			return ret;
		}
	} while (!(status & BME680_MSK_NEW_DATA));
	LOG_DBG("New data after %d ms", cnt);

	ret = bme680_reg_read(dev, BME680_REG_FIELD0, raw_data.buf, sizeof(raw_data.buf));
	if (ret < 0) {
		LOG_ERR("Failed to read raw data");
		return -EIO;
	}

	bme680_compensate_raw_data(chan,
							   &raw_data,
							   &data->comp_param,
							   &data->comp);

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
		val->val1 = data->comp.temp / 100;
		val->val2 = data->comp.temp % 100 * 10000;
		break;
	case SENSOR_CHAN_PRESS:
		/*
		 * data->calc_press has a resolution of 1 Pa.
		 * So 96321 equals 96.321 kPa.
		 */
		val->val1 = data->comp.press / 1000;
		val->val2 = (data->comp.press % 1000) * 1000;
		break;
	case SENSOR_CHAN_HUMIDITY:
		/*
		 * data->calc_humidity has a resolution of 0.001 %RH.
		 * So 46333 equals 46.333 %RH.
		 */
		val->val1 = data->comp.humidity / 1000;
		val->val2 = (data->comp.humidity % 1000) * 1000;
		break;
	case SENSOR_CHAN_GAS_RES:
		/*
		 * data->calc_gas_resistance has a resolution of 1 ohm.
		 * So 100000 equals 100000 ohms.
		 */
		val->val1 = data->comp.gas_resistance;
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

	if (data->comp_param.has_read_compensation) {
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
	data->comp_param.par_t1 = (uint16_t)(BME680_CONCAT_BYTES(buff[32], buff[31]));
	data->comp_param.par_t2 = (int16_t)(BME680_CONCAT_BYTES(buff[1], buff[0]));
	data->comp_param.par_t3 = (uint8_t)(buff[2]);

	/* Pressure related coefficients */
	data->comp_param.par_p1 = (uint16_t)(BME680_CONCAT_BYTES(buff[5], buff[4]));
	data->comp_param.par_p2 = (int16_t)(BME680_CONCAT_BYTES(buff[7], buff[6]));
	data->comp_param.par_p3 = (int8_t)buff[8];
	data->comp_param.par_p4 = (int16_t)(BME680_CONCAT_BYTES(buff[11], buff[10]));
	data->comp_param.par_p5 = (int16_t)(BME680_CONCAT_BYTES(buff[13], buff[12]));
	data->comp_param.par_p6 = (int8_t)(buff[15]);
	data->comp_param.par_p7 = (int8_t)(buff[14]);
	data->comp_param.par_p8 = (int16_t)(BME680_CONCAT_BYTES(buff[19], buff[18]));
	data->comp_param.par_p9 = (int16_t)(BME680_CONCAT_BYTES(buff[21], buff[20]));
	data->comp_param.par_p10 = (uint8_t)(buff[22]);

	/* Humidity related coefficients */
	data->comp_param.par_h1 = (uint16_t)(((uint16_t)buff[25] << 4) | (buff[24] & 0x0f));
	data->comp_param.par_h2 = (uint16_t)(((uint16_t)buff[23] << 4) | ((buff[24]) >> 4));
	data->comp_param.par_h3 = (int8_t)buff[26];
	data->comp_param.par_h4 = (int8_t)buff[27];
	data->comp_param.par_h5 = (int8_t)buff[28];
	data->comp_param.par_h6 = (uint8_t)buff[29];
	data->comp_param.par_h7 = (int8_t)buff[30];

	/* Gas heater related coefficients */
	data->comp_param.par_gh1 = (int8_t)buff[35];
	data->comp_param.par_gh2 = (int16_t)(BME680_CONCAT_BYTES(buff[34], buff[33]));
	data->comp_param.par_gh3 = (int8_t)buff[36];

	data->comp_param.res_heat_val = (int8_t)buff[37];
	data->comp_param.res_heat_range = ((buff[39] & BME680_MSK_RH_RANGE) >> 4);
	data->comp_param.range_sw_err = ((int8_t)(buff[41] & BME680_MSK_RANGE_SW_ERR)) / 16;

	data->comp_param.has_read_compensation = true;
	return 0;
}

static int bme680_power_up(const struct device *dev)
{
	const struct bme680_bus *bus = &((const struct bme680_config *)dev->config)->bus;
	struct bme680_data *data = dev->data;
	int err;

	err = bme680_reg_write(dev, BME680_REG_SOFT_RESET, BME680_SOFT_RESET_VAL);
	if (err < 0) {
		return err;
	}
	k_sleep(K_MSEC(5));

	if (bus->rtio.type == BME680_BUS_TYPE_SPI) {
		uint8_t mem_page;

		err = bme680_reg_read(dev, BME680_REG_STATUS, &mem_page, 1);
		if (err < 0) {
			return err;
		}

		data->mem_page = (mem_page & BME680_SPI_MEM_PAGE_MSK) >> BME680_SPI_MEM_PAGE_POS;
	}

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

	err = bme680_reg_write(dev, BME680_REG_CTRL_MEAS, BME680_CTRL_MEAS_VAL);
	if (err < 0) {
		return err;
	}

	return 0;
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
#ifdef CONFIG_SENSOR_ASYNC_API
	.submit = bme680_submit,
	.get_decoder = bme680_get_decoder,
#endif
};

/*
 * Main instantiation macro, which selects the correct bus-specific
 * instantiation macros for the instance.
 */
#define BME680_DEFINE(inst)					\
	RTIO_DEFINE(bme680_rtio_ctx_##inst, 8, 8);				\
											\
	COND_CODE_1(DT_INST_ON_BUS(inst, i2c),	\
			(I2C_DT_IODEV_DEFINE(bme680_bus_##inst,			\
					 DT_DRV_INST(inst))),	\
	(COND_CODE_1(DT_INST_ON_BUS(inst, spi),	\
			(SPI_DT_IODEV_DEFINE(bme680_bus_##inst,			\
					 DT_DRV_INST(inst),		\
					 SPI_OP_MODE_MASTER | SPI_WORD_SET(8)	\
					 | SPI_TRANSFER_MSB,	\
					 0U)),					\
			())));							\
											\
	static struct bme680_data bme680_data_##inst;			\
	static const struct bme680_config bme680_config_##inst = {	\
		.bus = {							\
			.rtio = {						\
				.ctx = &bme680_rtio_ctx_##inst,				\
				.iodev = &bme680_bus_##inst,				\
				COND_CODE_1(DT_INST_ON_BUS(inst, i2c),		\
				(.type =  BME680_BUS_TYPE_I2C),				\
				(COND_CODE_1(DT_INST_ON_BUS(inst, spi),		\
				(.type = BME680_BUS_TYPE_SPI), ())))		\
			},								\
			COND_CODE_1(DT_INST_ON_BUS(inst, i2c),			\
			(.i2c = I2C_DT_SPEC_INST_GET(inst)),			\
			(COND_CODE_1(DT_INST_ON_BUS(inst, spi),			\
			(.spi = SPI_DT_SPEC_INST_GET(inst, BME680_SPI_OPERATION, 0)),	\
			())))							\
		},									\
		.bus_io = &bme680_bus_rtio,			\
	};										\
	PM_DEVICE_DT_INST_DEFINE(inst, bme680_pm_control);		\
	SENSOR_DEVICE_DT_INST_DEFINE(inst,		\
		bme680_init,						\
		PM_DEVICE_DT_INST_GET(inst),		\
		&bme680_data_##inst,				\
		&bme680_config_##inst,				\
		POST_KERNEL,						\
		CONFIG_SENSOR_INIT_PRIORITY,		\
		&bme680_api_funcs);

/* Create the struct device for every status "okay" node in the devicetree. */
DT_INST_FOREACH_STATUS_OKAY(BME680_DEFINE)
