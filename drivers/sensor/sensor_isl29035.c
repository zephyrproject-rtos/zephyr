/* sensor_isl29035.c - driver for ISL29035 light sensor */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <arch/cpu.h>
#include <device.h>
#include <i2c.h>
#include <init.h>
#include <nanokernel.h>
#include <sensor.h>
#include <zephyr.h>

#include "sensor_isl29035.h"

static int isl29035_sample_fetch(struct device *dev)
{
	struct isl29035_driver_data *drv_data = dev->driver_data;
	uint8_t msb, lsb;
	int ret;

	ret = i2c_reg_read_byte(drv_data->i2c, ISL29035_I2C_ADDRESS,
				ISL29035_DATA_MSB_REG, &msb);
	if (ret != 0) {
		return -EIO;
	}

	ret = i2c_reg_read_byte(drv_data->i2c, ISL29035_I2C_ADDRESS,
				ISL29035_DATA_LSB_REG, &lsb);
	if (ret != 0) {
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
	uint64_t tmp;

#if CONFIG_ISL29035_MODE_ALS
	/* val = sample_val * lux_range / (2 ^ adc_data_bits) */
	tmp = (uint64_t)drv_data->data_sample * ISL29035_LUX_RANGE;
	val->type = SENSOR_TYPE_INT_PLUS_MICRO;
	val->val1 = tmp >> ISL29035_ADC_DATA_BITS;
	tmp = (tmp & ISL29035_ADC_DATA_MASK) * 1000000;
	val->val2 = tmp >> ISL29035_ADC_DATA_BITS;
#elif CONFIG_ISL29035_MODE_IR
	ARG_UNUSED(tmp);
	val->type = SENSOR_TYPE_INT;
	val->val1 = drv_data->data_sample;
#endif

	return 0;
}

static struct sensor_driver_api isl29035_api = {
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
	int ret;

	drv_data->i2c = device_get_binding(CONFIG_ISL29035_I2C_MASTER_DEV_NAME);
	if (drv_data->i2c == NULL) {
		DBG("Failed to get I2C device.\n");
		return -EINVAL;
	}

	drv_data->data_sample = 0;

	/* clear blownout status bit */
	ret = i2c_reg_update_byte(drv_data->i2c, ISL29035_I2C_ADDRESS,
				  ISL29035_ID_REG, ISL29035_BOUT_MASK, 0);
	if (ret != 0) {
		DBG("Failed to clear blownout status bit.\n");
		return ret;
	}

	/* set command registers to set default attributes */
	ret = i2c_reg_write_byte(drv_data->i2c, ISL29035_I2C_ADDRESS,
				 ISL29035_COMMAND_I_REG, 0);
	if (ret != 0) {
		DBG("Failed to clear COMMAND-I.\n");
		return ret;
	}

	ret = i2c_reg_write_byte(drv_data->i2c, ISL29035_I2C_ADDRESS,
				 ISL29035_COMMAND_II_REG, 0);
	if (ret != 0) {
		DBG("Failed to clear COMMAND-II.\n");
		return ret;
	}

	/* set operation mode */
	ret = i2c_reg_update_byte(drv_data->i2c, ISL29035_I2C_ADDRESS,
				  ISL29035_COMMAND_I_REG,
				  ISL29035_OPMODE_MASK,
				  ISL29035_ACTIVE_OPMODE_BITS);
	if (ret != 0) {
		DBG("Failed to set opmode.\n");
		return ret;
	}

	/* set lux range */
	ret = i2c_reg_update_byte(drv_data->i2c, ISL29035_I2C_ADDRESS,
				  ISL29035_COMMAND_II_REG,
				  ISL29035_LUX_RANGE_MASK,
				  ISL29035_LUX_RANGE_BITS);
	if (ret != 0) {
		DBG("Failed to set lux range.\n");
		return ret;
	}

	/* set ADC resolution */
	ret = i2c_reg_update_byte(drv_data->i2c, ISL29035_I2C_ADDRESS,
				  ISL29035_COMMAND_II_REG,
				  ISL29035_ADC_RES_MASK,
				  ISL29035_ADC_RES_BITS);
	if (ret != 0) {
		DBG("Failed to set ADC resolution.\n");
		return ret;
	}

#ifdef CONFIG_ISL29035_TRIGGER
	ret = isl29035_init_interrupt(dev);
	if (ret != 0) {
		DBG("Failed to initialize interrupt.\n");
		return ret;
	}
#endif

	dev->driver_api = &isl29035_api;

	return 0;
}

struct isl29035_driver_data isl29035_data;

DEVICE_INIT(isl29035_dev, CONFIG_ISL29035_NAME, &isl29035_init,
	    &isl29035_data, NULL, SECONDARY, CONFIG_ISL29035_INIT_PRIORITY);
