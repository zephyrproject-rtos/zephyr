/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <adi_tmc_spi.h>

#define BUFFER_SIZE 5U

int tmc_spi_read_register(const struct spi_dt_spec *bus, const uint8_t read_address_mask,
			  const uint8_t register_address, uint32_t *data)
{
	uint8_t tx_buffer[BUFFER_SIZE] = {read_address_mask & register_address, 0U, 0U, 0U, 0U};
	uint8_t rx_buffer[BUFFER_SIZE];
	int status;

	const struct spi_buf spi_buffer_tx = {
		.buf = &tx_buffer,
		.len = sizeof(tx_buffer),
	};
	struct spi_buf_set spi_buffer_array_tx = {
		.buffers = &spi_buffer_tx,
		.count = 1U,
	};

	struct spi_buf spi_buffer_rx = {
		.buf = &rx_buffer,
		.len = sizeof(rx_buffer),
	};
	struct spi_buf_set spi_buffer_array_rx = {
		.buffers = &spi_buffer_rx,
		.count = 1U,
	};

	/** send read with the address byte */
	status = spi_transceive_dt(bus, &spi_buffer_array_tx, &spi_buffer_array_rx);
	if (status < 0) {
		return status;
	}

	/** read the value from the address */
	status = spi_transceive_dt(bus, &spi_buffer_array_tx, &spi_buffer_array_rx);
	if (status < 0) {
		return status;
	}

	*data = ((uint32_t)rx_buffer[1] << 24) + ((uint32_t)rx_buffer[2] << 16) +
		((uint32_t)rx_buffer[3] << 8) + (uint32_t)rx_buffer[4];

	return status;
}

int tmc_spi_write_register(const struct spi_dt_spec *bus, const uint8_t write_bit,
			   const uint8_t register_address, const uint32_t data)
{
	uint8_t tx_buffer[BUFFER_SIZE] = {write_bit | register_address, data >> 24, data >> 16,
					  data >> 8, data};
	uint8_t rx_buffer[BUFFER_SIZE];
	int status;

	const struct spi_buf spi_buffer_tx = {
		.buf = &tx_buffer,
		.len = sizeof(tx_buffer),
	};
	struct spi_buf_set spi_buffer_array_tx = {
		.buffers = &spi_buffer_tx,
		.count = 1U,
	};

	struct spi_buf spi_buffer_rx = {
		.buf = &rx_buffer,
		.len = sizeof(rx_buffer),
	};
	struct spi_buf_set spi_buffer_array_rx = {
		.buffers = &spi_buffer_rx,
		.count = 1U,
	};

	status = spi_transceive_dt(bus, &spi_buffer_array_tx, &spi_buffer_array_rx);
	if (status < 0) {
		return status;
	}

	return status;
}
