/* tcs3472x.c - Driver for AMS TCS3472X color sensor */

/*
 * Copyright (c) 2023, Thomas Kiss <thokis@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/logging/log.h>

#include "tcs3472x.h"
#include "zephyr/drivers/i2c.h"
#include <stdint.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(TCS3472X, CONFIG_SENSOR_LOG_LEVEL);

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "TCS3472x driver enabled without any devices"
#endif

struct tcs3472x_data {
	uint16_t c_chan;
	uint16_t r_chan;
	uint16_t g_chan;
	uint16_t b_chan;
	uint8_t chip_id;
};

int tcs3472x_reg_read(const struct i2c_dt_spec *i2c, uint8_t reg, uint8_t *buf, uint8_t size)
{
	int ret;

	ret = i2c_burst_read_dt(i2c, (TCS3472X_COMMAND_BIT | reg), buf, size);
	if (ret < 0) {
		LOG_DBG("Could not read reg 0x%x", reg);
	}
	return ret;
}

int tcs3472x_reg_write(const struct i2c_dt_spec *i2c, uint8_t reg, uint8_t val)
{
	int ret;

	ret = i2c_reg_write_byte_dt(i2c, (TCS3472X_COMMAND_BIT | reg),  val);
	if (ret < 0) {
		LOG_DBG("Could not write 0x%x into reg 0x%x", val, reg);
	}
	return ret;
}

int tcs3472x_enable(const struct i2c_dt_spec *i2c)
{
	int ret;

	ret = tcs3472x_reg_write(i2c, TCS3472X_ENABLE, TCS3472X_ENABLE_PON);
	if (ret < 0) {
		return -EIO;
	}
	k_sleep(K_USEC(2400));
	ret = tcs3472x_reg_write(i2c, TCS3472X_ENABLE, TCS3472X_ENABLE_PON | TCS3472X_ENABLE_AEN);
	if (ret < 0) {
		return -EIO;
	}
	k_sleep(K_MSEC(CONFIG_TCS3472X_INT_TIME_SCALE * 12 / 5 + 1));
	return ret;
}

int tcs3472x_disable(const struct i2c_dt_spec *i2c)
{
	int ret;
	uint8_t reg = 0;

	ret = tcs3472x_reg_read(i2c, TCS3472X_ENABLE, &reg, 1);
	if (ret < 0) {
		return -EIO;
	}
	ret = tcs3472x_reg_write(i2c, TCS3472X_ENABLE,
				 reg & ~(TCS3472X_ENABLE_PON | TCS3472X_ENABLE_AEN));
	if (ret < 0) {
		return -EIO;
	}
	return ret;
}

static int tcs3472x_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct tcs3472x_data *data = dev->data;
	const struct tcs3472x_config *config = dev->config;
	uint8_t buf[8];
	int size = 8;
	int ret;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	ret = tcs3472x_reg_read(&config->i2c, TCS3472X_CDATAL, buf, size);
	if (ret < 0) {
		return ret;
	}

	data->c_chan = (buf[1] << 8) | (buf[0] & 0xFF);
	data->r_chan = (buf[3] << 8) | (buf[2] & 0xFF);
	data->g_chan = (buf[5] << 8) | (buf[4] & 0xFF);
	data->b_chan = (buf[7] << 8) | (buf[6] & 0xFF);

	return 0;
}

static int tcs3472x_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct tcs3472x_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_LIGHT:
		val->val1 = data->c_chan;
		val->val2 = data->c_chan;
		break;
	case SENSOR_CHAN_RED:
		val->val1 = (uint16_t)((float)(data->r_chan) / data->c_chan * 255) / 100;
		val->val2 = (uint16_t)((float)(data->r_chan) / data->c_chan * 255) % 100 * 10000;
		break;
	case SENSOR_CHAN_GREEN:
		val->val1 = (uint16_t)((float)(data->g_chan) / data->c_chan * 255) / 100;
		val->val2 = (uint16_t)((float)(data->g_chan) / data->c_chan * 255) % 100 * 10000;
		break;
	case SENSOR_CHAN_BLUE:
		val->val1 = (uint16_t)((float)(data->b_chan) / data->c_chan * 255) / 100;
		val->val2 = (uint16_t)((float)(data->b_chan) / data->c_chan * 255) % 100 * 10000;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}


static const struct sensor_driver_api tcs3472x_driver_api = {
	.sample_fetch = tcs3472x_sample_fetch,
	.channel_get = tcs3472x_channel_get,
};

int tcs3472x_init(const struct device *dev)
{

	struct tcs3472x_data *data = dev->data;
	const struct tcs3472x_config *config = dev->config;
	uint8_t id = 0U;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

	/* read device ID */
	if (tcs3472x_reg_read(&config->i2c, TCS3472X_REG_ID, &data->chip_id, 1) < 0) {
		LOG_ERR("Could not read id");
		return -EIO;
	}

	switch (data->chip_id) {
	case TCS_34721_34725: {
		LOG_INF("TCS34721/TCS34725 detected");
		break;
		}
	case TCS_34723_34727: {
		LOG_INF("TCS34723/TCS34727 detected");
		break;
		}
	default: {
		LOG_ERR("Unexpected id (%x)", id);
		return -EIO;
		}
	}

	if (tcs3472x_reg_write(&config->i2c, TCS3472X_ATIME, TCS3472X_INT_TIME) < 0) {
		LOG_ERR("Could not set integration time");
		return -EIO;
	}

	if (tcs3472x_reg_write(&config->i2c, TCS3472X_CONTROL, TCS3472X_GAIN) < 0) {
		LOG_ERR("Could not set gain");
		return -EIO;
	}
	if (tcs3472x_enable(&config->i2c) < 0) {
		LOG_ERR("Could not enable tcs3472x");
		return -EIO;
	}
	return EXIT_SUCCESS;
}

#define TCS3472X_DEFINE(inst)						\
	static struct tcs3472x_data tcs3472x_data_##inst;		\
	static const struct tcs3472x_config tcs3472x_config##inst = {	\
		.i2c = I2C_DT_SPEC_INST_GET(inst),			\
	};								\
									\
	SENSOR_DEVICE_DT_INST_DEFINE(inst,				\
			      tcs3472x_init,				\
			      NULL,					\
			      &tcs3472x_data_##inst,			\
			      &tcs3472x_config##inst,			\
			      POST_KERNEL,				\
			      CONFIG_APPLICATION_INIT_PRIORITY,		\
			      &tcs3472x_driver_api);			\

DT_INST_FOREACH_STATUS_OKAY(TCS3472X_DEFINE)
