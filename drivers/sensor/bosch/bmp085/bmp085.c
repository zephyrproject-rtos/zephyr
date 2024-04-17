/*
 * Copyright (c) 2024, Mario Paja
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bmp085

#include <zephyr/init.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/types.h>

#include "bmp085.h"

LOG_MODULE_REGISTER(BMP085, CONFIG_SENSOR_LOG_LEVEL);

static int bmp085_read_raw_temp(const struct device *dev)
{
	struct bmp085_data *data = dev->data;
	int ret = 0;
	uint8_t buf[2];

	ret = bmp085_write_byte(dev, BMP085_CTRL, BMP085_CMD_READ_TEMP);

	if (ret < 0) {
		return ret;
	}
	k_msleep(5);

	ret = bmp085_read(dev, BMP085_REG_MSB, buf, sizeof(buf));
	if (ret < 0) {
		LOG_DBG("Failed to read MSB.");
		return ret;
	}

	data->raw_temp = buf[1] | (buf[0] << 8);

	return ret;
}

static int bmp085_read_raw_pres(const struct device *dev, uint8_t oversampling)
{
	struct bmp085_data *data = dev->data;
	const struct bmp085_config *cfg = dev->config;
	int ret = 0;
	uint8_t buf[2];
	uint8_t buf_xlsb;

	ret = bmp085_write_byte(dev, BMP085_CTRL, (BMP085_CMD_READ_PRESS + (oversampling << 6)));
	if (ret < 0) {
		return ret;
	}

	if (cfg->oversampling == BMP085_MODE_1_ULTRALOWPOWER) {
		k_msleep(5);
	} else if (cfg->oversampling == BMP085_MODE_2_STANDARD) {
		k_msleep(8);
	} else if (cfg->oversampling == BMP085_MODE_3_HIGHRES) {
		k_msleep(14);
	} else {
		k_msleep(26);
	}

	ret = bmp085_read(dev, BMP085_REG_MSB, buf, sizeof(buf));
	if (ret < 0) {
		LOG_DBG("Failed to read MSB.");
		return ret;
	}

	data->raw_press = buf[1] | (buf[0] << 8);
	data->raw_press <<= 8;

	ret = bmp085_read(dev, BMP085_REG_XLSB, &buf_xlsb, sizeof(buf_xlsb));
	if (ret < 0) {
		LOG_DBG("Failed to read XLSB.");
		return ret;
	}

	data->raw_press |= buf_xlsb;
	data->raw_press >>= (8 - oversampling);

	return ret;
}

static int bmp085_read_temp(const struct device *dev)
{
	struct bmp085_data *data = dev->data;
	int ret = 0;

	ret |= bmp085_read_raw_temp(dev);
	if (ret != 0) {
		return ret;
	}

	int32_t x1 = (data->raw_temp - (int32_t)data->ac6) * ((int32_t)data->ac5) >> 15;
	int32_t x2 = ((int32_t)data->mc << 11) / (x1 + (int32_t)data->md);
	int32_t b5 = x1 + x2;

	data->temp = (b5 + 8) >> 4;

	return 0;
}

static int bmp085_read_press(const struct device *dev, uint8_t oversampling)
{
	struct bmp085_data *data = dev->data;
	int ret = 0;

	ret |= bmp085_read_raw_temp(dev);
	if (ret != 0) {
		return ret;
	}

	ret |= bmp085_read_raw_pres(dev, oversampling);
	if (ret != 0) {
		return ret;
	}

	int32_t b3, b5, b6, x1, x2, x3;
	uint32_t b4, b7;

	/* See datasheet, page 13
	 * Calculation of pressure and temperature
	 * Datasheet: https://mm.digikey.com/Volume0/opasdata/d220001/medias/docus/1085/BMP085.pdf
	 */

	x1 = (data->raw_temp - (int32_t)data->ac6) * ((int32_t)data->ac5) >> 15;
	x2 = ((int32_t)data->mc << 11) / (x1 + (int32_t)data->md);
	b5 = x1 + x2;

	b6 = b5 - 4000;
	x1 = ((int32_t)data->b2 * ((b6 * b6) >> 12)) >> 11;
	x2 = ((int32_t)data->ac2 * b6) >> 11;
	x3 = x1 + x2;
	b3 = ((((int32_t)data->ac1 * 4 + x3) << oversampling) + 2) / 4;

	x1 = ((int32_t)data->ac3 * b6) >> 13;
	x2 = ((int32_t)data->b1 * ((b6 * b6) >> 12)) >> 16;
	x3 = ((x1 + x2) + 2) >> 2;
	b4 = ((uint32_t)data->ac4 * (uint32_t)(x3 + 32768)) >> 15;
	b7 = ((uint32_t)data->raw_press - b3) * (uint32_t)(50000UL >> oversampling);

	if (b7 < 0x80000000) {
		data->press = (b7 * 2) / b4;
	} else {
		data->press = (b7 / b4) * 2;
	}
	x1 = (data->press >> 8) * (data->press >> 8);
	x1 = (x1 * 3038) >> 16;
	x2 = (-7357 * data->press) >> 16;

	data->press = data->press + ((x1 + x2 + (int32_t)3791) >> 4);

	return 0;
}

