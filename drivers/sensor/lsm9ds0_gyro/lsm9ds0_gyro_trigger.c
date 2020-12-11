/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/sensor.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/i2c.h>
#include <sys/byteorder.h>
#include <sys/__assert.h>
#include <logging/log.h>
#include <drivers/gpio.h>

#include "lsm9ds0_gyro.h"

extern struct lsm9ds0_gyro_data lsm9ds0_gyro_data;

LOG_MODULE_DECLARE(LSM9DS0_GYRO, CONFIG_SENSOR_LOG_LEVEL);

static inline void setup_drdy(const struct device *dev,
			      bool enable)
{
	struct lsm9ds0_gyro_data *data = dev->data;
	const struct lsm9ds0_gyro_config *cfg = dev->config;

	gpio_pin_interrupt_configure(data->gpio_drdy,
				     cfg->gpio_drdy_int_pin,
				     enable
				     ? GPIO_INT_EDGE_TO_ACTIVE
				     : GPIO_INT_DISABLE);
}

int lsm9ds0_gyro_trigger_set(const struct device *dev,
			     const struct sensor_trigger *trig,
			     sensor_trigger_handler_t handler)
{
	struct lsm9ds0_gyro_data *data = dev->data;
	const struct lsm9ds0_gyro_config * const config =
					 dev->config;
	uint8_t state;

	if (trig->type == SENSOR_TRIG_DATA_READY) {
		setup_drdy(dev, false);

		state = 0U;
		if (handler) {
			state = 1U;
		}

		data->handler_drdy = handler;
		data->trigger_drdy = *trig;

		if (i2c_reg_update_byte(data->i2c_master,
					config->i2c_slave_addr,
					LSM9DS0_GYRO_REG_CTRL_REG3_G,
					LSM9DS0_GYRO_MASK_CTRL_REG3_G_I2_DRDY,
					state << LSM9DS0_GYRO_SHIFT_CTRL_REG3_G_I2_DRDY)
					< 0) {
			LOG_DBG("failed to set DRDY interrupt");
			return -EIO;
		}

		setup_drdy(dev, true);
		return 0;
	}

	return -ENOTSUP;
}

static void lsm9ds0_gyro_gpio_drdy_callback(const struct device *dev,
					    struct gpio_callback *cb, uint32_t pins)
{
	struct lsm9ds0_gyro_data *data =
		CONTAINER_OF(cb, struct lsm9ds0_gyro_data, gpio_cb);

	setup_drdy(data->dev, false);

	k_sem_give(&data->sem);
}

static void lsm9ds0_gyro_thread_main(struct lsm9ds0_gyro_data *data)
{
	while (1) {
		k_sem_take(&data->sem, K_FOREVER);

		if (data->handler_drdy) {
			data->handler_drdy(data->dev, &data->trigger_drdy);
		}

		setup_drdy(data->dev, true);
	}
}

int lsm9ds0_gyro_init_interrupt(const struct device *dev)
{
	const struct lsm9ds0_gyro_config * const config =
					   dev->config;
	struct lsm9ds0_gyro_data *data = dev->data;

	data->dev = dev;
	k_sem_init(&data->sem, 0, UINT_MAX);

	k_thread_create(&data->thread, data->thread_stack,
			CONFIG_LSM9DS0_GYRO_THREAD_STACK_SIZE,
			(k_thread_entry_t)lsm9ds0_gyro_thread_main,
			data, NULL, NULL, K_PRIO_COOP(10), 0, K_NO_WAIT);

	data->gpio_drdy = device_get_binding(config->gpio_drdy_dev_name);
	if (!data->gpio_drdy) {
		LOG_DBG("gpio controller %s not found",
			    config->gpio_drdy_dev_name);
		return -EINVAL;
	}

	gpio_pin_configure(data->gpio_drdy, config->gpio_drdy_int_pin,
			   GPIO_INPUT | config->gpio_drdy_int_flags);

	gpio_init_callback(&data->gpio_cb,
			   lsm9ds0_gyro_gpio_drdy_callback,
			   BIT(config->gpio_drdy_int_pin));

	if (gpio_add_callback(data->gpio_drdy, &data->gpio_cb) < 0) {
		LOG_DBG("failed to set gpio callback");
		return -EINVAL;
	}

	return 0;
}
