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
		uint8_t calc_crc;

		if (rlen*2 > TAD214X_BUFFER_LEN) {
			LOG_ERR("tad214x_read_reg_i2c size error : %d", rlen);
			return -EINVAL;
		}

		cmd_buf[0] = TAD214X_SERIF_SPI_REG_READ_CMD;
		cmd_buf[1] = reg;
		cmd_buf[2] = crc8(cmd_buf, 2, 0x1D, 0xFF, false) ^ 0xFF;

		const struct spi_buf tx_buf = {.buf = cmd_buf, .len = sizeof(cmd_buf)};
		const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

		struct spi_buf rx_buf[] = {
			{.buf = NULL, .len = tx_buf.len},
			{.buf = buf, .len = rlen*2},
			{.buf = &crc_val, .len = 1},
		};
		const struct spi_buf_set rx = {.buffers = rx_buf, .count = 3};

		rc = spi_transceive_dt(&bus->spi, &tx, &rx);

		calc_crc = crc8(buf, rlen * 2, 0x1D, 0xFF, false) ^ 0xFF;
		if (calc_crc != crc_val) {
			LOG_ERR("%s CRC Mismatch! Calc: %x, Recv: %x", __func__, calc_crc, crc_val);
			return -EBADMSG;
		}

		for (uint32_t i = 0; i < rlen; i++) {
			rbuffer[i] = sys_get_be16(&buf[i * 2]);
		}

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

		if (wlen*2+1 > TAD214X_BUFFER_LEN) {
			LOG_ERR("tad214x_read_reg_i2c size error : %d", wlen);
			return -EINVAL;
		}

		memset(buf, 0x00, sizeof(buf));

		buf[0] = TAD214X_SERIF_SPI_REG_WRITE_CMD;
		buf[1] = reg;

		for (uint32_t i = 0; i < wlen; i++) {
			sys_put_be16(wbuffer[i], &buf[2 + (i * 2)]);
		}

		/* CRC Compute on command, addr and data */
		buf[wlen * 2 + 2] = crc8(buf, wlen * 2 + 2, 0x1D, 0xFF, false) ^ 0xFF;

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