static int bmp085_read_cal_coef(const struct device *dev)
{
	struct bmp085_data *data = dev->data;
	int ret = 0;
	uint8_t buf[22];

	ret = bmp085_read(dev, BMP085_REG_CAL_COEF, buf, sizeof(buf));
	if (ret < 0) {
		LOG_DBG("Failed to read calibration coefficients.");
		return ret;
	}

	data->ac1 = buf[1] | (buf[0] << 8);
	data->ac2 = buf[3] | (buf[2] << 8);
	data->ac3 = buf[5] | (buf[4] << 8);
	data->ac4 = buf[7] | (buf[6] << 8);
	data->ac5 = buf[9] | (buf[8] << 8);
	data->ac6 = buf[11] | (buf[10] << 8);
	data->b1 = buf[13] | (buf[12] << 8);
	data->b2 = buf[15] | (buf[14] << 8);
	data->mb = buf[17] | (buf[16] << 8);
	data->mc = buf[19] | (buf[18] << 8);
	data->md = buf[21] | (buf[20] << 8);

	return 0;
}

static int bmp085_read(const struct device *dev, uint8_t reg_addr, uint8_t *data, uint8_t len)
{
	const struct bmp085_config *cfg = dev->config;
	int ret = 0;

	if (i2c_burst_read_dt(&cfg->i2c, reg_addr, data, len) < 0) {
		ret = -EIO;
	}

	return ret;
}

static int bmp085_read_byte(const struct device *dev, uint8_t reg_addr, uint8_t *byte)
{
	return bmp085_read(dev, reg_addr, byte, 1);
}

static int bmp085_write(const struct device *dev, uint8_t reg_addr, uint8_t *data, uint8_t len)
{
	const struct bmp085_config *cfg = dev->config;
	int ret = 0;

	if (i2c_burst_write_dt(&cfg->i2c, reg_addr, data, len) < 0) {
		ret = -EIO;
	}

	return ret;
}

static int bmp085_write_byte(const struct device *dev, uint8_t reg_addr, uint8_t byte)
{
	return bmp085_write(dev, reg_addr, &byte, 1);
}

static int bmp085_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct bmp085_config *cfg = dev->config;
	int ret = 0;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	ret |= bmp085_read_temp(dev);
	ret |= bmp085_read_press(dev, cfg->oversampling);

	return ret;
}

static int bmp085_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct bmp085_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		val->val1 = data->temp / 10;
		val->val2 = data->temp % 10;
		break;
	case SENSOR_CHAN_PRESS:
		val->val1 = data->press / 1000;
		val->val2 = data->press % 1000;
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

static const struct sensor_driver_api bmp085_api = {
	.sample_fetch = bmp085_sample_fetch,
	.channel_get = bmp085_channel_get,
};

static int bmp085_init(const struct device *dev)
{
	struct bmp085_data *data = dev->data;
	const struct bmp085_config *cfg = dev->config;
	int ret = 0;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

	if (bmp085_read_byte(dev, BMP085_REG_CHIPID, &data->chip_id) < 0) {
		LOG_DBG("Failed to read chip id.");
		return -EIO;
	}

	if (data->chip_id != BMP085_CHIP_ID) {
		LOG_DBG("Unsupported chip detected (0x%x)!", data->chip_id);
		return -ENODEV;
	}

	ret |= bmp085_read_cal_coef(dev);

	return ret;
}

#define BMP085_DEFINE(inst)                                                                        \
	static struct bmp085_data bmp085_data_##inst;                                              \
                                                                                                   \
	static const struct bmp085_config bmp085_config_##inst = {                                 \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.oversampling = DT_INST_PROP(inst, oversampling),                                  \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, bmp085_init, NULL, &bmp085_data_##inst,                 \
				     &bmp085_config_##inst, POST_KERNEL,                           \
				     CONFIG_SENSOR_INIT_PRIORITY, &bmp085_api);

DT_INST_FOREACH_STATUS_OKAY(BMP085_DEFINE)
