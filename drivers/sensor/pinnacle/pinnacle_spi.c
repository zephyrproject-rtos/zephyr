/*
 * Copyright (c) 2024, Ilia Kharin
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * SPI specific functionality for Pinnacle 1CA027 Register Access Protocol
 */

#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

#include "pinnacle.h"

LOG_MODULE_DECLARE(PINNACLE, CONFIG_SENSOR_LOG_LEVEL);

int pinnacle_check_spi(const struct spi_dt_spec *spi)
{
	int ret = spi_is_ready_dt(spi);

	if (!ret) {
		LOG_ERR("SPI bus %s is not ready", spi->bus->name);
		return ret;
	}

	return 0;
}

int pinnacle_write_spi(const struct spi_dt_spec *spi,
		       uint8_t address, uint8_t value)
{
	uint8_t tx_data[] = {
		PINNACLE_WRITE_REG(address),
		value,
	};
	const struct spi_buf tx_buf[] = {{
		.buf = tx_data,
		.len = ARRAY_SIZE(tx_data),
	}};
	const struct spi_buf_set tx_set = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf),
	};

	int ret = spi_write_dt(spi, &tx_set);

	if (ret) {
		LOG_ERR("Failed to write to SPI %s", spi->bus->name);
		return ret;
	}

	return 0;
}

int pinnacle_read_spi(const struct spi_dt_spec *spi, uint8_t address,
		      uint8_t *value)
{
	uint8_t tx_data[] = {
		PINNACLE_READ_REG(address),
		PINNACLE_FB,
		PINNACLE_FB,
		PINNACLE_FB,
	};
	const struct spi_buf tx_buf[] = {{
		.buf = tx_data,
		.len = ARRAY_SIZE(tx_data),
	}};
	const struct spi_buf_set tx_set = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf),
	};

	uint8_t rx_data[4];
	const struct spi_buf rx_buf[] = {{
		.buf = rx_data,
		.len = ARRAY_SIZE(rx_data),
	}};
	const struct spi_buf_set rx_set = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(rx_buf),
	};

	int ret = spi_transceive_dt(spi, &tx_set, &rx_set);

	if (ret) {
		LOG_ERR("Failed to read from SPI %s", spi->bus->name);
		return ret;
	}

	*value = rx_data[3];

	return 0;
}

int pinnacle_seq_read_spi(const struct spi_dt_spec *spi, uint8_t address,
			  uint8_t *data, uint8_t count)
{

	uint8_t size = count + 3;
	uint8_t tx_data[size];

	tx_data[0] = PINNACLE_READ_REG(address);
	tx_data[1] = PINNACLE_FC;
	tx_data[2] = PINNACLE_FC;

	uint8_t i = 3;

	for (; i < (count + 2); ++i) {
		tx_data[i] = PINNACLE_FC;
	}

	tx_data[i++] = PINNACLE_FB;

	const struct spi_buf tx_buf[] = {{
		.buf = tx_data,
		.len = size,
	}};
	const struct spi_buf_set tx_set = {
		.buffers = tx_buf,
		.count = 1,
	};

	uint8_t rx_data[size];
	const struct spi_buf rx_buf[] = {{
		.buf = rx_data,
		.len = size,
	}};
	const struct spi_buf_set rx_set = {
		.buffers = rx_buf,
		.count = 1,
	};

	int ret = spi_transceive_dt(spi, &tx_set, &rx_set);

	if (ret) {
		LOG_ERR("Failed to read from SPI %s", spi->bus->name);
		return ret;
	}

	for (i = 0; i < count; ++i) {
		data[i] = rx_data[i + 3];
	}

	return 0;
}
