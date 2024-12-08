/*
 * Copyright (c) 2017-2019 Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT panasonic_amg88xx

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

#include "amg88xx.h"

LOG_MODULE_REGISTER(AMG88XX, CONFIG_SENSOR_LOG_LEVEL);

static int amg88xx_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	struct amg88xx_data *drv_data = dev->data;
	const struct amg88xx_config *config = dev->config;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_AMBIENT_TEMP);

	if (i2c_burst_read_dt(&config->i2c,
			   AMG88XX_OUTPUT_BASE,
			   (uint8_t *)drv_data->sample,
			   sizeof(drv_data->sample))) {
		return -EIO;
	}

	return 0;
}

static int amg88xx_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct amg88xx_data *drv_data = dev->data;
	size_t len = ARRAY_SIZE(drv_data->sample);

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	for (size_t idx = 0; idx < len; idx++) {
		/* fix negative values */
		if (drv_data->sample[idx] & (1 << 11)) {
			drv_data->sample[idx] |= 0xF000;
		}
		val[idx].val1 = (((int32_t)drv_data->sample[idx]) *
				  AMG88XX_TREG_LSB_SCALING) / 1000000;
		val[idx].val2 = (((int32_t)drv_data->sample[idx]) *
				  AMG88XX_TREG_LSB_SCALING) % 1000000;
	}

	return 0;
}

static int amg88xx_init_device(const struct device *dev)
{
	const struct amg88xx_config *config = dev->config;
	uint8_t tmp;

	if (i2c_reg_read_byte_dt(&config->i2c,
			      AMG88XX_PCLT, &tmp)) {
		LOG_ERR("Failed to read Power mode");
		return -EIO;
	}

	LOG_DBG("Power mode 0x%02x", tmp);
	if (tmp != AMG88XX_PCLT_NORMAL_MODE) {
		if (i2c_reg_write_byte_dt(&config->i2c,
				       AMG88XX_PCLT,
				       AMG88XX_PCLT_NORMAL_MODE)) {
			return -EIO;
		}
		k_busy_wait(AMG88XX_WAIT_MODE_CHANGE_US);
	}

	if (i2c_reg_write_byte_dt(&config->i2c,
			       AMG88XX_RST, AMG88XX_RST_INITIAL_RST)) {
		return -EIO;
	}

	k_busy_wait(AMG88XX_WAIT_INITIAL_RESET_US);

	if (i2c_reg_write_byte_dt(&config->i2c,
			       AMG88XX_FPSC, AMG88XX_FPSC_10FPS)) {
		return -EIO;
	}

	LOG_DBG("");

	return 0;
}

int amg88xx_init(const struct device *dev)
{
	const struct amg88xx_config *config = dev->config;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -EINVAL;
	}

	if (amg88xx_init_device(dev) < 0) {
		LOG_ERR("Failed to initialize device!");
		return -EIO;
	}


#ifdef CONFIG_AMG88XX_TRIGGER
	if (amg88xx_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupt!");
		return -EIO;
	}
#endif

	return 0;
}

static DEVICE_API(sensor, amg88xx_driver_api) = {
#ifdef CONFIG_AMG88XX_TRIGGER
	.attr_set = amg88xx_attr_set,
	.trigger_set = amg88xx_trigger_set,
#endif
	.sample_fetch = amg88xx_sample_fetch,
	.channel_get = amg88xx_channel_get,
};

#define AMG88XX_DEFINE(inst)									\
	static struct amg88xx_data amg88xx_data_##inst;						\
												\
	static const struct amg88xx_config amg88xx_config_##inst = {				\
		.i2c = I2C_DT_SPEC_INST_GET(inst),						\
		IF_ENABLED(CONFIG_AMG88XX_TRIGGER,						\
			   (.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, { 0 }),))	\
	};											\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, amg88xx_init, NULL,					\
			      &amg88xx_data_##inst, &amg88xx_config_##inst, POST_KERNEL,	\
			      CONFIG_SENSOR_INIT_PRIORITY, &amg88xx_driver_api);		\

DT_INST_FOREACH_STATUS_OKAY(AMG88XX_DEFINE)
