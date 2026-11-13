/*
 * Copyright (c) 2025 Pedro Ramos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "atm90e26.h"

LOG_MODULE_DECLARE(atm90e26, CONFIG_ENERGY_METER_LOG_LEVEL);

static int atm90e26_spi_check(const struct device *dev)
{
	const struct atm90e26_config *cfg = dev->config;

	if (!spi_is_ready_dt(&cfg->bus.spi)) {
		LOG_ERR("SPI bus not ready");
		return -ENODEV;
	}

	return 0;
}

static int atm90e26_spi_read(const struct device *dev, uint8_t addr, uint16_t *val)
{
	const struct atm90e26_config *cfg = dev->config;
	uint8_t tx[3] = {ATM90E26_READ | (addr & ATM90E26_ADDR_MASK), 0U, 0U};
	uint8_t rx[3];
	const struct spi_buf tx_buf = {.buf = tx, .len = sizeof(tx)};
	const struct spi_buf rx_buf = {.buf = rx, .len = sizeof(rx)};
	const struct spi_buf_set tx_set = {.buffers = &tx_buf, .count = 1};
	const struct spi_buf_set rx_set = {.buffers = &rx_buf, .count = 1};
	int ret;

	ret = spi_transceive_dt(&cfg->bus.spi, &tx_set, &rx_set);
	if (ret < 0) {
		return ret;
	}

	*val = sys_get_be16(&rx[1]);

	return 0;
}

static int atm90e26_spi_write(const struct device *dev, uint8_t addr, uint16_t val)
{
	const struct atm90e26_config *cfg = dev->config;
	uint8_t tx[3] = {addr & ATM90E26_ADDR_MASK};
	const struct spi_buf tx_buf = {.buf = tx, .len = sizeof(tx)};
	const struct spi_buf_set tx_set = {.buffers = &tx_buf, .count = 1};

	sys_put_be16(val, &tx[1]);

	return spi_write_dt(&cfg->bus.spi, &tx_set);
}

const struct atm90e26_bus_io atm90e26_bus_io_spi = {
	.check = atm90e26_spi_check,
	.read = atm90e26_spi_read,
	.write = atm90e26_spi_write,
};
