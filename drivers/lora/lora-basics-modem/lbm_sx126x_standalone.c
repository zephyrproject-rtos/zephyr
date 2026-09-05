/*
 * Copyright (c) 2025 Embeint Inc
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Board glue for a discrete SX126x, where reset, busy and the radio interrupt
 * are ordinary GPIOs.
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "lbm_sx126x_common.h"

LOG_MODULE_DECLARE(lbm_driver, CONFIG_LORA_LOG_LEVEL);

int lbm_sx126x_pins_init(const struct device *dev)
{
	const struct lbm_sx126x_config *config = dev->config;

	if (gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT_INACTIVE) ||
	    gpio_pin_configure_dt(&config->busy, GPIO_INPUT) ||
	    gpio_pin_configure_dt(&config->lbm_common.dio1, GPIO_INPUT)) {
		LOG_ERR("Could not configure the reset, busy and DIO1 pins.");
		return -EIO;
	}

	return 0;
}

void lbm_sx126x_reset(const struct device *dev)
{
	const struct lbm_sx126x_config *config = dev->config;

	gpio_pin_set_dt(&config->reset, 1);
	k_sleep(K_MSEC(20));
	gpio_pin_set_dt(&config->reset, 0);
	k_sleep(K_MSEC(10));
}

bool lbm_sx126x_is_busy(const struct device *dev)
{
	const struct lbm_sx126x_config *config = dev->config;

	return gpio_pin_get_dt(&config->busy);
}

void lbm_driver_dio1_irq_enable(const struct device *dev)
{
	const struct lbm_sx126x_config *config = dev->config;

	(void)gpio_pin_interrupt_configure_dt(&config->lbm_common.dio1, GPIO_INT_EDGE_TO_ACTIVE);
}

void lbm_driver_dio1_irq_disable(const struct device *dev)
{
	const struct lbm_sx126x_config *config = dev->config;

	(void)gpio_pin_interrupt_configure_dt(&config->lbm_common.dio1, GPIO_INT_DISABLE);
}

static void sx126x_dio1_callback(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	struct lbm_sx126x_data *data = CONTAINER_OF(cb, struct lbm_sx126x_data, dio1_callback);

	LOG_DBG("");
	/* Submit work to process the interrupt immediately */
	k_work_schedule(&data->lbm_common.op_done_work, K_NO_WAIT);
}

int lbm_sx126x_variant_init(const struct device *dev)
{
	const struct lbm_sx126x_config *config = dev->config;
	struct lbm_sx126x_data *data = dev->data;

	gpio_init_callback(&data->dio1_callback, sx126x_dio1_callback,
			   BIT(config->lbm_common.dio1.pin));
	if (gpio_add_callback(config->lbm_common.dio1.port, &data->dio1_callback) < 0) {
		LOG_ERR("Could not set GPIO callback for DIO1 interrupt.");
		return -EIO;
	}

	lbm_driver_dio1_irq_enable(dev);

	return 0;
}

int lbm_driver_add_dio1_gpio_callback(const struct device *dev, struct gpio_callback *callback,
				      gpio_callback_handler_t handler)
{
	const struct lbm_sx126x_config *config = dev->config;
	int ret;

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	if (callback == NULL || handler == NULL) {
		return -EINVAL;
	}

	gpio_init_callback(callback, handler, BIT(config->lbm_common.dio1.pin));

	ret = gpio_add_callback(config->lbm_common.dio1.port, callback);
	if (ret < 0) {
		LOG_ERR("Failed to add GPIO callback: %d", ret);
		return ret;
	}

	lbm_driver_dio1_irq_enable(dev);

	LOG_DBG("Added user GPIO callback");
	return 0;
}

int lbm_driver_remove_dio1_gpio_callback(const struct device *dev, struct gpio_callback *callback)
{
	const struct lbm_sx126x_config *config = dev->config;
	int ret;

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	if (callback == NULL) {
		return -EINVAL;
	}

	ret = gpio_remove_callback(config->lbm_common.dio1.port, callback);
	if (ret < 0) {
		LOG_ERR("Failed to remove GPIO callback: %d", ret);
		return ret;
	}

	LOG_DBG("Removed user GPIO callback");
	return 0;
}
