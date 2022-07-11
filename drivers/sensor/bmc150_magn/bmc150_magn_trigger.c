/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>

#include "bmc150_magn.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(BMC150_MAGN, CONFIG_SENSOR_LOG_LEVEL);

static inline void setup_drdy(const struct device *dev,
			      bool enable)
{
	const struct bmc150_magn_config *const cfg =
		dev->config;

	gpio_pin_interrupt_configure_dt(&cfg->int_gpio,
					enable ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE);
}


int bmc150_magn_trigger_set(const struct device *dev,
			    const struct sensor_trigger *trig,
			    sensor_trigger_handler_t handler)
{
	struct bmc150_magn_data *data = dev->data;
	const struct bmc150_magn_config * const config =
					dev->config;
	uint8_t state;

	if (!config->int_gpio.port) {
		return -ENOTSUP;
	}

#if defined(CONFIG_BMC150_MAGN_TRIGGER_DRDY)
	if (trig->type == SENSOR_TRIG_DATA_READY) {
		setup_drdy(dev, false);

		state = 0U;
		if (handler) {
			state = 1U;
		}

		data->handler_drdy = handler;
		data->trigger_drdy = *trig;

		if (i2c_reg_update_byte_dt(&config->i2c,
					   BMC150_MAGN_REG_INT_DRDY,
					   BMC150_MAGN_MASK_DRDY_EN,
					   state << BMC150_MAGN_SHIFT_DRDY_EN)
					   < 0) {
			LOG_DBG("failed to set DRDY interrupt");
			return -EIO;
		}

		setup_drdy(dev, true);
	}
#endif

	return 0;
}

static void bmc150_magn_gpio_drdy_callback(const struct device *dev,
					   struct gpio_callback *cb,
					   uint32_t pins)
{
	struct bmc150_magn_data *data =
		CONTAINER_OF(cb, struct bmc150_magn_data, gpio_cb);

	ARG_UNUSED(pins);

	setup_drdy(data->dev, false);

	k_sem_give(&data->sem);
}

static void bmc150_magn_thread_main(struct bmc150_magn_data *data)
{
	const struct bmc150_magn_config *config = data->dev->config;
	uint8_t reg_val;

	while (1) {
		k_sem_take(&data->sem, K_FOREVER);

		while (i2c_reg_read_byte_dt(&config->i2c,
					    BMC150_MAGN_REG_INT_STATUS,
					    &reg_val) < 0) {
			LOG_DBG("failed to clear data ready interrupt");
		}

		if (data->handler_drdy) {
			data->handler_drdy(data->dev, &data->trigger_drdy);
		}

		setup_drdy(data->dev, true);
	}
}

static int bmc150_magn_set_drdy_polarity(const struct device *dev, int state)
{
	const struct bmc150_magn_config *config = dev->config;

	if (state) {
		state = 1;
	}

	return i2c_reg_update_byte_dt(&config->i2c,
				      BMC150_MAGN_REG_INT_DRDY,
				      BMC150_MAGN_MASK_DRDY_DR_POLARITY,
				      state << BMC150_MAGN_SHIFT_DRDY_DR_POLARITY);
}

int bmc150_magn_init_interrupt(const struct device *dev)
{
	const struct bmc150_magn_config * const config =
						dev->config;
	struct bmc150_magn_data *data = dev->data;


#if defined(CONFIG_BMC150_MAGN_TRIGGER_DRDY)
	if (bmc150_magn_set_drdy_polarity(dev, 0) < 0) {
		LOG_DBG("failed to set DR polarity");
		return -EIO;
	}

	if (i2c_reg_update_byte_dt(&config->i2c,
				   BMC150_MAGN_REG_INT_DRDY,
				   BMC150_MAGN_MASK_DRDY_EN,
				   0 << BMC150_MAGN_SHIFT_DRDY_EN) < 0) {
		LOG_DBG("failed to set data ready interrupt enabled bit");
		return -EIO;
	}
#endif

	data->handler_drdy = NULL;

	k_sem_init(&data->sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&data->thread, data->thread_stack,
			CONFIG_BMC150_MAGN_TRIGGER_THREAD_STACK,
			(k_thread_entry_t)bmc150_magn_thread_main,
			data, NULL, NULL,
			K_PRIO_COOP(10), 0, K_NO_WAIT);

	if (!device_is_ready(config->int_gpio.port)) {
		LOG_ERR("GPIO device not ready");
		return -ENODEV;
	}

	gpio_pin_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);

	gpio_init_callback(&data->gpio_cb,
			   bmc150_magn_gpio_drdy_callback,
			   BIT(config->int_gpio.pin));

	if (gpio_add_callback(config->int_gpio.port, &data->gpio_cb) < 0) {
		LOG_DBG("failed to set gpio callback");
		return -EIO;
	}

	data->dev = dev;

	return 0;
}
