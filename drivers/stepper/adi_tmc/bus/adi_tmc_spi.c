/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/byteorder.h>
#include <adi_tmc_spi.h>
#include "../adi_tmc_reg.h"

#define BUFFER_SIZE 5U

/* TMC SPI protocol constants */
#define ADI_TMC_SPI_WRITE_BIT    0x80U  /* MSB=1 for write access */
#define ADI_TMC_SPI_ADDRESS_MASK 0x7FU  /* 7-bit address field mask */

int tmc_spi_read_register(const struct spi_dt_spec *bus, uint8_t register_address, uint32_t *data)
{
	uint8_t tx_buffer[BUFFER_SIZE] = {ADI_TMC_SPI_ADDRESS_MASK & register_address, 0U, 0U, 0U, 0U};
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

	*data = sys_get_be32(&rx_buffer[1]);

	return status;
}

int tmc_spi_write_register(const struct spi_dt_spec *bus, uint8_t register_address, uint32_t data)
{
	uint8_t tx_buffer[BUFFER_SIZE];
	uint8_t rx_buffer[BUFFER_SIZE];

	tx_buffer[0] = ADI_TMC_SPI_WRITE_BIT | (register_address & ADI_TMC_SPI_ADDRESS_MASK);
	sys_put_be32(data, &tx_buffer[1]);

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

	return spi_transceive_dt(bus, &spi_buffer_array_tx, &spi_buffer_array_rx);
}
