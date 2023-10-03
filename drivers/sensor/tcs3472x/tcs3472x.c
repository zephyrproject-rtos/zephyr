/* tcs3472x.c - Driver for AMS TCS3472X color sensor */

/*
 * Copyright (c) 2023, Thomas Kiss <thokis@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <sys/errno.h>

#include "tcs3472x.h"

LOG_MODULE_REGISTER(TCS3472X, CONFIG_SENSOR_LOG_LEVEL);

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "TCS3472x driver enabled without any devices"
#endif

static int tcs3472x_reg_read(const struct i2c_dt_spec *i2c, uint8_t reg, uint8_t *buf, uint8_t size)
{
	int ret;

	ret = i2c_burst_read_dt(i2c, (TCS3472X_COMMAND_BIT | reg), buf, size);
	if (ret < 0) {
		LOG_DBG("Could not read reg 0x%x", reg);
	}
	return ret;
}

static int tcs3472x_reg_write(const struct i2c_dt_spec *i2c, uint8_t reg, uint8_t val)
{
	int ret;

	ret = i2c_reg_write_byte_dt(i2c, (TCS3472X_COMMAND_BIT | reg),  val);
	if (ret < 0) {
		LOG_DBG("Could not write 0x%x into reg 0x%x", val, reg);
	}
	return ret;
}

static int tcs3472x_enable(const struct i2c_dt_spec *i2c)
{
	int ret;

	ret = tcs3472x_reg_write(i2c, TCS3472X_ENABLE, TCS3472X_ENABLE_PON);
	if (ret < 0) {
		return -EIO;
	}
	/* There is a 2.4ms warm-up delay when PON was enabled */
	k_sleep(K_USEC(2400));
	ret = tcs3472x_reg_write(i2c, TCS3472X_ENABLE, TCS3472X_ENABLE_PON | TCS3472X_ENABLE_AEN);
	if (ret < 0) {
		return -EIO;
	}
	return ret;
}

static int tcs3472x_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct tcs3472x_data *data = dev->data;
	const struct tcs3472x_config *config = dev->config;
	
	/* Wait one ADC integration cycle */
	k_sleep(K_USEC(config->int_time));

	if (i2c_burst_read_dt(&config->i2c,
			      TCS3472X_COMMAND_BIT | TCS3472X_CDATAL,
			      (uint8_t *)&data->sample_crgb,
                              sizeof(data->sample_crgb))) {
		return -EIO;
	}

	return 0;
}

static int tcs3472x_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct tcs3472x_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_LIGHT:
		val->val1 = sys_le16_to_cpu(data->sample_crgb[0]);
		val->val2 = 0;
		break;
	case SENSOR_CHAN_RED:
		val->val1 = sys_le16_to_cpu(data->sample_crgb[1]);
		val->val2 = 0;
		break;
	case SENSOR_CHAN_GREEN:
		val->val1 = sys_le16_to_cpu(data->sample_crgb[2]);
		val->val2 = 0;
		break;
	case SENSOR_CHAN_BLUE:
		val->val1 = sys_le16_to_cpu(data->sample_crgb[3]);
		val->val2 = 0;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}


static const struct sensor_driver_api tcs3472x_driver_api = {
	.sample_fetch = tcs3472x_sample_fetch,
	.channel_get = tcs3472x_channel_get,
};

static int tcs3472x_init(const struct device *dev)
{

	struct tcs3472x_data *data = dev->data;
	const struct tcs3472x_config *config = dev->config;

	uint8_t chip_id = 0U;
	uint8_t again = 0U;

	(void)memset(data->sample_crgb, 0, sizeof(data->sample_crgb));

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

	/* read device ID */
	if (tcs3472x_reg_read(&config->i2c, TCS3472X_REG_ID, &chip_id, 1) < 0) {
		LOG_ERR("Could not read id");
		return -EIO;
	}

	switch (chip_id) {
	case TCS_34721_34725: {
		LOG_INF("TCS34721/TCS34725 detected");
		break;
		}
	case TCS_34723_34727: {
		LOG_INF("TCS34723/TCS34727 detected");
		break;
		}
	default: {
		LOG_ERR("Unexpected id (%x)", chip_id);
		return -EIO;
		}
	}

	if (tcs3472x_reg_write(&config->i2c, TCS3472X_ATIME, config->atime) < 0) {
		LOG_ERR("Could not set integration time");
		return -EIO;
	}

	switch(config->again) {
	case TCS3472X_GAIN_1X:
		again = TCS3472X_AGAIN_1X;
		break;
	case TCS3472X_GAIN_4X:
		again = TCS3472X_AGAIN_4X;
		break;
	case TCS3472X_GAIN_16X:
		again = TCS3472X_AGAIN_16X;
		break;
	case TCS3472X_GAIN_60X:
		again = TCS3472X_AGAIN_60X;
		break;
	}

	if (tcs3472x_reg_write(&config->i2c, TCS3472X_CONTROL, again) < 0) {
		LOG_ERR("Could not set gain");
		return -EIO;
	}
	if (tcs3472x_enable(&config->i2c) < 0) {
		LOG_ERR("Could not enable tcs3472x");
		return -EIO;
	}
	return EXIT_SUCCESS;
}

#define TCS3472X_DEFINE(inst)							\
	static struct tcs3472x_data tcs3472x_data_##inst;			\
	static const struct tcs3472x_config tcs3472x_config##inst = {		\
		.i2c = I2C_DT_SPEC_INST_GET(inst),				\
		.again = DT_PROP(DT_DRV_INST(inst), again),			\
		/*								\
		 * Calculating ATIME according to TCS3472 datasheet page 15	\
		 */								\
		.atime = 256 - DT_PROP(DT_DRV_INST(inst), int_time) / 2400 + 1,	\
		.int_time = DT_PROP(DT_DRV_INST(inst), int_time),		\
	};									\
										\
	SENSOR_DEVICE_DT_INST_DEFINE(inst,					\
			      tcs3472x_init,					\
			      NULL,						\
			      &tcs3472x_data_##inst,				\
			      &tcs3472x_config##inst,				\
			      POST_KERNEL,					\
			      CONFIG_SENSOR_INIT_PRIORITY,			\
			      &tcs3472x_driver_api);				\

DT_INST_FOREACH_STATUS_OKAY(TCS3472X_DEFINE)
