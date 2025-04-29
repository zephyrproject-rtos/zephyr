/*
 * Copyright (c) 2025 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bus-specific functionality for ICP201XX accessed via SPI.
 */

#include "icp201xx_drv.h"

#define ICP201XX_SERIF_SPI_REG_WRITE_CMD 0X33
#define ICP201XX_SERIF_SPI_REG_READ_CMD  0X3C
static int icp201xx_read_reg_spi(const union icp201xx_bus *bus, uint8_t reg, uint8_t *rbuffer,
				 uint32_t rlen)
{
	if (reg != 0) {
		int rc = 0;

		uint8_t cmd[] = {ICP201XX_SERIF_SPI_REG_READ_CMD, reg};
		const struct spi_buf tx_buf = {.buf = cmd, .len = 2};
		const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
		struct spi_buf rx_buf[] = {{.buf = NULL, .len = 2}, {.buf = rbuffer, .len = rlen}};
		const struct spi_buf_set rx = {.buffers = rx_buf, .count = 2};

		rc = spi_transceive_dt(&bus->spi, &tx, &rx);

		return rc;
	}
	return 0;
}

static int icp201xx_write_reg_spi(const union icp201xx_bus *bus, uint8_t reg, uint8_t *wbuffer,
				  uint32_t wlen)
{
	if (reg != 0) {
		int rc = 0;
		uint8_t cmd[] = {ICP201XX_SERIF_SPI_REG_WRITE_CMD, reg};
		const struct spi_buf tx_buf[] = {{.buf = cmd, .len = 2},
						 {.buf = (uint8_t *)wbuffer, .len = wlen}};
		const struct spi_buf_set tx = {.buffers = tx_buf, .count = 2};

		rc = spi_write_dt(&bus->spi, &tx);
		return rc;
	}
	return 0;
}

const struct icp201xx_bus_io icp201xx_bus_io_spi = {
	.read = icp201xx_read_reg_spi,
	.write = icp201xx_write_reg_spi,
};
