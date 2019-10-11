/* sensor_isl29035.c - driver for ISL29035 light sensor */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <init.h>
#include <drivers/i2c.h>
#include <drivers/sensor.h>
#include <sys/__assert.h>
#include <logging/log.h>

#include "isl29035.h"

LOG_MODULE_REGISTER(ISL29035, CONFIG_SENSOR_LOG_LEVEL);

static int isl29035_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct isl29035_driver_data *drv_data = dev->driver_data;
	u8_t msb, lsb;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	if (i2c_reg_read_byte(drv_data->i2c, ISL29035_I2C_ADDRESS,
				ISL29035_DATA_MSB_REG, &msb) < 0) {
		return -EIO;
	}

	if (i2c_reg_read_byte(drv_data->i2c, ISL29035_I2C_ADDRESS,
				ISL29035_DATA_LSB_REG, &lsb) < 0) {
		return -EIO;
	}

	drv_data->data_sample = (msb << 8) + lsb;

	return 0;
}

static int isl29035_channel_get(struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	struct isl29035_driver_data *drv_data = dev->driver_data;
	u64_t tmp;

#if CONFIG_ISL29035_MODE_ALS
	/* val = sample_val * lux_range / (2 ^ adc_data_bits) */
	tmp = (u64_t)drv_data->data_sample * ISL29035_LUX_RANGE;
	val->val1 = tmp >> ISL29035_ADC_DATA_BITS;
	tmp = (tmp & ISL29035_ADC_DATA_MASK) * 1000000U;
	val->val2 = tmp >> ISL29035_ADC_DATA_BITS;
#elif CONFIG_ISL29035_MODE_IR
	ARG_UNUSED(tmp);
	val->val1 = drv_data->data_sample;
	val->val2 = 0;
#endif

	return 0;
}

static const struct sensor_driver_api isl29035_api = {
#if CONFIG_ISL29035_TRIGGER
	.attr_set = &isl29035_attr_set,
	.trigger_set = &isl29035_trigger_set,
#endif
	.sample_fetch = &isl29035_sample_fetch,
	.channel_get = &isl29035_channel_get,
};

static int isl29035_init(struct device *dev)
{
	struct isl29035_driver_data *drv_data = dev->driver_data;

	drv_data->i2c = device_get_binding(CONFIG_ISL29035_I2C_MASTER_DEV_NAME);
	if (drv_data->i2c == NULL) {
		LOG_DBG("Failed to get I2C device.");
		return -EINVAL;
	}

	drv_data->data_sample = 0U;

	/* clear blownout status bit */
	if (i2c_reg_update_byte(drv_data->i2c, ISL29035_I2C_ADDRESS,
				ISL29035_ID_REG, ISL29035_BOUT_MASK, 0) < 0) {
		LOG_DBG("Failed to clear blownout status bit.");
		return -EIO;
	}

	/* set command registers to set default attributes */
	if (i2c_reg_write_byte(drv_data->i2c, ISL29035_I2C_ADDRESS,
			       ISL29035_COMMAND_I_REG, 0) < 0) {
		LOG_DBG("Failed to clear COMMAND-I.");
		return -EIO;
	}

	if (i2c_reg_write_byte(drv_data->i2c, ISL29035_I2C_ADDRESS,
				 ISL29035_COMMAND_II_REG, 0) < 0) {
		LOG_DBG("Failed to clear COMMAND-II.");
		return -EIO;
	}

	/* set operation mode */
	if (i2c_reg_update_byte(drv_data->i2c, ISL29035_I2C_ADDRESS,
				  ISL29035_COMMAND_I_REG,
				  ISL29035_OPMODE_MASK,
				  ISL29035_ACTIVE_OPMODE_BITS) < 0) {
		LOG_DBG("Failed to set opmode.");
		return -EIO;
	}

	/* set lux range */
	if (i2c_reg_update_byte(drv_data->i2c, ISL29035_I2C_ADDRESS,
				ISL29035_COMMAND_II_REG,
				ISL29035_LUX_RANGE_MASK,
				ISL29035_LUX_RANGE_BITS) < 0) {
		LOG_DBG("Failed to set lux range.");
		return -EIO;
	}

	/* set ADC resolution */
	if (i2c_reg_update_byte(drv_data->i2c, ISL29035_I2C_ADDRESS,
				ISL29035_COMMAND_II_REG,
				ISL29035_ADC_RES_MASK,
				ISL29035_ADC_RES_BITS) < 0) {
		LOG_DBG("Failed to set ADC resolution.");
		return -EIO;
	}

#ifdef CONFIG_ISL29035_TRIGGER
	if (isl29035_init_interrupt(dev) < 0) {
		LOG_DBG("Failed to initialize interrupt.");
		return -EIO;
	}
#endif

	return 0;
}

struct isl29035_driver_data isl29035_data;

DEVICE_AND_API_INIT(isl29035_dev, CONFIG_ISL29035_NAME, &isl29035_init,
		    &isl29035_data, NULL, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &isl29035_api);
