/*
 * Copyright (c) 2026 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bus-specific functionality for TAD214X accessed via SPI.
 */

#include "tad214x_drv.h"

#if CONFIG_SPI

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(TAD214X_SPI, CONFIG_SENSOR_LOG_LEVEL);

#define TAD214X_SERIF_SPI_REG_WRITE_CMD 0X33
#define TAD214X_SERIF_SPI_REG_READ_CMD  0X3C

static int tad214x_bus_check_spi(const union tad214x_bus *bus)
{
	return spi_is_ready_dt(&bus->spi) ? 0 : -ENODEV;
}

static int tad214x_read_reg_spi(const union tad214x_bus *bus, uint8_t reg, uint16_t *rbuffer,
				 uint32_t rlen)
{
	if (reg != 0) {
		uint8_t cmd_buf[3];
		uint8_t buf[TAD214X_BUFFER_LEN];
		int rc = 0;
		uint8_t crc_val;

		if(rlen*2 > TAD214X_BUFFER_LEN) {
			LOG_ERR("tad214x_read_reg_i2c size error : %d", rlen);
			return -EINVAL;
		}

		cmd_buf[0] = TAD214X_SERIF_SPI_REG_READ_CMD;
		cmd_buf[1] = reg;
		cmd_buf[2] = crc8_sae_j1850(&cmd_buf[0], 2);

		const struct spi_buf tx_buf = {.buf = cmd_buf, .len = sizeof(cmd_buf)};
		const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

		struct spi_buf rx_buf[] = {
                                    {.buf = NULL, .len = tx_buf.len},
                                    {.buf = buf, .len = rlen*2},
                                    {.buf = &crc_val, .len = 1},
                                  };
		const struct spi_buf_set rx = {.buffers = rx_buf, .count = 3};

		rc = spi_transceive_dt(&bus->spi, &tx, &rx);

		if (crc8_sae_j1850(&buf[0], rlen * 2) != crc_val) {
			LOG_ERR("tad214x_read_reg_spi rc: %d, crc: %x %x",
				rc, crc8_sae_j1850(&buf[0], rlen * 2), crc_val);
			return -EBADMSG;
		}

		memswap16(&buf[0], rlen*2);
		memcpy((uint8_t*) rbuffer, buf, rlen*2);

		return rc;
	}

	return -EBADMSG;
}

static int tad214x_write_reg_spi(const union tad214x_bus *bus, uint8_t reg, uint16_t *wbuffer,
				  uint32_t wlen)
{
	if (reg != 0) {
		uint8_t buf[TAD214X_BUFFER_LEN+2];
		int rc = 0;

		if(wlen*2+1 > TAD214X_BUFFER_LEN) {
			LOG_ERR("tad214x_read_reg_i2c size error : %d", wlen);
			return -EINVAL;
		}

		memset(buf, 0x00, sizeof(buf));

		buf[0] = TAD214X_SERIF_SPI_REG_WRITE_CMD;
		buf[1] = reg;
		memcpy(&buf[2], (uint8_t *)wbuffer, wlen*2);
		memswap16(&buf[2], wlen*2);
		buf[wlen*2+2] = crc8_sae_j1850(&buf[0], wlen*2+2); /* CRC Compute on command, Adr and data */

		const struct spi_buf tx_buf = {.buf = &buf[0], .len = wlen*2+3};
		const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
		rc = spi_write_dt(&bus->spi, &tx);
		k_busy_wait(100); /*processing waiting time */
		return rc;
	}

	return -EBADMSG;
}

const struct tad214x_bus_io tad214x_bus_io_spi = {
	.check = tad214x_bus_check_spi,
	.read = tad214x_read_reg_spi,
	.write = tad214x_write_reg_spi,
};

#endif
