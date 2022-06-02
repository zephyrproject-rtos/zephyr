/* bmp180.c - Driver for Bosch BMP180 temperature and pressure sensor */

/*
 * Copyright (c) 2016, 2017 Intel Corporation
 * Copyright (c) 2017 IpTronix S.r.l.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>
#include <math.h>

#include "bmp180.h"

#define DT_DRV_COMPAT 		bosch_bmp180
#define BMP180_COMP_SIZE 	11

LOG_MODULE_REGISTER(BMP180, CONFIG_SENSOR_LOG_LEVEL);

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "BMP180 driver enabled without any devices"
#endif

struct bmp180_data {
	struct bmp180_comp_data_t {
		int16_t AC1;
		int16_t AC2;
		int16_t AC3;
		uint16_t AC4;
		uint16_t AC5;
		uint16_t AC6;
		int16_t B1;
		int16_t B2;
		int16_t MB;
		int16_t MC;
		int16_t MD;
	} comp_data;

	int32_t comp_temp;
	int32_t comp_press;
	int32_t altitude; 
	uint8_t chip_id;
	int32_t x1;
	int32_t x2;
	int32_t x3;
	int32_t b3;
	uint32_t b4;
	int32_t b5;
	int32_t b6;
	uint32_t b7;
};

struct bmp180_config {
	union bmp180_bus bus;
	const struct bmp180_bus_io *bus_io;
	int oversampling;
};

static int bmp180_bus_check(const struct device *dev);
static int bmp180_reg_read(const struct device *dev, uint8_t start, uint8_t *buf, int size);
static int bmp180_reg_write(const struct device *dev, uint8_t reg, uint8_t val);
static int bmp180_wait_until_ready(const struct device *dev);
static int bmp180_read_compensation(const struct device *dev);
static void bmp180_compensate_temp(struct bmp180_data *data, int32_t adc_temp);
static void bmp180_compensate_press(struct bmp180_data *data, int32_t adc_press, const uint8_t oversampling);
static void bmp180_calculate_altitude(struct bmp180_data *data);

static int bmp180_chip_init(const struct device *dev) {
	struct bmp180_data *data = dev->data;

	int err = bmp180_bus_check(dev);
	if (err < 0) {
		LOG_ERR("bus check failed: %d", err);
		return err;
	}

	err = bmp180_reg_read(dev, BMP180_REG_ID, &data->chip_id, 1);
	if (err < 0) {
		LOG_ERR("ID read failed: %d", err);
		return err;
	}

	if (data->chip_id == BMP180_CHIP_ID)
		LOG_DBG("ID OK");
	else {
		LOG_ERR("bad chip id 0x%x", data->chip_id);
		return -ENOTSUP;
	}

	err = bmp180_reg_write(dev, BMP180_REG_RESET, BMP180_CMD_SOFT_RESET);
	if (err < 0)
		LOG_ERR("Soft-reset failed: %d", err);

	err = bmp180_wait_until_ready(dev);
	if (err < 0)
		return err;

	err = bmp180_read_compensation(dev);
	if (err < 0) {
		LOG_ERR("Compensation data read failed: %d", err);
		return err;
	}

	k_sleep(K_MSEC(1));

	LOG_DBG("\"%s\" OK", dev->name);
	return 0;
}

static int bmp180_sample_fetch(const struct device *dev, enum sensor_channel chan) {

	struct bmp180_data *data = dev->data;
	const struct bmp180_config *config = dev->config;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	int ret = bmp180_reg_write(dev, BMP180_REG_MEAS_CTRL, (BMP_START_CONVERSION | BMP_SELECT_TEMPERATURE));
	if (ret < 0)
		return ret;
	
	ret = bmp180_wait_until_ready(dev);
	if (ret < 0)
		return ret;

	int16_t uncompensated_temperature = 0;
	ret = bmp180_reg_read(dev, BMP180_REG_OUT_MSB, (uint8_t*)&uncompensated_temperature, 2);
	if (ret < 0)
		return ret;
	uncompensated_temperature = sys_be16_to_cpu(uncompensated_temperature);

	ret = bmp180_reg_write(dev, BMP180_REG_MEAS_CTRL, (BMP180_OVERSAMPLING_REGVALUE(config->oversampling) | BMP_START_CONVERSION | BMP_SELECT_PRESSURE));
	if (ret < 0)
		return ret;
	
	ret = bmp180_wait_until_ready(dev);
	if (ret < 0)
		return ret;

	uint8_t uncompensated_pressure_buff[3];
	ret = bmp180_reg_read(dev, BMP180_REG_OUT_MSB, uncompensated_pressure_buff, 3);
	if (ret < 0)
		return ret;
	
	/*
	* Calculation code taken from BMP180 datasheet, Section 3.5
	* "Calculating pressure and temperature".
	*/
	uint32_t uncompensated_pressure = \
		uncompensated_pressure_buff[0] << 16 |
		uncompensated_pressure_buff[1] << 8  | 
		uncompensated_pressure_buff[2];
	uncompensated_pressure >>= (8 - config->oversampling);

	bmp180_compensate_temp(data, uncompensated_temperature);
	bmp180_compensate_press(data, uncompensated_pressure, config->oversampling);
	bmp180_calculate_altitude(data);
	return 0;
}

