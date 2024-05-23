/*
 * Driver for Bosch BMP180 digital pressure and temperature sensor
 *
 * Copyright (C) 2024 Jakov Jovic <jovic.jakov@outlook.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://soldered.com/productdata/2022/03/Soldered_BMP180_datasheet.pdf
 */

#define DT_DRV_COMPAT bosch_bmp180

#include "zephyr/device.h"
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <math.h>

#include "bmp180.h"

LOG_MODULE_REGISTER(BMP180, CONFIG_SENSOR_LOG_LEVEL);

static int bmp180_read_calib_data(const struct device *dev)
{
	const struct bmp180_config *config = dev->config;
	struct bmp180_data *data = dev->data;
	uint8_t buf[22];

	if (i2c_burst_read_dt(&config->i2c, BMP180_REG_CALIB, buf, sizeof(buf)) < 0) {
		LOG_ERR(ERROR_MSG_FMT, "read", "calibration data");
		return -EIO;
	}

	data->ac1 = (int16_t)sys_get_be16(&buf[0]);
	data->ac2 = (int16_t)sys_get_be16(&buf[2]);
	data->ac3 = (int16_t)sys_get_be16(&buf[4]);
	data->ac4 = sys_get_be16(&buf[6]);  /* AC4 is unsigned */
	data->ac5 = sys_get_be16(&buf[8]);  /* AC5 is unsigned */
	data->ac6 = sys_get_be16(&buf[10]); /* AC6 is unsigned */
	data->b1 = (int16_t)sys_get_be16(&buf[12]);
	data->b2 = (int16_t)sys_get_be16(&buf[14]);
	data->mb = (int16_t)sys_get_be16(&buf[16]);
	data->mc = (int16_t)sys_get_be16(&buf[18]);
	data->md = (int16_t)sys_get_be16(&buf[20]);

	return 0;
}

static int bmp180_read_uncomp_temp(const struct device *dev)
{
	const struct bmp180_config *config = dev->config;
	struct bmp180_data *data = dev->data;
	uint8_t buf[2];

	if (i2c_reg_write_byte_dt(&config->i2c, BMP180_REG_CTRL_MEAS, BMP180_CMD_READ_TEMP) < 0) {
		LOG_ERR(ERROR_MSG_FMT, "start", "temperature measurement");
		return -EIO;
	}

	k_msleep(5);

	if (i2c_burst_read_dt(&config->i2c, BMP180_REG_OUT_MSB, buf, sizeof(buf)) < 0) {
		LOG_ERR(ERROR_MSG_FMT, "read", "temperature measurement");
		return -EIO;
	}

	data->uncomp_temp = (int16_t)sys_get_be16(&buf[0]);

	return 0;
}

static int bmp180_read_uncomp_press(const struct device *dev)
{
	const struct bmp180_config *config = dev->config;
	struct bmp180_data *data = dev->data;
	uint8_t buf[3];
	uint8_t oversampling_setting = config->oversampling;
	/* See Table 8 from the datasheet */
	uint8_t ctrl_reg_val = BMP180_CMD_READ_PRESS + (oversampling_setting << 6);

	if (i2c_reg_write_byte_dt(&config->i2c, BMP180_REG_CTRL_MEAS, ctrl_reg_val) < 0) {
		LOG_ERR(ERROR_MSG_FMT, "start", "pressure measurement");
		return -EIO;
	}

	/* Sleep for x ms based on the oversampling setting */
	switch (oversampling_setting) {
	case BMP180_MODE_ULTRA_LOW_POWER:
		k_msleep(5);
		break;
	case BMP180_MODE_STANDARD:
		k_msleep(8);
		break;
	case BMP180_MODE_HIGH_RESOLUTION:
		k_msleep(14);
		break;
	case BMP180_MODE_ULTRA_HIGH_RESOLUTION:
		k_msleep(26);
		break;
	default:
		LOG_ERR("Invalid oversampling setting.");
		return -EINVAL;
	}

	/* Read the MSB, LSB, and XLSB from the sensor */
	if (i2c_burst_read_dt(&config->i2c, BMP180_REG_OUT_MSB, buf, sizeof(buf)) < 0) {
		LOG_ERR(ERROR_MSG_FMT, "read", "pressure measurement");
		return -EIO;
	}

	data->uncomp_press = (int32_t)sys_get_be24(buf);
	data->uncomp_press >>= (8 - oversampling_setting);

	return 0;
}

static int bmp180_read_temp(const struct device *dev)
{
	struct bmp180_data *data = dev->data;

	if (bmp180_read_uncomp_temp(dev) < 0) {
		return -EIO;
	}

	int32_t x1 = (data->uncomp_temp - (int32_t)data->ac6) * ((int32_t)data->ac5) >> 15;
	int32_t x2 = ((int32_t)data->mc << 11) / (x1 + (int32_t)data->md);
	int32_t b5 = x1 + x2;

	data->temp = (b5 + 8) >> 4;

	return 0;
}

