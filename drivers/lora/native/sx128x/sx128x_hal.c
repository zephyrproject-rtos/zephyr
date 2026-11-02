/*
 * Copyright (c) 2026 Giuseppe Fabiano <gfabiano40@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * SX128X pins: reset, BUSY and the optional IRQ pin. The SPI transactions
 * are the common SX12xx ones (sx12xx_hal.c).
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>

#include "sx128x.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sx128x_hal, CONFIG_LORA_LOG_LEVEL);

#define SX128X_RESET_PULSE_MS       10
#define SX128X_RESET_WAIT_MS        10

static inline struct sx128x_hal_data *get_hal_data(const struct device *dev)
{
	struct sx128x_data *data = dev->data;

	return &data->hal;
}

bool sx128x_hal_is_busy(const struct device *dev)
{
	const struct sx128x_hal_config *config = dev->config;

	return gpio_pin_get_dt(&config->busy) != 0;
}

int sx128x_hal_reset(const struct device *dev)
{
	const struct sx128x_hal_config *config = dev->config;
	int ret;

	if (config->reset.port == NULL) {
		/* NRESET has an internal pull-up; rely on power-on reset. */
		k_msleep(SX128X_RESET_WAIT_MS);
		return sx12xx_hal_wait_busy(dev, SX12XX_BUSY_DEFAULT_TIMEOUT);
	}

	ret = gpio_pin_set_dt(&config->reset, 1);
	if (ret < 0) {
		LOG_ERR("Failed to assert reset: %d", ret);
		return ret;
	}

	k_msleep(SX128X_RESET_PULSE_MS);

	ret = gpio_pin_set_dt(&config->reset, 0);
	if (ret < 0) {
		LOG_ERR("Failed to release reset: %d", ret);
		return ret;
	}

	k_msleep(SX128X_RESET_WAIT_MS);

	ret = sx12xx_hal_wait_busy(dev, SX12XX_BUSY_DEFAULT_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("Reset complete");
	return 0;
}

static void irq_isr(const struct device *gpio, struct gpio_callback *cb, uint32_t pins)
{
	struct sx128x_hal_data *data = CONTAINER_OF(cb, struct sx128x_hal_data, irq_cb);

	if (data->irq_callback != NULL) {
		data->irq_callback(data->dev);
	}
}

int sx128x_hal_set_irq_callback(const struct device *dev,
				void (*callback)(const struct device *dev))
{
	const struct sx128x_hal_config *config = dev->config;
	struct sx128x_hal_data *data = get_hal_data(dev);
	int ret;

	data->irq_callback = callback;
	if (config->irq.port == NULL) {
		return 0;
	}

	if (callback != NULL) {
		ret = gpio_pin_interrupt_configure_dt(&config->irq, GPIO_INT_EDGE_TO_ACTIVE);
	} else {
		ret = gpio_pin_interrupt_configure_dt(&config->irq, GPIO_INT_DISABLE);
	}

	return ret;
}

int sx128x_hal_irq_active(const struct device *dev)
{
	const struct sx128x_hal_config *config = dev->config;

	if (config->irq.port == NULL) {
		return 0;
	}
	return gpio_pin_get_dt(&config->irq);
}

int sx128x_hal_init(const struct device *dev)
{
	const struct sx128x_hal_config *config = dev->config;
	struct sx128x_hal_data *data = get_hal_data(dev);
	int ret;

	data->dev = dev;
	data->irq_callback = NULL;
	sx12xx_hal_init(dev);

	if (!spi_is_ready_dt(&config->common.spi)) {
		LOG_ERR("SPI bus not ready");
		return -ENODEV;
	}

	ret = sx12xx_hal_configure_gpio(&config->reset, GPIO_OUTPUT_INACTIVE, "reset");
	if (ret < 0) {
		return ret;
	}

	if (!gpio_is_ready_dt(&config->busy)) {
		LOG_ERR("Busy GPIO not ready");
		return -ENODEV;
	}
	ret = gpio_pin_configure_dt(&config->busy, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure busy GPIO: %d", ret);
		return ret;
	}

	ret = sx12xx_hal_configure_gpio(&config->irq, GPIO_INPUT, "IRQ pin");
	if (ret < 0) {
		return ret;
	}

	if (config->irq.port != NULL) {
		gpio_init_callback(&data->irq_cb, irq_isr, BIT(config->irq.pin));
		ret = gpio_add_callback(config->irq.port, &data->irq_cb);
		if (ret < 0) {
			LOG_ERR("Failed to add IRQ pin callback: %d", ret);
			return ret;
		}
	}

	ret = sx12xx_hal_configure_gpio(&config->common.tx_enable, GPIO_OUTPUT_INACTIVE,
					"TX enable");
	if (ret < 0) {
		return ret;
	}

	ret = sx12xx_hal_configure_gpio(&config->common.rx_enable, GPIO_OUTPUT_INACTIVE,
					"RX enable");
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("HAL initialized");
	return 0;
}
