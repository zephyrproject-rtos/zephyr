/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * Copyright (c) 2026 Giuseppe Fabiano <gfabiano40@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/byteorder.h>

#include "sx12xx_hal.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sx12xx_hal, CONFIG_LORA_LOG_LEVEL);

/*
 * How long to read BUSY back to back before sleeping on it, within the same
 * timeout. Every command waits for BUSY low twice, before it is sent and
 * after, and BUSY drops within a few hundred microseconds: on an SX1280
 * under traffic, a fifth of the waits found it high, nearly all for 10 to
 * 200 us and none for 500. A sleep costs at least a millisecond, so spinning
 * for less saves nothing.
 */
#define SX12XX_BUSY_SPIN_US 1000U

static int spi_transfer(const struct spi_dt_spec *spi,
			const uint8_t *hdr, size_t hdr_len,
			uint8_t *data, size_t data_len, bool read)
{
	uint8_t rx_hdr[4];

	__ASSERT_NO_MSG(hdr_len <= sizeof(rx_hdr));

	struct spi_buf tx_bufs[] = {
		{ .buf = (uint8_t *)hdr, .len = hdr_len },
		{ .buf = read ? NULL : data, .len = data_len },
	};
	struct spi_buf rx_bufs[] = {
		{ .buf = rx_hdr, .len = hdr_len },
		{ .buf = data, .len = data_len },
	};
	int count = (data_len == 0) ? 1 : 2;
	struct spi_buf_set tx_set = { .buffers = tx_bufs, .count = count };
	struct spi_buf_set rx_set = { .buffers = rx_bufs, .count = count };

	return spi_transceive_dt(spi, &tx_set, read ? &rx_set : NULL);
}

int sx12xx_hal_wait_busy(const struct device *dev, uint32_t timeout_ms)
{
	const struct sx12xx_hal_config *config = dev->config;
	uint32_t timeout_us = timeout_ms * USEC_PER_MSEC;
	uint32_t spin_us = MIN(timeout_us, SX12XX_BUSY_SPIN_US);

	if (WAIT_FOR(!config->is_busy(dev), spin_us, NULL) ||
	    WAIT_FOR(!config->is_busy(dev), timeout_us - spin_us, k_msleep(1))) {
		return 0;
	}

	LOG_WRN("Busy timeout after %u ms", timeout_ms);
	return -ETIMEDOUT;
}

void sx12xx_hal_set_rf_switch(const struct device *dev, bool enable, bool tx)
{
	const struct sx12xx_hal_config *config = dev->config;

	if (config->tx_enable.port != NULL) {
		gpio_pin_set_dt(&config->tx_enable, enable && tx);
	}

	if (config->rx_enable.port != NULL) {
		gpio_pin_set_dt(&config->rx_enable, enable && !tx);
	}
}

