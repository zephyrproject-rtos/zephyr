/*
 * Copyright (c) 2019 Manivannan Sadhasivam
 * Copyright (c) 2020 Andreas Sandberg
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#include "sx126x_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(sx126x, CONFIG_LORA_LOG_LEVEL);

static const struct gpio_dt_spec sx126x_gpio_reset = GPIO_DT_SPEC_INST_GET(
		0, reset_gpios);
static const struct gpio_dt_spec sx126x_gpio_busy = GPIO_DT_SPEC_INST_GET(
		0, busy_gpios);
static const struct gpio_dt_spec sx126x_gpio_dio1 = GPIO_DT_SPEC_INST_GET(
		0, dio1_gpios);

void sx126x_reset(struct sx126x_data *dev_data)
{
	gpio_pin_set_dt(&sx126x_gpio_reset, 1);
	k_sleep(K_MSEC(20));
	gpio_pin_set_dt(&sx126x_gpio_reset, 0);
	k_sleep(K_MSEC(10));
}

bool sx126x_is_busy(struct sx126x_data *dev_data)
{
	return gpio_pin_get_dt(&sx126x_gpio_busy);
}

uint32_t sx126x_get_dio1_pin_state(struct sx126x_data *dev_data)
{
	return gpio_pin_get_dt(&sx126x_gpio_dio1) > 0 ? 1U : 0U;
}

void sx126x_dio1_irq_enable(struct sx126x_data *dev_data)
{
	gpio_pin_interrupt_configure_dt(&sx126x_gpio_dio1,
				     GPIO_INT_EDGE_TO_ACTIVE);
}

void sx126x_dio1_irq_disable(struct sx126x_data *dev_data)
{
	gpio_pin_interrupt_configure_dt(&sx126x_gpio_dio1,
				     GPIO_INT_DISABLE);
}

static void sx126x_dio1_irq_callback(const struct device *dev,
				     struct gpio_callback *cb, uint32_t pins)
{
	struct sx126x_data *dev_data = CONTAINER_OF(cb, struct sx126x_data,
						    dio1_irq_callback);

	if (pins & BIT(sx126x_gpio_dio1.pin)) {
		k_work_submit(&dev_data->dio1_irq_work);
	}
}

void sx126x_set_tx_params(int8_t power, RadioRampTimes_t ramp_time)
{
	SX126xSetTxParams(power, ramp_time);
}

int sx126x_variant_init(const struct device *dev)
{
	struct sx126x_data *dev_data = dev->data;

	if (gpio_pin_configure_dt(&sx126x_gpio_reset, GPIO_OUTPUT_ACTIVE) ||
	    gpio_pin_configure_dt(&sx126x_gpio_busy, GPIO_INPUT) ||
	    gpio_pin_configure_dt(&sx126x_gpio_dio1, GPIO_INPUT)) {
		LOG_ERR("GPIO configuration failed.");
		return -EIO;
	}

	gpio_init_callback(&dev_data->dio1_irq_callback,
			   sx126x_dio1_irq_callback, BIT(sx126x_gpio_dio1.pin));
	if (gpio_add_callback(sx126x_gpio_dio1.port,
			      &dev_data->dio1_irq_callback) < 0) {
		LOG_ERR("Could not set GPIO callback for DIO1 interrupt.");
		return -EIO;
	}

	return 0;
}
