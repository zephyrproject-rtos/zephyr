/* sensor_sx9500.c - Driver for Semtech SX9500 SAR proximity chip */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT semtech_sx9500

#include <errno.h>

#include <kernel.h>
#include <drivers/i2c.h>
#include <drivers/sensor.h>
#include <init.h>
#include <drivers/gpio.h>
#include <sys/__assert.h>
#include <logging/log.h>

#include "sx9500.h"

LOG_MODULE_REGISTER(SX9500, CONFIG_SENSOR_LOG_LEVEL);

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
		 * highest average positive filter.
		 */
	0x0e,	/* Proximity detection threshold: 280 */
	0x00,	/* No automatic compensation, compensate each pin
		 * independently, proximity hysteresis: 32, close
		 * debouncer off, far debouncer off.
		 */
	0x00,	/* No stuck timeout, no periodic compensation. */
};

static int sx9500_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct sx9500_data *data = (struct sx9500_data *) dev->data;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_PROX);

	return i2c_reg_read_byte(data->i2c_master, data->i2c_slave_addr,
				 SX9500_REG_STAT, &data->prox_stat);
}

static int sx9500_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct sx9500_data *data = (struct sx9500_data *) dev->data;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_PROX);

	val->val1 = !!(data->prox_stat &
		       (1 << (4 + CONFIG_SX9500_PROX_CHANNEL)));
	val->val2 = 0;

	return 0;
}

static const struct sensor_driver_api sx9500_api_funcs = {
	.sample_fetch = sx9500_sample_fetch,
	.channel_get = sx9500_channel_get,
#ifdef CONFIG_SX9500_TRIGGER
	.trigger_set = sx9500_trigger_set,
#endif
};

static int sx9500_init_chip(const struct device *dev)
{
	struct sx9500_data *data = (struct sx9500_data *) dev->data;
	uint8_t val;

	if (i2c_write(data->i2c_master, sx9500_reg_defaults,
		      sizeof(sx9500_reg_defaults), data->i2c_slave_addr)
		      < 0) {
		return -EIO;
	}

	/* No interrupts active.  We only activate them when an
	 * application registers a trigger.
	 */
	if (i2c_reg_write_byte(data->i2c_master, data->i2c_slave_addr,
			       SX9500_REG_IRQ_MSK, 0) < 0) {
		return -EIO;
	}

	/* Read irq source reg to clear reset status. */
	if (i2c_reg_read_byte(data->i2c_master, data->i2c_slave_addr,
			      SX9500_REG_IRQ_SRC, &val) < 0) {
		return -EIO;
	}

	return i2c_reg_write_byte(data->i2c_master, data->i2c_slave_addr,
				  SX9500_REG_PROX_CTRL0,
				  1 << CONFIG_SX9500_PROX_CHANNEL);
}

int sx9500_init(const struct device *dev)
{
	struct sx9500_data *data = dev->data;

	data->i2c_master = device_get_binding(DT_INST_BUS_LABEL(0));
	if (!data->i2c_master) {
		LOG_DBG("sx9500: i2c master not found: %s",
		    DT_INST_BUS_LABEL(0));
		return -EINVAL;
	}

	data->i2c_slave_addr = DT_INST_REG_ADDR(0);

	if (sx9500_init_chip(dev) < 0) {
		LOG_DBG("sx9500: failed to initialize chip");
		return -EINVAL;
	}

	if (sx9500_setup_interrupt(dev) < 0) {
		LOG_DBG("sx9500: failed to setup interrupt");
		return -EINVAL;
	}

	return 0;
}

struct sx9500_data sx9500_data;

DEVICE_AND_API_INIT(sx9500, DT_INST_LABEL(0), sx9500_init, &sx9500_data,
		    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &sx9500_api_funcs);
