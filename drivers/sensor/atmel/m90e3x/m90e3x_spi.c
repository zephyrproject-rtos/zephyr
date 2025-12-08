/*
 * Copyright (c) 2025, Atmel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(m90e3x_spi, CONFIG_SENSOR_LOG_LEVEL);

#include "m90e3x.h"

static int m90e3x_bus_check_spi(const struct device *dev)
{
	const struct m90e3x_config *config = (const struct m90e3x_config *)(dev->config);

	if (SPI_MODE_GET(config->bus.config.operation) != (SPI_MODE_CPOL | SPI_MODE_CPHA)) {
		LOG_ERR("SPI mode for device %s must be CPOL=1 and CPHA=1.", dev->name);
		return -EINVAL;
	}

	return spi_is_ready_dt(&config->bus) ? 0 : -ENODEV;
}

static int m90e3x_read_reg_spi(const struct device *dev, const m90e3x_register_t addr,
			       m90e3x_data_value_t *value)
{
	if (!value) {
		return -EINVAL;
	}

	int ret = 0;
	const struct m90e3x_config *config = (const struct m90e3x_config *)(dev->config);
	uint8_t buffer[4] = {0};

	sys_put_be16(sys_be16_to_cpu((uint16_t)(addr | M90E3X_SPI_READ_MASK)), &buffer[0]);

	const struct spi_buf rxtx_buf[] = {{.buf = buffer, .len = ARRAY_SIZE(buffer)}};

	const struct spi_buf_set rxtx = {
		.buffers = rxtx_buf,
		.count = ARRAY_SIZE(rxtx_buf),
	};

	ret = spi_transceive_dt(&config->bus, &rxtx, &rxtx);
	if (ret < 0) {
		LOG_ERR("Failed to read SPI Reg 0x%04X: %d", addr, ret);
		return ret;
	}

	ret = spi_release_dt(&config->bus);
	if (ret < 0) {
		LOG_ERR("Failed to release SPI bus after reading Reg 0x%04X: %d", addr, ret);
		return ret;
	}

	value->uint16 = sys_be16_to_cpu(sys_get_be16(&buffer[2]));

	return ret;
}

static int m90e3x_write_reg_spi(const struct device *dev, const m90e3x_register_t addr,
				const m90e3x_data_value_t *value)
{
	int ret = 0;
	const struct m90e3x_config *config = (const struct m90e3x_config *)(dev->config);
	m90e3x_register_t tx_data[2] = {sys_le16_to_cpu((uint16_t)(addr & M90E3X_SPI_WRITE_MASK)),
					sys_le16_to_cpu(value->uint16)};

	const struct spi_buf tx_buf[] = {{
		.buf = tx_data,
		.len = sizeof(tx_data),
	}};

	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf),
	};

	ret = spi_transceive_dt(&config->bus, &tx, NULL);
	if (ret != 0) {
		LOG_ERR("Failed to write SPI Reg 0x%04X: %d", addr, ret);
		return ret;
	}

	ret = spi_release_dt(&config->bus);
	if (ret != 0) {
		LOG_ERR("Failed to release SPI bus after writing Reg 0x%04X: %d", addr, ret);
	}

	return ret;
}

const struct m90e3x_bus_io m90e3x_bus_io_spi = {
	.bus_check = m90e3x_bus_check_spi,
	.read = m90e3x_read_reg_spi,
	.write = m90e3x_write_reg_spi,
};
