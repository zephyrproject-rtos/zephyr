/* sensor_sx9500.c - Driver for Semtech SX9500 SAR proximity chip */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT semtech_sx9500

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

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
	struct sx9500_data *data = dev->data;
	const struct sx9500_config *cfg = dev->config;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_PROX);

	return i2c_reg_read_byte_dt(&cfg->i2c, SX9500_REG_STAT, &data->prox_stat);
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
	const struct sx9500_config *cfg = dev->config;
	uint8_t val;

	if (i2c_write_dt(&cfg->i2c, sx9500_reg_defaults,
		      sizeof(sx9500_reg_defaults))
		      < 0) {
		return -EIO;
	}

	/* No interrupts active.  We only activate them when an
	 * application registers a trigger.
	 */
	if (i2c_reg_write_byte_dt(&cfg->i2c, SX9500_REG_IRQ_MSK, 0) < 0) {
		return -EIO;
	}

	/* Read irq source reg to clear reset status. */
	if (i2c_reg_read_byte_dt(&cfg->i2c, SX9500_REG_IRQ_SRC, &val) < 0) {
		return -EIO;
	}

	return i2c_reg_write_byte_dt(&cfg->i2c, SX9500_REG_PROX_CTRL0,
				     1 << CONFIG_SX9500_PROX_CHANNEL);
}

int sx9500_init(const struct device *dev)
{
	const struct sx9500_config *cfg = dev->config;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	if (sx9500_init_chip(dev) < 0) {
		LOG_DBG("sx9500: failed to initialize chip");
		return -EINVAL;
	}

#ifdef CONFIG_SX9500_TRIGGER
	if (cfg->int_gpio.port) {
		if (sx9500_setup_interrupt(dev) < 0) {
			LOG_DBG("sx9500: failed to setup interrupt");
			return -EINVAL;
		}
	}
#endif

	return 0;
}

#define SX9500_DEFINE(inst)									\
	struct sx9500_data sx9500_data_##inst;							\
												\
	static const struct sx9500_config sx9500_config_##inst = {				\
		.i2c = I2C_DT_SPEC_INST_GET(inst),						\
		IF_ENABLED(CONFIG_SX9500_TRIGGER,						\
			   (.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, { 0 }),))	\
	};											\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, sx9500_init, NULL,					\
			      &sx9500_data_##inst, &sx9500_config_##inst, POST_KERNEL,		\
			      CONFIG_SENSOR_INIT_PRIORITY, &sx9500_api_funcs);			\

DT_INST_FOREACH_STATUS_OKAY(SX9500_DEFINE)