static int bmp180_channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val) {
	struct bmp180_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		/*
		* data->comp_temp has a resolution of 0.1 degC. So
		* 512 equals 51.2 degC.
		*/
		val->val1 = data->comp_temp / 10;
		val->val2 = data->comp_temp % 10 * 100000;
		break;
	case SENSOR_CHAN_PRESS:
		/*
		* data->comp_press has a resolution of 1 Pa.
		*/
		val->val1 = data->comp_press;
		val->val2 = 0;
		break;
	case SENSOR_CHAN_ALTITUDE:
		/*
		* data->altitude has a resolution of 1 m.
		*/
		val->val1 = data->altitude;
		val->val2 = 0;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static const struct sensor_driver_api bmp180_api_funcs = {
	.sample_fetch = bmp180_sample_fetch,
	.channel_get = bmp180_channel_get,
};

/*
 * Compensation code taken from BMP180 datasheet, Section 3.5
 * "Calculating pressure and temperature".
 */
static void bmp180_compensate_temp(struct bmp180_data *data, int32_t adc_temp) {
	data->x1 = ((adc_temp - data->comp_data.AC6) * data->comp_data.AC5) >> 15;
	data->x2 = (data->comp_data.MC << 11) / (data->x1 + data->comp_data.MD);
	data->b5 = data->x1 + data->x2;
	data->comp_temp = (data->b5 + 8) >> 4;
}

/*
 * Compensation code taken from BMP180 datasheet, Section 3.5
 * "Calculating pressure and temperature".
 */
static void bmp180_compensate_press(struct bmp180_data *data, int32_t adc_press, const uint8_t oversampling) {

	data->b6 = data->b5 - 4000;
	data->x1 = (data->comp_data.B2 * ((data->b6 * data->b6) >> 12)) >> 11;
	data->x2 = (data->comp_data.AC2 * data->b6) >> 11;
	data->x3 = data->x1 + data->x2;
	data->b3 = (((data->comp_data.AC1 * 4 + data->x3) << oversampling) + 2) >> 2;

	data->x1 = (data->comp_data.AC3 * data->b6) >> 13;
	data->x2 = (data->comp_data.B1 * ((data->b6 * data->b6) >> 12)) >> 16;
	data->x3 = ((data->x1 + data->x2) + 2) >> 2;
	data->b4 = (data->comp_data.AC4 * (unsigned long)(data->x3 + 32768UL)) >> 15;
	data->b7 = ((unsigned long)adc_press - data->b3) * (50000UL >> oversampling);

	if(data->b7 < 0x80000000)
		data->comp_press = (data->b7 * 2) / data->b4;
	else
		data->comp_press = (data->b7 / data->b4) * 2;
	
	data->x1 = (data->comp_press >> 8) * (data->comp_press >> 8);
	data->x1 = (data->x1 * 3038) >> 16;
	data->x2 = (-7357 * data->comp_press) >> 16;

	data->comp_press = data->comp_press + ((data->x1 + data->x2 + 3791) >> 4);
}

/* Pressure at sea level, in Pa */ 
#define NORMAL_PRESSURE 101325.0

/* 
 * Value taken from BMP180 datasheet, Section 3.6
 * 1/5.255 = 0.190295
 */ 
#define FRACTION_POWER	0.190295

/*
 * Calculating code taken from BMP180 datasheet, Section 3.6
 * "Calculating absolute altitude".
 */
static void bmp180_calculate_altitude(struct bmp180_data *data) {
	data->altitude = 44330 * (1.0 - pow((data->comp_press / NORMAL_PRESSURE), FRACTION_POWER));
}

static inline int bmp180_bus_check(const struct device *dev) {
	const struct bmp180_config *cfg = dev->config;
	return cfg->bus_io->check(&cfg->bus);
}

static inline int bmp180_reg_read(const struct device *dev, uint8_t start, uint8_t *buf, int size) {
	const struct bmp180_config *cfg = dev->config;
	return cfg->bus_io->read(&cfg->bus, start, buf, size);
}

static inline int bmp180_reg_write(const struct device *dev, uint8_t reg, uint8_t val) {
	const struct bmp180_config *cfg = dev->config;
	return cfg->bus_io->write(&cfg->bus, reg, val);
}

static int bmp180_wait_until_ready(const struct device *dev) {
	uint8_t status = 0;
	do {
		k_sleep(K_MSEC(3));
		const int ret = bmp180_reg_read(dev, BMP180_REG_MEAS_CTRL, &status, 1);
		if (ret < 0) {
			return ret;
		}
	} while (status & BMP_START_CONVERSION);
	return 0;
}

static int bmp180_read_compensation(const struct device *dev) {
	struct bmp180_data *data = dev->data;
	uint16_t buf[BMP180_COMP_SIZE];
	int err = 0;

	err = bmp180_reg_read(dev, BMP180_REG_COMP_START, (uint8_t*)buf, sizeof(buf));

	if (err < 0) {
		LOG_ERR("compensation data read failed: %d", err);
		return err;
	}

	uint16_t* ptr = (uint16_t*)&data->comp_data;
	for( int i = 0; i < BMP180_COMP_SIZE; i++) {
		ptr[i] = sys_be16_to_cpu(buf[i]);
	}

	LOG_INF("Compensation data:\r\nAC1 = %d\r\nAC2 = %d\r\nAC3 = %d\r\nAC4 = %u\r\nAC5 = %u\r\nAC6 = %u\r\nB1 = %d\r\nB2 = %d\r\nMB %d\r\nMC = %d\r\nMD = %d",
		data->comp_data.AC1,
		data->comp_data.AC2,
		data->comp_data.AC3,
		data->comp_data.AC4,
		data->comp_data.AC5,
		data->comp_data.AC6,
		data->comp_data.B1,
		data->comp_data.B2,
		data->comp_data.MB,
		data->comp_data.MC,
		data->comp_data.MD
	);

	return 0;
}

/* Initializes a struct bmp180_config for an instance on an I2C bus. */
#define BMP180_CONFIG(inst)			       \
	{					       \
		.bus.i2c = I2C_DT_SPEC_INST_GET(inst), \
		.bus_io = &bmp180_bus_io_i2c,	       \
		.oversampling = DT_INST_PROP(inst, pressure_oversampling_level)\
	}

/*
 * Main instantiation macro, which selects the correct bus-specific
 * instantiation macros for the instance.
 */
#define BMP180_DEFINE(inst)						\
	static struct bmp180_data bmp180_data_##inst;			\
	static const struct bmp180_config bmp180_config_##inst =	\
			    BMP180_CONFIG(inst);		\
									\
	DEVICE_DT_INST_DEFINE(inst,					\
			 bmp180_chip_init,				\
			 NULL,			\
			 &bmp180_data_##inst,				\
			 &bmp180_config_##inst,				\
			 POST_KERNEL,					\
			 CONFIG_SENSOR_INIT_PRIORITY,			\
			 &bmp180_api_funcs);

/* Create the struct device for every status "okay" node in the devicetree. */
DT_INST_FOREACH_STATUS_OKAY(BMP180_DEFINE)