static int bmp180_read_press(const struct device *dev)
{
	const struct bmp180_config *config = dev->config;
	struct bmp180_data *data = dev->data;

	if (bmp180_read_uncomp_temp(dev) < 0) {
		return -EIO;
	}

	if (bmp180_read_uncomp_press(dev) < 0) {
		return -EIO;
	}

	/* See datasheet, page 15 for more details */
	int32_t x1 = (data->uncomp_temp - (int32_t)data->ac6) * ((int32_t)data->ac5) >> 15;
	int32_t x2 = ((int32_t)data->mc << 11) / (x1 + (int32_t)data->md);
	int32_t b5 = x1 + x2;

	int32_t b6 = b5 - 4000;

	x1 = (data->b2 * ((b6 * b6) >> 12)) >> 11;
	x2 = (data->ac2 * b6) >> 11;
	int32_t x3 = x1 + x2;
	int32_t b3 = (((((int32_t)data->ac1) * 4 + x3) << config->oversampling) + 2) / 4;

	x1 = (data->ac3 * b6) >> 13;
	x2 = (data->b1 * ((b6 * b6) >> 12)) >> 16;
	x3 = ((x1 + x2) + 2) >> 2;
	uint32_t b4 = (data->ac4 * (uint32_t)(x3 + 32768)) >> 15;

	uint32_t b7 = ((uint32_t)data->uncomp_press - b3) * (50000 >> config->oversampling);
	int32_t p;

	if (b7 < 0x80000000) {
		p = (b7 * 2) / b4;
	} else {
		p = (b7 / b4) * 2;
	}

	x1 = (p >> 8) * (p >> 8);
	x1 = (x1 * 3038) >> 16;
	x2 = (-7357 * p) >> 16;
	p = p + ((x1 + x2 + 3791) >> 4); /* Final pressure in Pa */

	data->press = p;

	return 0;
}

static int bmp180_calculate_altitude(const struct device *dev)
{
	struct bmp180_data *data = dev->data;
	/* Standard atmospheric pressure at sea level in Pa */
	const double p0 = 101325;
	const double pwr = 1. / 5.255;

	if (data->press == 0) {
		return -EINVAL;
	}

	/*  International barometric formula - datasheet page 16 */
	data->alt = 44330 * (1.0 - pow((data->press / p0), pwr));
	return 0;
}

static int bmp180_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	if (bmp180_read_temp(dev) < 0) {
		return -EIO;
	}

	if (bmp180_read_press(dev) < 0) {
		return -EIO;
	}

	if (bmp180_calculate_altitude(dev) < 0) {
		return -EIO;
	}

	return 0;
}

static int bmp180_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct bmp180_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		/* Temperature is calculated in steps of 0.1 °C*/
		val->val1 = data->temp / 10;
		val->val2 = data->temp % 10 * 100000;
		break;
	case SENSOR_CHAN_PRESS:
		/* Pressure is calculated in steps of 1 Pa */
		val->val1 = data->press;
		val->val2 = 0;
		break;
	case SENSOR_CHAN_ALTITUDE:
		val->val1 = data->alt;
		val->val2 = 0;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static const struct sensor_driver_api bmp180_driver_api = {
	.sample_fetch = bmp180_sample_fetch,
	.channel_get = bmp180_channel_get,
};

static int bmp180_init(const struct device *dev)
{
	const struct bmp180_config *config = dev->config;
	uint8_t id = 0U;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C device is not ready.");
		return -ENODEV;
	}

	if (i2c_reg_read_byte_dt(&config->i2c, BMP180_REG_CHIP_ID, &id) < 0) {
		LOG_ERR("Error reading chip ID.");
		return -EIO;
	}

	if (id != BMP180_CHIP_ID) {
		LOG_ERR("Chip ID mismatch: read 0x%x, expected 0x%x", id, BMP180_CHIP_ID);
		return -EIO;
	}

	if (bmp180_read_calib_data(dev) < 0) {
		LOG_ERR("Calibration data read failed.");
		return -EIO;
	}

	LOG_INF("BMP180 initialized successfully");
	return 0;
}

#define BMP180_DEFINE(inst)                                                                        \
	static struct bmp180_data bmp180_data_##inst;                                              \
                                                                                                   \
	static const struct bmp180_config bmp180_config_##inst = {                                 \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.oversampling = DT_INST_PROP(inst, oversampling),                                  \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, bmp180_init, NULL, &bmp180_data_##inst,                 \
				     &bmp180_config_##inst, POST_KERNEL,                           \
				     CONFIG_SENSOR_INIT_PRIORITY, &bmp180_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BMP180_DEFINE)
