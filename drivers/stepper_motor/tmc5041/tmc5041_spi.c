/*
 * SPDX-FileCopyrightText: Copyright (c) 2023 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "tmc5041_spi.h"

#define BUFFER_SIZE 5U

LOG_MODULE_REGISTER(tmc_spi, CONFIG_STEPPER_MOTOR_CONTROLLER_LOG_LEVEL);

static void parse_tmc_spi_status(const uint8_t status_byte)
{
	if ((status_byte & BIT_MASK(0)) != 0) {
		LOG_WRN("spi dataframe: reset_flag detected");
	}
	if ((status_byte & BIT_MASK(1)) != 0) {
		LOG_WRN("spi dataframe: driver_error(1) detected");
	}
	if ((status_byte & BIT_MASK(2)) != 0) {
		LOG_WRN("spi dataframe: driver_error(2) detected");
	}
}

int tmc_spi_read_register(const struct spi_dt_spec *bus, const uint8_t register_address,
			  uint32_t *data)
{
	uint8_t tx_buffer[BUFFER_SIZE] = {0x7FU & register_address, 0U, 0U, 0U, 0U};
	uint8_t rx_buffer[BUFFER_SIZE] = {0U};
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
	LOG_DBG("TX [0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x]", tx_buffer[0], tx_buffer[1],
		tx_buffer[2], tx_buffer[3], tx_buffer[4]);
	LOG_DBG("RX [0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x]", rx_buffer[0], rx_buffer[1],
		rx_buffer[2], rx_buffer[3], rx_buffer[4]);
	parse_tmc_spi_status(rx_buffer[0]);

	/** read the value from the address */
	status = spi_transceive_dt(bus, &spi_buffer_array_tx, &spi_buffer_array_rx);
	if (status < 0) {
		return status;
	}

	*data = ((uint32_t)rx_buffer[1] << 24) + ((uint32_t)rx_buffer[2] << 16) +
		((uint32_t)rx_buffer[3] << 8) + (uint32_t)rx_buffer[4];

	LOG_DBG("TX [0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x]", tx_buffer[0], tx_buffer[1],
		tx_buffer[2], tx_buffer[3], tx_buffer[4]);
	LOG_DBG("RX [0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x]", rx_buffer[0], rx_buffer[1],
		rx_buffer[2], rx_buffer[3], rx_buffer[4]);
	parse_tmc_spi_status(rx_buffer[0]);
	return status;
}

int tmc_spi_write_register(const struct spi_dt_spec *bus, const uint8_t register_address,
			   const uint32_t data)
{
	uint8_t tx_buffer[BUFFER_SIZE] = {0x80U | register_address, data >> 24, data >> 16,
					  data >> 8, data};
	uint8_t rx_buffer[BUFFER_SIZE] = {0U};
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
	LOG_DBG("TX [0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x]", tx_buffer[0], tx_buffer[1],
		tx_buffer[2], tx_buffer[3], tx_buffer[4]);
	LOG_DBG("RX [0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x]", rx_buffer[0], rx_buffer[1],
		rx_buffer[2], rx_buffer[3], rx_buffer[4]);

	parse_tmc_spi_status(rx_buffer[0]);

	return status;
}
