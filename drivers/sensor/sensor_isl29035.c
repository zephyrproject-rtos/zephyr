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

int isl29035_write_reg(struct isl29035_driver_data *drv_data,
		       uint8_t reg, uint8_t val)
{
	uint8_t tx_buf[2] = {reg, val};

	return i2c_write(drv_data->i2c, tx_buf, sizeof(tx_buf),
			 ISL29035_I2C_ADDRESS);
}

int isl29035_read_reg(struct isl29035_driver_data *drv_data,
		      uint8_t reg, uint8_t *val)
{
	struct i2c_msg msgs[2] = {
		{
			.buf = &reg,
			.len = 1,
			.flags = I2C_MSG_WRITE | I2C_MSG_RESTART,
		},
		{
			.buf = val,
			.len = 1,
			.flags = I2C_MSG_READ | I2C_MSG_STOP,
		}
	};

	return i2c_transfer(drv_data->i2c, msgs, 2, ISL29035_I2C_ADDRESS);
}

int isl29035_update_reg(struct isl29035_driver_data *drv_data,
			uint8_t reg, uint8_t mask, uint8_t val)
{
	uint8_t old_val = 0;
	uint8_t new_val;

	if (isl29035_read_reg(drv_data, reg, &old_val) != DEV_OK) {
		return DEV_FAIL;
	}

	new_val = old_val & ~mask;
	new_val |= val & mask;

	return isl29035_write_reg(drv_data, reg, new_val);
}

static int isl29035_sample_fetch(struct device *dev)
{
	struct isl29035_driver_data *drv_data = dev->driver_data;
	uint8_t msb, lsb;
	int ret;

	ret = isl29035_read_reg(drv_data, ISL29035_DATA_MSB_REG, &msb);
	if (ret != DEV_OK) {
		return DEV_FAIL;
	}

	ret = isl29035_read_reg(drv_data, ISL29035_DATA_LSB_REG, &lsb);
	if (ret != DEV_OK) {
		return DEV_FAIL;
	}

	drv_data->data_sample = (msb << 8) + lsb;

	return DEV_OK;
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

	return DEV_OK;
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
		return DEV_INVALID_CONF;
	}

	dev->driver_api = &isl29035_api;
	drv_data->data_sample = 0;

	/* clear blownout status bit */
	ret = isl29035_update_reg(drv_data, ISL29035_ID_REG,
				  ISL29035_BOUT_MASK, 0);
	if (ret != DEV_OK) {
		DBG("Failed to clear blownout status bit.\n");
		return ret;
	}

	/* set command registers to set default attributes */
	ret = isl29035_write_reg(drv_data, ISL29035_COMMAND_I_REG, 0);
	if (ret != DEV_OK) {
		DBG("Failed to clear COMMAND-I.\n");
		return ret;
	}

	ret = isl29035_write_reg(drv_data, ISL29035_COMMAND_II_REG, 0);
	if (ret != DEV_OK) {
		DBG("Failed to clear COMMAND-II.\n");
		return ret;
	}

	/* set operation mode */
	ret = isl29035_update_reg(drv_data,
				  ISL29035_COMMAND_I_REG, ISL29035_OPMODE_MASK,
				  ISL29035_ACTIVE_OPMODE << ISL29035_OPMODE_SHIFT);
	if (ret != DEV_OK) {
		DBG("Failed to set opmode.\n");
		return ret;
	}

	/* set lux range */
	ret = isl29035_update_reg(drv_data,
				  ISL29035_COMMAND_II_REG, ISL29035_LUX_RANGE_MASK,
				  ISL29035_LUX_RANGE_IDX << ISL29035_LUX_RANGE_SHIFT);
	if (ret != DEV_OK) {
		DBG("Failed to set lux range.\n");
		return ret;
	}

	/* set ADC resolution */
	ret = isl29035_update_reg(drv_data,
				  ISL29035_COMMAND_II_REG, ISL29035_ADC_RES_MASK,
				  ISL29035_ADC_RES_IDX << ISL29035_ADC_RES_SHIFT);
	if (ret != DEV_OK) {
		DBG("Failed to set ADC resolution.\n");
		return ret;
	}

#ifdef CONFIG_ISL29035_TRIGGER
	ret = isl29035_init_interrupt(dev);
	if (ret != DEV_OK) {
		DBG("Failed to initialize interrupt.\n");
		return ret;
	}
#endif

	return DEV_OK;
}

struct isl29035_driver_data isl29035_data;

DEVICE_INIT(isl29035_dev, CONFIG_ISL29035_NAME, &isl29035_init,
	    &isl29035_data, NULL, SECONDARY, CONFIG_ISL29035_INIT_PRIORITY);
