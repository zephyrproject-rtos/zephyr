/*
 * Copyright (c) 2025, Atmel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(m90e26_spi, CONFIG_SENSOR_LOG_LEVEL);

#include "m90e26.h"

static int m90e26_bus_check_spi(const struct device *dev)
{
	const struct m90e26_config *config = (const struct m90e26_config *)(dev->config);

	if (SPI_MODE_GET(config->bus.spi.config.operation) != (SPI_MODE_CPOL | SPI_MODE_CPHA)) {
		LOG_ERR("SPI mode for device %s must be CPOL=1 and CPHA=1.", dev->name);
		return -EINVAL;
	}

	return 0;
}

static int m90e26_read_reg_spi(const struct device *dev, const m90e26_register_t addr,
			       m90e26_data_value_t *value)
{
	if (!value) {
		return -EINVAL;
	}

	int ret;
	const struct m90e26_config *config = (const struct m90e26_config *)(dev->config);
	uint8_t buffer[3] = {M90E26_CMD_READ_MASK | addr, 0, 0};

	const struct spi_buf rxtx_buf[] = {{.buf = buffer, .len = ARRAY_SIZE(buffer)}};

	const struct spi_buf_set rxtx = {
		.buffers = rxtx_buf,
		.count = ARRAY_SIZE(rxtx_buf),
	};

	ret = spi_transceive_dt(&config->bus.spi, &rxtx, &rxtx);
	if (ret < 0) {
		LOG_ERR("Failed to read SPI Reg 0x%04X: %d", addr, ret);
		return ret;
	}

	value->uint16 = sys_get_be16(&buffer[1]);

	return spi_release_dt(&config->bus.spi);
}

static int m90e26_write_reg_spi(const struct device *dev, const m90e26_register_t addr,
				const m90e26_data_value_t *value)
{
	int ret;
	const struct m90e26_config *config = (const struct m90e26_config *)(dev->config);
	uint8_t buffer[3] = {addr & M90E26_CMD_WRITE_MASK, 0xFF, 0xFF};

	sys_put_be16(value->uint16, &buffer[1]);

	const struct spi_buf tx_buf[] = {{.buf = buffer, .len = sizeof(buffer)}};

	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf),
	};

	ret = spi_write_dt(&config->bus.spi, &tx);
	if (ret < 0) {
		LOG_ERR("Failed to write SPI Reg 0x%04X: %d", addr, ret);
		return ret;
	}

	return spi_release_dt(&config->bus.spi);
}

const struct m90e26_bus_io m90e26_bus_io_spi = {
	.bus_check = m90e26_bus_check_spi,
	.read = m90e26_read_reg_spi,
	.write = m90e26_write_reg_spi,
};
