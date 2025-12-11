/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

#include "lsm9ds0_gyro.h"

extern struct lsm9ds0_gyro_data lsm9ds0_gyro_data;

LOG_MODULE_DECLARE(LSM9DS0_GYRO, CONFIG_SENSOR_LOG_LEVEL);

static inline void setup_drdy(const struct device *dev,
			      bool enable)
{
	const struct lsm9ds0_gyro_config *cfg = dev->config;

	gpio_pin_interrupt_configure_dt(&cfg->int_gpio,
					enable ? GPIO_INT_EDGE_TO_ACTIVE
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

	if (!config->int_gpio.port) {
		return -ENOTSUP;
	}

	if (trig->type == SENSOR_TRIG_DATA_READY) {
		setup_drdy(dev, false);

		state = 0U;
		if (handler) {
			state = 1U;
		}

		data->handler_drdy = handler;
		data->trigger_drdy = trig;

		if (i2c_reg_update_byte_dt(&config->i2c, LSM9DS0_GYRO_REG_CTRL_REG3_G,
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

static void lsm9ds0_gyro_thread_main(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = p1;
	struct lsm9ds0_gyro_data *data = dev->data;

	while (1) {
		k_sem_take(&data->sem, K_FOREVER);

		if (data->handler_drdy) {
			data->handler_drdy(dev, data->trigger_drdy);
		}

		setup_drdy(dev, true);
	}
}

int lsm9ds0_gyro_init_interrupt(const struct device *dev)
{
	const struct lsm9ds0_gyro_config * const config =
					   dev->config;
	struct lsm9ds0_gyro_data *data = dev->data;

	data->dev = dev;
	k_sem_init(&data->sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&data->thread, data->thread_stack,
			CONFIG_LSM9DS0_GYRO_THREAD_STACK_SIZE,
			lsm9ds0_gyro_thread_main,
			(void *)dev, NULL, NULL, K_PRIO_COOP(10), 0, K_NO_WAIT);

	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("GPIO device not ready");
		return -ENODEV;
	}

	gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);

	gpio_init_callback(&data->gpio_cb,
			   lsm9ds0_gyro_gpio_drdy_callback,
			   BIT(config->int_gpio.pin));

	if (gpio_add_callback(config->int_gpio.port, &data->gpio_cb) < 0) {
		LOG_DBG("failed to set gpio callback");
		return -EINVAL;
	}

	return 0;
}
