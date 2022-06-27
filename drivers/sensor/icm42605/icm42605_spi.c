/*
 * Copyright (c) 2020 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include "icm42605_spi.h"

LOG_MODULE_DECLARE(ICM42605, CONFIG_SENSOR_LOG_LEVEL);

int inv_spi_single_write(const struct spi_dt_spec *bus, uint8_t reg, uint8_t *data)
{
	int result;

	const struct spi_buf buf[2] = {
		{
			.buf = &reg,
			.len = 1,
		},
		{
			.buf = data,
			.len = 1,
		}
	};
	const struct spi_buf_set tx = {
		.buffers = buf,
		.count = 2,
	};

	result = spi_write_dt(bus, &tx);

	if (result) {
		return result;
	}

	return 0;
}

int inv_spi_read(const struct spi_dt_spec *bus, uint8_t reg, uint8_t *data, size_t len)
{
	int result;
	unsigned char tx_buffer[2] = { 0, };

	tx_buffer[0] = 0x80 | reg;

	const struct spi_buf tx_buf = {
		.buf = tx_buffer,
		.len = 1,
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};

	struct spi_buf rx_buf[2] = {
		{
			.buf = tx_buffer,
			.len = 1,
		},
		{
			.buf = data,
			.len = len,
		}
	};

	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = 2,
	};

	result = spi_transceive_dt(bus, &tx, &rx);

	if (result) {
		return result;
	}

	return 0;
}
