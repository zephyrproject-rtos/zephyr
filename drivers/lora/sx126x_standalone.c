/*
 * Copyright (c) 2019 Manivannan Sadhasivam
 * Copyright (c) 2020 Andreas Sandberg
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/gpio.h>
#include <zephyr.h>

#include "sx126x_common.h"

#include <logging/log.h>
LOG_MODULE_DECLARE(sx126x, CONFIG_LORA_LOG_LEVEL);

void sx126x_reset(const struct device *dev)
{
	const struct sx126x_config *cfg = dev->config;

	gpio_pin_set_dt(&cfg->reset, 1);
	k_sleep(K_MSEC(20));
	gpio_pin_set_dt(&cfg->reset, 0);
	k_sleep(K_MSEC(10));
}

bool sx126x_is_busy(const struct device *dev)
{
	const struct sx126x_config *cfg = dev->config;

	return gpio_pin_get_dt(&cfg->busy);
}

uint32_t sx126x_get_dio1_pin_state(const struct device *dev)
{
	const struct sx126x_config *cfg = dev->config;

	return gpio_pin_get_dt(&cfg->dio1) > 0 ? 1U : 0U;
}

void sx126x_dio1_irq_enable(const struct device *dev)
{
	const struct sx126x_config *cfg = dev->config;

	gpio_pin_interrupt_configure_dt(&cfg->dio1,
				     GPIO_INT_EDGE_TO_ACTIVE);
}

void sx126x_dio1_irq_disable(const struct device *dev)
{
	const struct sx126x_config *cfg = dev->config;

	gpio_pin_interrupt_configure_dt(&cfg->dio1,
				     GPIO_INT_DISABLE);
}

static void sx126x_dio1_irq_callback(const struct device *dev,
				     struct gpio_callback *cb, uint32_t pins)
{
	struct sx126x_data *dev_data = CONTAINER_OF(cb, struct sx126x_data,
						    dio1_irq_callback);

	k_work_submit(&dev_data->dio1_irq_work);
}

int sx126x_variant_init(const struct device *dev)
{
	const struct sx126x_config *cfg = dev->config;
	struct sx126x_data *dev_data = dev->data;

	if (gpio_pin_configure_dt(&cfg->reset, GPIO_OUTPUT_ACTIVE) ||
	    gpio_pin_configure_dt(&cfg->busy, GPIO_INPUT) ||
	    gpio_pin_configure_dt(&cfg->dio1, GPIO_INPUT | GPIO_INT_DEBOUNCE)) {
		LOG_ERR("GPIO configuration failed.");
		return -EIO;
	}

	gpio_init_callback(&dev_data->dio1_irq_callback,
			   sx126x_dio1_irq_callback, BIT(cfg->dio1.pin));
	if (gpio_add_callback(cfg->dio1.port,
			      &dev_data->dio1_irq_callback) < 0) {
		LOG_ERR("Could not set GPIO callback for DIO1 interrupt.");
		return -EIO;
	}

	return 0;
}
