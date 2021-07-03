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

#define GPIO_RESET_PIN		DT_INST_GPIO_PIN(0, reset_gpios)
#define GPIO_BUSY_PIN		DT_INST_GPIO_PIN(0, busy_gpios)
#define GPIO_DIO1_PIN		DT_INST_GPIO_PIN(0, dio1_gpios)

void sx126x_reset(struct sx126x_data *dev_data)
{
	gpio_pin_set(dev_data->reset, GPIO_RESET_PIN, 1);
	k_sleep(K_MSEC(20));
	gpio_pin_set(dev_data->reset, GPIO_RESET_PIN, 0);
	k_sleep(K_MSEC(10));
}

bool sx126x_is_busy(struct sx126x_data *dev_data)
{
	return gpio_pin_get(dev_data->busy, GPIO_BUSY_PIN);
}

uint32_t sx126x_get_dio1_pin_state(struct sx126x_data *dev_data)
{
	return gpio_pin_get(dev_data->dio1, GPIO_DIO1_PIN) > 0 ? 1U : 0U;
}

void sx126x_dio1_irq_enable(struct sx126x_data *dev_data)
{
	gpio_pin_interrupt_configure(dev_data->dio1, GPIO_DIO1_PIN,
				     GPIO_INT_EDGE_TO_ACTIVE);
}

void sx126x_dio1_irq_disable(struct sx126x_data *dev_data)
{
	gpio_pin_interrupt_configure(dev_data->dio1, GPIO_DIO1_PIN,
				     GPIO_INT_DISABLE);
}

static void sx126x_dio1_irq_callback(const struct device *dev,
				     struct gpio_callback *cb, uint32_t pins)
{
	struct sx126x_data *dev_data = dev->data;

	if (pins & BIT(GPIO_DIO1_PIN)) {
		k_work_submit(&dev_data->dio1_irq_work);
	}
}

int sx126x_variant_init(const struct device *dev)
{
	struct sx126x_data *dev_data = dev->data;

	gpio_init_callback(&dev_data->dio1_irq_callback,
			   sx126x_dio1_irq_callback, BIT(GPIO_DIO1_PIN));
	if (gpio_add_callback(dev_data->dio1, &dev_data->dio1_irq_callback) < 0) {
		LOG_ERR("Could not set GPIO callback for DIO1 interrupt.");
		return -EIO;
	}

	return 0;
}
