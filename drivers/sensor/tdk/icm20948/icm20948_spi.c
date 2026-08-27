/*
 * Copyright (c) 2026 T3 Gemstone
 *	T3 Gemstone Developer Team <support@t3gemstone.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include "icm20948.h"
#include "icm20948_reg.h"

LOG_MODULE_DECLARE(ICM20948, CONFIG_SENSOR_LOG_LEVEL);

static int icm20948_raw_write(const struct spi_dt_spec *bus, uint8_t addr, uint8_t val)
{
	uint8_t cmd[2] = {addr & ~REG_SPI_READ_BIT, val};
	const struct spi_buf buf = {.buf = cmd, .len = sizeof(cmd)};
	const struct spi_buf_set tx = {.buffers = &buf, .count = 1};

	return spi_write_dt(bus, &tx);
}

static int icm20948_raw_read(const struct spi_dt_spec *bus, uint8_t addr, uint8_t *buf, size_t len)
{
	uint8_t cmd = addr | REG_SPI_READ_BIT;
	const struct spi_buf tx_buf = {.buf = &cmd, .len = sizeof(cmd)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	struct spi_buf rx_buf[2] = {
		{.buf = NULL, .len = sizeof(cmd)},
		{.buf = buf, .len = len},
	};
	const struct spi_buf_set rx = {.buffers = rx_buf, .count = ARRAY_SIZE(rx_buf)};

	return spi_transceive_dt(bus, &tx, &rx);
}

static int icm20948_select_bank(const struct device *dev, uint16_t reg)
{
	const struct icm20948_config *cfg = dev->config;
	struct icm20948_data *data = dev->data;
	uint8_t bank = FIELD_GET(REG_BANK_MASK, reg);
	int ret;

	if (bank == data->bank) {
		return 0;
	}

	ret = icm20948_raw_write(&cfg->spi, REG_BANK_SEL, FIELD_PREP(MASK_USER_BANK, bank));
	if (ret < 0) {
		LOG_ERR("failed to select user bank %u: %d", bank, ret);
		data->bank = ICM20948_BANK_UNKNOWN;
		return ret;
	}

	data->bank = bank;

	return 0;
}

int icm20948_read(const struct device *dev, uint16_t reg, uint8_t *buf, size_t len)
{
	const struct icm20948_config *cfg = dev->config;
	int ret;

	ret = icm20948_select_bank(dev, reg);
	if (ret < 0) {
		return ret;
	}

	return icm20948_raw_read(&cfg->spi, FIELD_GET(REG_ADDRESS_MASK, reg), buf, len);
}

int icm20948_write(const struct device *dev, uint16_t reg, uint8_t val)
{
	const struct icm20948_config *cfg = dev->config;
	int ret;

	ret = icm20948_select_bank(dev, reg);
	if (ret < 0) {
		return ret;
	}

	return icm20948_raw_write(&cfg->spi, FIELD_GET(REG_ADDRESS_MASK, reg), val);
}

int icm20948_update(const struct device *dev, uint16_t reg, uint8_t mask, uint8_t val)
{
	uint8_t old;
	int ret;

	ret = icm20948_read(dev, reg, &old, sizeof(old));
	if (ret < 0) {
		return ret;
	}

	return icm20948_write(dev, reg, (old & ~mask) | FIELD_PREP(mask, val));
}
