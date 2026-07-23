/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mfrc522.h"

#define MFRC522_SPI_READ      0x80U
#define MFRC522_SPI_ADDR_MASK 0x7EU

static int mfrc522_spi_read_reg(const struct device *dev, uint8_t reg, uint8_t *val)
{
	const struct mfrc522_config *cfg = dev->config;
	uint8_t tx[2] = {MFRC522_SPI_READ | ((uint8_t)(reg << 1) & MFRC522_SPI_ADDR_MASK), 0x00U};
	uint8_t rx[2];
	const struct spi_buf tx_buf = {.buf = tx, .len = sizeof(tx)};
	const struct spi_buf rx_buf = {.buf = rx, .len = sizeof(rx)};
	const struct spi_buf_set tx_set = {.buffers = &tx_buf, .count = 1};
	const struct spi_buf_set rx_set = {.buffers = &rx_buf, .count = 1};
	int ret;

	ret = spi_transceive_dt(&cfg->bus, &tx_set, &rx_set);
	if (ret < 0) {
		return ret;
	}

	*val = rx[1];

	return 0;
}

static int mfrc522_spi_write_reg(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct mfrc522_config *cfg = dev->config;
	uint8_t tx[2] = {(uint8_t)(reg << 1) & MFRC522_SPI_ADDR_MASK, val};
	const struct spi_buf tx_buf = {.buf = tx, .len = sizeof(tx)};
	const struct spi_buf_set tx_set = {.buffers = &tx_buf, .count = 1};

	return spi_write_dt(&cfg->bus, &tx_set);
}

const struct mfrc522_transport_api mfrc522_spi_transport = {
	.read_reg = mfrc522_spi_read_reg,
	.write_reg = mfrc522_spi_write_reg,
};
