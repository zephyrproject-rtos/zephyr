/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/byteorder.h>

#include "sx126x.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sx126x_hal_common, CONFIG_LORA_LOG_LEVEL);

#define SX126X_BUSY_DEFAULT_TIMEOUT 1000

static int spi_transfer(const struct spi_dt_spec *spi,
			const uint8_t *hdr, size_t hdr_len,
			uint8_t *data, size_t data_len, bool read)
{
	uint8_t rx_hdr[4];

	__ASSERT_NO_MSG(hdr_len <= sizeof(rx_hdr));

	struct spi_buf tx_bufs[] = {
		{ .buf = (uint8_t *)hdr, .len = hdr_len },
		{ .buf = data, .len = data_len },
	};
	struct spi_buf rx_bufs[] = {
		{ .buf = rx_hdr, .len = hdr_len },
		{ .buf = data, .len = data_len },
	};
	struct spi_buf_set tx_set = {
		.buffers = tx_bufs,
		.count = (read || (data_len == 0)) ? 1 : 2,
	};
	struct spi_buf_set rx_set = {
		.buffers = rx_bufs,
		.count = read ? 2 : 1,
	};

	return spi_transceive_dt(spi, &tx_set, &rx_set);
}

int sx126x_hal_wait_busy(const struct device *dev, uint32_t timeout_ms)
{
	if (!WAIT_FOR(!sx126x_hal_is_busy(dev),
		      timeout_ms * 1000,
		      k_msleep(1))) {
		LOG_WRN("Busy timeout after %u ms", timeout_ms);
		return -ETIMEDOUT;
	}

	return 0;
}

void sx126x_hal_set_antenna_enable(const struct device *dev, bool enable)
{
	const struct sx126x_hal_config *config = dev->config;

	if (config->antenna_enable.port != NULL) {
		gpio_pin_set_dt(&config->antenna_enable, enable);
	}
}

void sx126x_hal_set_rf_switch(const struct device *dev, bool tx)
{
	const struct sx126x_hal_config *config = dev->config;

	if (config->tx_enable.port != NULL) {
		gpio_pin_set_dt(&config->tx_enable, tx);
	}

	if (config->rx_enable.port != NULL) {
		gpio_pin_set_dt(&config->rx_enable, !tx);
	}
}

int sx126x_hal_configure_gpio(const struct gpio_dt_spec *gpio,
			      gpio_flags_t flags, const char *name)
{
	int ret;

	if (gpio->port == NULL) {
		return 0;
	}

	if (!gpio_is_ready_dt(gpio)) {
		LOG_ERR("%s GPIO not ready", name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(gpio, flags);
	if (ret < 0) {
		LOG_ERR("Failed to configure %s: %d", name, ret);
		return ret;
	}

	return 0;
}

int sx126x_hal_write_cmd(const struct device *dev, uint8_t opcode,
			 const uint8_t *data, size_t len)
{
	const struct sx126x_hal_config *config = dev->config;
	uint8_t hdr[1] = { opcode };
	int ret;

	ret = sx126x_hal_wait_busy(dev, SX126X_BUSY_DEFAULT_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	ret = spi_transfer(&config->spi, hdr, sizeof(hdr), (uint8_t *)data, len, false);
	if (ret < 0) {
		LOG_ERR("SPI write failed: %d", ret);
		return ret;
	}

	if (opcode != SX126X_CMD_SET_SLEEP) {
		ret = sx126x_hal_wait_busy(dev, SX126X_BUSY_DEFAULT_TIMEOUT);
	}

	return ret;
}

int sx126x_hal_read_cmd(const struct device *dev, uint8_t opcode,
			uint8_t *data, size_t len)
{
	const struct sx126x_hal_config *config = dev->config;
	uint8_t hdr[2] = { opcode, 0x00 };
	int ret;

	ret = sx126x_hal_wait_busy(dev, SX126X_BUSY_DEFAULT_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	ret = spi_transfer(&config->spi, hdr, sizeof(hdr), data, len, true);
	if (ret < 0) {
		LOG_ERR("SPI transceive failed: %d", ret);
		return ret;
	}

	return 0;
}

int sx126x_hal_write_regs(const struct device *dev, uint16_t address,
			  const uint8_t *data, size_t len)
{
	const struct sx126x_hal_config *config = dev->config;
	uint8_t hdr[3];
	int ret;

	ret = sx126x_hal_wait_busy(dev, SX126X_BUSY_DEFAULT_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	hdr[0] = SX126X_CMD_WRITE_REGISTER;
	sys_put_be16(address, &hdr[1]);

	ret = spi_transfer(&config->spi, hdr, sizeof(hdr), (uint8_t *)data, len, false);
	if (ret < 0) {
		LOG_ERR("SPI write regs failed: %d", ret);
		return ret;
	}

	return sx126x_hal_wait_busy(dev, SX126X_BUSY_DEFAULT_TIMEOUT);
}

int sx126x_hal_read_regs(const struct device *dev, uint16_t address,
			 uint8_t *data, size_t len)
{
	const struct sx126x_hal_config *config = dev->config;
	uint8_t hdr[4];
	int ret;

	ret = sx126x_hal_wait_busy(dev, SX126X_BUSY_DEFAULT_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	hdr[0] = SX126X_CMD_READ_REGISTER;
	sys_put_be16(address, &hdr[1]);
	hdr[3] = 0x00;

	ret = spi_transfer(&config->spi, hdr, sizeof(hdr), data, len, true);
	if (ret < 0) {
		LOG_ERR("SPI read regs failed: %d", ret);
		return ret;
	}

	return 0;
}

int sx126x_hal_write_buffer(const struct device *dev, uint8_t offset,
			    const uint8_t *data, size_t len)
{
	const struct sx126x_hal_config *config = dev->config;
	uint8_t hdr[2] = { SX126X_CMD_WRITE_BUFFER, offset };
	int ret;

	ret = sx126x_hal_wait_busy(dev, SX126X_BUSY_DEFAULT_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	ret = spi_transfer(&config->spi, hdr, sizeof(hdr), (uint8_t *)data, len, false);
	if (ret < 0) {
		LOG_ERR("SPI write buffer failed: %d", ret);
		return ret;
	}

	return sx126x_hal_wait_busy(dev, SX126X_BUSY_DEFAULT_TIMEOUT);
}

int sx126x_hal_read_buffer(const struct device *dev, uint8_t offset,
			   uint8_t *data, size_t len)
{
	const struct sx126x_hal_config *config = dev->config;
	uint8_t hdr[3] = { SX126X_CMD_READ_BUFFER, offset, 0x00 };
	int ret;

	ret = sx126x_hal_wait_busy(dev, SX126X_BUSY_DEFAULT_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	ret = spi_transfer(&config->spi, hdr, sizeof(hdr), data, len, true);
	if (ret < 0) {
		LOG_ERR("SPI read buffer failed: %d", ret);
		return ret;
	}

	return 0;
}
