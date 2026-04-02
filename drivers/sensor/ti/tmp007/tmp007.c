/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tmp007

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

#include "tmp007.h"

LOG_MODULE_REGISTER(TMP007, CONFIG_SENSOR_LOG_LEVEL);

int tmp007_reg_read(const struct i2c_dt_spec *i2c, uint8_t reg, uint16_t *val)
{
	if (i2c_burst_read_dt(i2c, reg, (uint8_t *) val, 2) < 0) {
		LOG_ERR("I2C read failed");
		return -EIO;
	}

	*val = sys_be16_to_cpu(*val);

	return 0;
}

int tmp007_reg_write(const struct i2c_dt_spec *i2c, uint8_t reg, uint16_t val)
{
	uint8_t tx_buf[3] = {reg, val >> 8, val & 0xFF};

	return i2c_write_dt(i2c, tx_buf, sizeof(tx_buf));
}

int tmp007_reg_update(const struct i2c_dt_spec *i2c, uint8_t reg,
		      uint16_t mask, uint16_t val)
{
	uint16_t old_val = 0U;
	uint16_t new_val;

	if (tmp007_reg_read(i2c, reg, &old_val) < 0) {
		return -EIO;
	}

	new_val = old_val & ~mask;
	new_val |= val & mask;

	return tmp007_reg_write(i2c, reg, new_val);
}

static int tmp007_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct tmp007_data *drv_data = dev->data;
	const struct tmp007_config *cfg = dev->config;
	uint16_t val;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_AMBIENT_TEMP);

	if (tmp007_reg_read(&cfg->i2c, TMP007_REG_TOBJ, &val) < 0) {
		return -EIO;
	}

	if (val & TMP007_DATA_INVALID_BIT) {
		return -EIO;
	}

	drv_data->sample = arithmetic_shift_right((int16_t)val, 2);

	return 0;
}

static int tmp007_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct tmp007_data *drv_data = dev->data;
	int32_t uval;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	uval = (int32_t)drv_data->sample * TMP007_TEMP_SCALE;
	val->val1 = uval / 1000000;
	val->val2 = uval % 1000000;

	return 0;
}

static DEVICE_API(sensor, tmp007_driver_api) = {
#ifdef CONFIG_TMP007_TRIGGER
	.attr_set = tmp007_attr_set,
	.trigger_set = tmp007_trigger_set,
#endif
	.sample_fetch = tmp007_sample_fetch,
	.channel_get = tmp007_channel_get,
};

int tmp007_init(const struct device *dev)
{
	const struct tmp007_config *cfg = dev->config;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

#ifdef CONFIG_TMP007_TRIGGER
	if (cfg->int_gpio.port) {
		if (tmp007_init_interrupt(dev) < 0) {
			LOG_DBG("Failed to initialize interrupt!");
			return -EIO;
		}
	}
#endif

	return 0;
}

#define TMP007_DEFINE(inst)									\
	static struct tmp007_data tmp007_data_##inst;						\
												\
	static const struct tmp007_config tmp007_config_##inst = {				\
		.i2c = I2C_DT_SPEC_INST_GET(inst),						\
		IF_ENABLED(CONFIG_TMP007_TRIGGER,						\
			   (.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, { 0 }),))	\
	};											\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, tmp007_init, NULL,					\
			      &tmp007_data_##inst, &tmp007_config_##inst, POST_KERNEL,		\
			      CONFIG_SENSOR_INIT_PRIORITY, &tmp007_driver_api);			\

DT_INST_FOREACH_STATUS_OKAY(TMP007_DEFINE)
