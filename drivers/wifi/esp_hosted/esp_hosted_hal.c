/*
 * Copyright (c) 2025 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include "esp_hosted_wifi.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(esp_hosted_hal, CONFIG_WIFI_LOG_LEVEL);

int esp_hosted_hal_init(const struct device *dev)
{
	const esp_hosted_config_t *config = dev->config;

	if (!spi_is_ready_dt(&config->spi_bus)) {
		LOG_ERR("SPI device is not ready");
		return -ENODEV;
	}

	/* Configure pins. */
	gpio_pin_configure_dt(&config->dataready_gpio, GPIO_OUTPUT);
	gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT);

	/* Perform a hard-reset */
	gpio_pin_set_dt(&config->dataready_gpio, 1);
	gpio_pin_set_dt(&config->reset_gpio, 0);
	k_msleep(100);
	gpio_pin_set_dt(&config->reset_gpio, 1);
	k_msleep(500);

	/* Configure handshake/dataready pins. */
	gpio_pin_configure_dt(&config->dataready_gpio, GPIO_INPUT);
	gpio_pin_configure_dt(&config->handshake_gpio, GPIO_INPUT);

	return 0;
}

bool esp_hosted_hal_data_ready(const struct device *dev)
{
	const esp_hosted_config_t *config = dev->config;

	return gpio_pin_get_dt(&config->dataready_gpio);
}

int esp_hosted_hal_spi_transfer(const struct device *dev, void *tx, void *rx, uint32_t size)
{
	int ret = 0;
	esp_hosted_data_t *data = dev->data;
	const esp_hosted_config_t *config = dev->config;

	const struct spi_buf tx_buf = {.buf = tx ? tx : rx, .len = size};
	const struct spi_buf_set tx_set = {.buffers = &tx_buf, .count = 1};

	const struct spi_buf rx_buf = {.buf = rx ? rx : tx, .len = size};
	const struct spi_buf_set rx_set = {.buffers = &rx_buf, .count = 1};

	/* Wait for handshake pin to go high. */
	for (uint64_t start = k_uptime_get();; k_msleep(1)) {
		if (gpio_pin_get_dt(&config->handshake_gpio) &&
		    (rx == NULL || gpio_pin_get_dt(&config->dataready_gpio))) {
			break;
		}
		if ((k_uptime_get() - start) >= 100) {
			return -ETIMEDOUT;
		}
	}

	if (k_sem_take(&data->bus_sem, K_FOREVER) != 0) {
		return -1;
	}

	/* Transfer SPI buffers. */
	if (spi_transceive_dt(&config->spi_bus, &tx_set, &rx_set)) {
		LOG_ERR("spi_transceive failed");
		ret = -EIO;
	}

	k_sem_give(&data->bus_sem);
	return ret;
}
