/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tmp007

#include <device.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>
#include <sys/byteorder.h>
#include <sys/util.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include <sys/__assert.h>
#include <logging/log.h>

#include "tmp007.h"

LOG_MODULE_REGISTER(TMP007, CONFIG_SENSOR_LOG_LEVEL);

int tmp007_reg_read(struct tmp007_data *drv_data,
		uint8_t reg, uint16_t *val)
{
	if (i2c_burst_read(drv_data->i2c, TMP007_I2C_ADDRESS,
				reg, (uint8_t *) val, 2) < 0) {
		LOG_ERR("I2C read failed");
		return -EIO;
	}

	*val = sys_be16_to_cpu(*val);

	return 0;
}

int tmp007_reg_write(struct tmp007_data *drv_data, uint8_t reg, uint16_t val)
{
	uint8_t tx_buf[3] = {reg, val >> 8, val & 0xFF};

	return i2c_write(drv_data->i2c, tx_buf, sizeof(tx_buf),
			 TMP007_I2C_ADDRESS);
}

int tmp007_reg_update(struct tmp007_data *drv_data, uint8_t reg,
		      uint16_t mask, uint16_t val)
{
	uint16_t old_val = 0U;
	uint16_t new_val;

	if (tmp007_reg_read(drv_data, reg, &old_val) < 0) {
		return -EIO;
	}

	new_val = old_val & ~mask;
	new_val |= val & mask;

	return tmp007_reg_write(drv_data, reg, new_val);
}

static int tmp007_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct tmp007_data *drv_data = dev->data;
	uint16_t val;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_AMBIENT_TEMP);

	if (tmp007_reg_read(drv_data, TMP007_REG_TOBJ, &val) < 0) {
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

static const struct sensor_driver_api tmp007_driver_api = {
#ifdef CONFIG_TMP007_TRIGGER
	.attr_set = tmp007_attr_set,
	.trigger_set = tmp007_trigger_set,
#endif
	.sample_fetch = tmp007_sample_fetch,
	.channel_get = tmp007_channel_get,
};

int tmp007_init(const struct device *dev)
{
	struct tmp007_data *drv_data = dev->data;

	drv_data->i2c = device_get_binding(DT_INST_BUS_LABEL(0));
	if (drv_data->i2c == NULL) {
		LOG_DBG("Failed to get pointer to %s device!",
			    DT_INST_BUS_LABEL(0));
		return -EINVAL;
	}

#ifdef CONFIG_TMP007_TRIGGER
	if (tmp007_init_interrupt(dev) < 0) {
		LOG_DBG("Failed to initialize interrupt!");
		return -EIO;
	}
#endif

	return 0;
}

struct tmp007_data tmp007_driver;

DEVICE_DT_INST_DEFINE(0, tmp007_init, device_pm_control_nop,
		    &tmp007_driver,
		    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &tmp007_driver_api);
