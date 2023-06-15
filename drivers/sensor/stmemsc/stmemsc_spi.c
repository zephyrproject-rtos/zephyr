/* ST Microelectronics STMEMS hal i/f
 *
 * Copyright (c) 2021 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * zephyrproject-rtos/modules/hal/st/sensor/stmemsc/
 */

#include "stmemsc.h"

#define SPI_READ		(1 << 7)

/*
 * SPI read
 */
int stmemsc_spi_read(const struct spi_dt_spec *stmemsc,
		     uint8_t reg_addr, uint8_t *value, uint8_t len)
{
	uint8_t buffer_tx[2] = { reg_addr | SPI_READ, 0 };
	bool half_duplex = (stmemsc->config.operation & SPI_HALF_DUPLEX);

	/*
	 * write 1 byte with reg addr (msb at 1)
	 * + 1 dummy byte in full duplex, none in half duplex
	 */
	const struct spi_buf tx_buf = {
		.buf = buffer_tx,
		.len = half_duplex ? 1 : 2,
	};
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };

	/*
	 * Full duplex RX buffers:
	 *   - dummy read to skip first byte
	 *   - read "len" byte of data
	 */
	const struct spi_buf rx_buf_full[2] = {
		{ .buf = NULL, .len = 1, },
		{ .buf = value, .len = len, }
	};
	/*
	 * Half duplex RX buffer:
	 *   - read "len" byte of data
	 */
	const struct spi_buf rx_buf_half[1] = {
		{ .buf = value, .len = len, }
	};

	/*
	 * Using the right RX buffer whether half or full duplex is used
	 */
	const struct spi_buf_set rx = {
		.buffers = half_duplex ? rx_buf_half : rx_buf_full,
		.count = half_duplex ? 1 : 2,
	};

	return spi_transceive_dt(stmemsc, &tx, &rx);
}

/*
 * SPI write
 */
int stmemsc_spi_write(const struct spi_dt_spec *stmemsc,
		      uint8_t reg_addr, uint8_t *value, uint8_t len)
{
	uint8_t buffer_tx[1] = { reg_addr & ~SPI_READ };

	/*
	 *   transaction #1: write 1 byte with reg addr (msb at 0)
	 *   transaction #2: write "len" byte of data
	 */
	const struct spi_buf tx_buf[2] = {
		{ .buf = buffer_tx, .len = 1, },
		{ .buf = value, .len = len, }
	};
	const struct spi_buf_set tx = { .buffers = tx_buf, .count = 2 };

	return spi_write_dt(stmemsc, &tx);
}