int sx12xx_hal_configure_gpio(const struct gpio_dt_spec *gpio,
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

static inline struct sx12xx_hal_data *hal_data(const struct device *dev)
{
	return dev->data;
}

void sx12xx_hal_init(const struct device *dev)
{
	k_mutex_init(&hal_data(dev)->bus_lock);
}

/*
 * The functions below hold bus_lock from the BUSY check to the end of the
 * transfer: the chip takes a command only while BUSY is low, and the IRQ
 * work and the caller's thread both talk to it. Without the lock, a command
 * sent right after the other thread's, past a BUSY check done before it, is
 * lost.
 */
int sx12xx_hal_wakeup(const struct device *dev)
{
	const struct sx12xx_hal_config *config = dev->config;
	uint8_t buf[2] = { SX12XX_CMD_GET_STATUS, 0x00 };
	struct spi_buf tx_buf = {
		.buf = buf,
		.len = sizeof(buf),
	};
	struct spi_buf_set tx_set = {
		.buffers = &tx_buf,
		.count = 1,
	};
	int ret;

	k_mutex_lock(&hal_data(dev)->bus_lock, K_FOREVER);

	/*
	 * Send a write-only GET_STATUS command. The NSS falling edge wakes
	 * the chip from sleep. Use spi_write_dt() (TX-only) rather than
	 * spi_transceive_dt() because the chip's SDO line is undefined
	 * during sleep mode.
	 */
	ret = spi_write_dt(&config->spi, &tx_set);
	if (ret < 0) {
		LOG_ERR("Wakeup SPI failed: %d", ret);
		goto out;
	}

	ret = sx12xx_hal_wait_busy(dev, SX12XX_BUSY_DEFAULT_TIMEOUT);
out:
	k_mutex_unlock(&hal_data(dev)->bus_lock);
	return ret;
}

int sx12xx_hal_write_cmd(const struct device *dev, uint8_t opcode,
			 const uint8_t *data, size_t len)
{
	const struct sx12xx_hal_config *config = dev->config;
	uint8_t hdr[1] = { opcode };
	int ret;

	k_mutex_lock(&hal_data(dev)->bus_lock, K_FOREVER);

	ret = sx12xx_hal_wait_busy(dev, SX12XX_BUSY_DEFAULT_TIMEOUT);
	if (ret < 0) {
		goto out;
	}

	ret = spi_transfer(&config->spi, hdr, sizeof(hdr), (uint8_t *)data, len, false);
	if (ret < 0) {
		LOG_ERR("SPI write failed: %d", ret);
		goto out;
	}

	if (opcode != SX12XX_CMD_SET_SLEEP) {
		ret = sx12xx_hal_wait_busy(dev, SX12XX_BUSY_DEFAULT_TIMEOUT);
	} else {
		/*
		 * The chip needs time to fully enter sleep mode before the
		 * next NSS falling edge can wake it up reliably.
		 */
		k_busy_wait(500);
	}

out:
	k_mutex_unlock(&hal_data(dev)->bus_lock);
	return ret;
}

int sx12xx_hal_read_cmd(const struct device *dev, uint8_t opcode,
			uint8_t *data, size_t len)
{
	const struct sx12xx_hal_config *config = dev->config;
	uint8_t hdr[2] = { opcode, 0x00 };
	int ret;

	k_mutex_lock(&hal_data(dev)->bus_lock, K_FOREVER);

	ret = sx12xx_hal_wait_busy(dev, SX12XX_BUSY_DEFAULT_TIMEOUT);
	if (ret < 0) {
		goto out;
	}

	ret = spi_transfer(&config->spi, hdr, sizeof(hdr), data, len, true);
	if (ret < 0) {
		LOG_ERR("SPI transceive failed: %d", ret);
		goto out;
	}

	ret = 0;
out:
	k_mutex_unlock(&hal_data(dev)->bus_lock);
	return ret;
}

int sx12xx_hal_write_regs(const struct device *dev, uint16_t address,
			  const uint8_t *data, size_t len)
{
	const struct sx12xx_hal_config *config = dev->config;
	uint8_t hdr[3];
	int ret;

	k_mutex_lock(&hal_data(dev)->bus_lock, K_FOREVER);

	ret = sx12xx_hal_wait_busy(dev, SX12XX_BUSY_DEFAULT_TIMEOUT);
	if (ret < 0) {
		goto out;
	}

	hdr[0] = config->opcodes->write_register;
	sys_put_be16(address, &hdr[1]);

	ret = spi_transfer(&config->spi, hdr, sizeof(hdr), (uint8_t *)data, len, false);
	if (ret < 0) {
		LOG_ERR("SPI write regs failed: %d", ret);
		goto out;
	}

	ret = sx12xx_hal_wait_busy(dev, SX12XX_BUSY_DEFAULT_TIMEOUT);
out:
	k_mutex_unlock(&hal_data(dev)->bus_lock);
	return ret;
}

int sx12xx_hal_read_regs(const struct device *dev, uint16_t address,
			 uint8_t *data, size_t len)
{
	const struct sx12xx_hal_config *config = dev->config;
	uint8_t hdr[4];
	int ret;

	k_mutex_lock(&hal_data(dev)->bus_lock, K_FOREVER);

	ret = sx12xx_hal_wait_busy(dev, SX12XX_BUSY_DEFAULT_TIMEOUT);
	if (ret < 0) {
		goto out;
	}

	hdr[0] = config->opcodes->read_register;
	sys_put_be16(address, &hdr[1]);
	hdr[3] = 0x00;

	ret = spi_transfer(&config->spi, hdr, sizeof(hdr), data, len, true);
	if (ret < 0) {
		LOG_ERR("SPI read regs failed: %d", ret);
		goto out;
	}

	ret = 0;
out:
	k_mutex_unlock(&hal_data(dev)->bus_lock);
	return ret;
}

int sx12xx_hal_write_buffer(const struct device *dev, uint8_t offset,
			    const uint8_t *data, size_t len)
{
	const struct sx12xx_hal_config *config = dev->config;
	uint8_t hdr[2] = { config->opcodes->write_buffer, offset };
	int ret;

	k_mutex_lock(&hal_data(dev)->bus_lock, K_FOREVER);

	ret = sx12xx_hal_wait_busy(dev, SX12XX_BUSY_DEFAULT_TIMEOUT);
	if (ret < 0) {
		goto out;
	}

	ret = spi_transfer(&config->spi, hdr, sizeof(hdr), (uint8_t *)data, len, false);
	if (ret < 0) {
		LOG_ERR("SPI write buffer failed: %d", ret);
		goto out;
	}

	ret = sx12xx_hal_wait_busy(dev, SX12XX_BUSY_DEFAULT_TIMEOUT);
out:
	k_mutex_unlock(&hal_data(dev)->bus_lock);
	return ret;
}

int sx12xx_hal_read_buffer(const struct device *dev, uint8_t offset,
			   uint8_t *data, size_t len)
{
	const struct sx12xx_hal_config *config = dev->config;
	uint8_t hdr[3] = { config->opcodes->read_buffer, offset, 0x00 };
	int ret;

	k_mutex_lock(&hal_data(dev)->bus_lock, K_FOREVER);

	ret = sx12xx_hal_wait_busy(dev, SX12XX_BUSY_DEFAULT_TIMEOUT);
	if (ret < 0) {
		goto out;
	}

	ret = spi_transfer(&config->spi, hdr, sizeof(hdr), data, len, true);
	if (ret < 0) {
		LOG_ERR("SPI read buffer failed: %d", ret);
		goto out;
	}

	ret = 0;
out:
	k_mutex_unlock(&hal_data(dev)->bus_lock);
	return ret;
}
