/*
 * Trigger code for the MAX32664C biometric sensor hub.
 *
 * Copyright (c) 2025, Daniel Kampert
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "max32664c.h"

LOG_MODULE_REGISTER(maxim_max32664c_interrupt, CONFIG_SENSOR_LOG_LEVEL);

#ifdef CONFIG_MAX32664C_USE_INTERRUPT
static void max32664c_interrupt_worker(struct k_work *p_work)
{
	struct max32664c_data *data = CONTAINER_OF(p_work, struct max32664c_data, interrupt_work);

	/* TODO */
}

static void max32664c_gpio_callback_handler(const struct device *p_port, struct gpio_callback *p_cb,
					   gpio_port_pins_t pins)
{
	ARG_UNUSED(pins);
	ARG_UNUSED(p_port);

	struct max32664c_data *data = CONTAINER_OF(p_cb, struct max32664c_data, gpio_cb);

	k_work_submit(&data->interrupt_work);
}

int max32664c_init_interrupt(const struct device *dev)
{
	LOG_DBG("\tUsing MFIO interrupt mode");

	int err;
	uint8_t tx[2];
	uint8_t rx;
	struct max32664c_data *data = dev->data;
	const struct max32664c_config *config = dev->config;

	LOG_DBG("Configure interrupt pin");
	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("GPIO not ready!");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (err < 0) {
		LOG_ERR("Failed to configure GPIO! Error: %u", err);
		return err;
	}

	err = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_FALLING);
	if (err < 0) {
		LOG_ERR("Failed to configure interrupt! Error: %u", err);
		return err;
	}

	gpio_init_callback(&data->gpio_cb, max32664c_gpio_callback_handler,
			   BIT(config->int_gpio.pin));

	err = gpio_add_callback_dt(&config->int_gpio, &data->gpio_cb);
	if (err < 0) {
		LOG_ERR("Failed to add GPIO callback! Error: %u", err);
		return err;
	}

	data->interrupt_work.handler = max32664c_interrupt_worker;

	tx[0] = 0xB8;
	tx[1] = 0x01;
	if (max32664c_i2c_transmit(dev, tx, 2, &rx, 1, MAX32664C_DEFAULT_CMD_DELAY)) {
		LOG_ERR("Can not enable interrupt mode!");
		return -EINVAL;
	}

	return 0;
}
#endif /* CONFIG_MAX32664C_USE_INTERRUPT */
