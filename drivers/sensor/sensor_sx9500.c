/* sensor_sx9500.c - Driver for Semtech SX9500 SAR proximity chip */

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

#include <errno.h>

#include <nanokernel.h>
#include <i2c.h>
#include <sensor.h>
#include <init.h>
#include <gpio.h>
#include "sensor_sx9500.h"

static uint8_t sx9500_reg_defaults[] = {
	/*
	 * First number is register address to write to.  The chip
	 * auto-increments the address for subsequent values in a single
	 * write message.
	 */
	SX9500_REG_PROX_CTRL1,

	0x43,	/* Shield enabled, small range. */
	0x77,	/* x8 gain, 167kHz frequency, finest resolution. */
	0x40,	/* Doze enabled, 2x scan period doze, no raw filter. */
	0x30,	/* Average threshold. */
	0x0f,	/* Debouncer off, lowest average negative filter,
		 * highest average postive filter.
		 */
	0x0e,	/* Proximity detection threshold: 280 */
	0x00,	/* No automatic compensation, compensate each pin
		 * independently, proximity hysteresis: 32, close
		 * debouncer off, far debouncer off.
		 */
	0x00,	/* No stuck timeout, no periodic compensation. */
};

struct sx9500_data sx9500_data;

int sx9500_reg_read(struct sx9500_data *data, uint8_t reg, uint8_t *val)
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
		},
	};

	return i2c_transfer(data->i2c_master, msgs, 2, data->i2c_slave_addr);
}

int sx9500_reg_write(struct sx9500_data *data, uint8_t reg, uint8_t val)
{
	uint8_t buf[2] = {reg, val};

	return i2c_write(data->i2c_master, buf, 2, data->i2c_slave_addr);
}

int sx9500_update_bits(struct sx9500_data *data, uint8_t reg,
		       uint8_t mask, uint8_t val)
{
	uint8_t old_val, new_val;
	int ret;

	ret = sx9500_reg_read(data, reg, &old_val);
	if (ret) {
		return ret;
	}

	new_val = (old_val & ~mask) | (val & mask);

	if (new_val == old_val) {
		return 0;
	}

	return sx9500_reg_write(data, reg, new_val);
}

static int sx9500_sample_fetch(struct device *dev)
{
	struct sx9500_data *data = (struct sx9500_data *) dev->driver_data;

	return sx9500_reg_read(data, SX9500_REG_STAT, &data->prox_stat);
}

static int sx9500_channel_get(struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct sx9500_data *data = (struct sx9500_data *) dev->driver_data;

#ifdef CONFIG_SENSOR_DEBUG
	if (chan != SENSOR_CHAN_PROX) {
		return -ENOTSUP;
	}
#endif

	val->type = SENSOR_TYPE_INT;
	val->val1 = !!(data->prox_stat &
		       (1 << (4 + CONFIG_SX9500_PROX_CHANNEL)));

	return 0;
}

static struct sensor_driver_api sx9500_api_funcs = {
	.sample_fetch = sx9500_sample_fetch,
	.channel_get = sx9500_channel_get,
#ifdef CONFIG_SX9500_TRIGGER
	.trigger_set = sx9500_trigger_set,
#endif
};

static int sx9500_init_chip(struct device *dev)
{
	struct sx9500_data *data = (struct sx9500_data *) dev->driver_data;
	int ret;
	uint8_t val;

	ret = i2c_write(data->i2c_master, sx9500_reg_defaults,
			sizeof(sx9500_reg_defaults), data->i2c_slave_addr);
	if (ret) {
		return ret;
	}

	/* No interrupts active.  We only activate them when an
	 * application registers a trigger.
	 */
	ret = sx9500_reg_write(data, SX9500_REG_IRQ_MSK, 0);
	if (ret) {
		return ret;
	}

	/* Read irq source reg to clear reset status. */
	ret = sx9500_reg_read(data, SX9500_REG_IRQ_SRC, &val);
	if (ret) {
		return ret;
	}

	return sx9500_reg_write(data, SX9500_REG_PROX_CTRL0,
				1 << CONFIG_SX9500_PROX_CHANNEL);
}

int sx9500_init(struct device *dev)
{
	struct sx9500_data *data = dev->driver_data;
	int ret;

	dev->driver_api = &sx9500_api_funcs;

	data->i2c_master = device_get_binding(CONFIG_SX9500_I2C_DEV_NAME);
	if (!data->i2c_master) {
		DBG("sx9500: i2c master not found: %s\n",
		    CONFIG_SX9500_I2C_DEV_NAME);
		return DEV_INVALID_CONF;
	}

	data->i2c_slave_addr = CONFIG_SX9500_I2C_ADDR;

	ret = sx9500_init_chip(dev);
	if (ret) {
		DBG("sx9500: failed to initialize chip err %d\n", ret);
		return ret;
	}

	ret = sx9500_setup_interrupt(dev);
	if (ret) {
		DBG("sx9500: failed to setup interrupt err %d\n", ret);
		return ret;
	}

	return 0;
}

DEVICE_INIT(sx9500, CONFIG_SX9500_DEV_NAME, sx9500_init, &sx9500_data,
	    NULL, SECONDARY, CONFIG_SX9500_INIT_PRIORITY);
