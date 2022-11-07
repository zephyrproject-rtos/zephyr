/*
 * Copyright (c) 2022 Esco Medical ApS
 * Copyright (c) 2020 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include "icm42670_spi.h"
#include "icm42670_reg.h"

static inline int spi_write_register(const struct spi_dt_spec *bus, uint8_t reg, uint8_t data)
{
	const struct spi_buf buf[2] = {
		{
			.buf = &reg,
			.len = 1,
		},
		{
			.buf = &data,
			.len = 1,
		}
	};

	const struct spi_buf_set tx = {
		.buffers = buf,
		.count = 2,
	};

	return spi_write_dt(bus, &tx);
}

static inline int spi_read_register(const struct spi_dt_spec *bus, uint8_t reg, uint8_t *data,
				    size_t len)
{
	uint8_t tx_buffer = REG_SPI_READ_BIT | reg;

	const struct spi_buf tx_buf = {
		.buf = &tx_buffer,
		.len = 1,
	};

	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};

	struct spi_buf rx_buf[2] = {
		{
			.buf = NULL,
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

	return spi_transceive_dt(bus, &tx, &rx);
}

static inline int spi_read_mreg(const struct spi_dt_spec *bus, uint8_t reg, uint8_t bank,
				uint8_t *buf, size_t len)
{
	int res = spi_write_register(bus, REG_BLK_SEL_R, bank);

	if (res) {
		return res;
	}

	/* reads from MREG registers must be done byte-by-byte */
	for (size_t i = 0; i < len; i++) {
		uint8_t addr = reg + i;

		res = spi_write_register(bus, REG_MADDR_R, addr);

		if (res) {
			return res;
		}

		k_usleep(MREG_R_W_WAIT_US);
		res = spi_read_register(bus, REG_M_R, &buf[i], 1);

		if (res) {
			return res;
		}

		k_usleep(MREG_R_W_WAIT_US);
	}

	return 0;
}

static inline int spi_write_mreg(const struct spi_dt_spec *bus, uint8_t reg, uint8_t bank,
				 uint8_t buf)
{
	int res = spi_write_register(bus, REG_BLK_SEL_W, bank);

	if (res) {
		return res;
	}

	res = spi_write_register(bus, REG_MADDR_W, reg);

	if (res) {
		return res;
	}

	res = spi_write_register(bus, REG_M_W, buf);

	if (res) {
		return res;
	}

	k_usleep(MREG_R_W_WAIT_US);

	return 0;
}

int icm42670_spi_read(const struct spi_dt_spec *bus, uint16_t reg, uint8_t *data, size_t len)
{
	int res = 0;
	uint8_t bank = FIELD_GET(REG_BANK_MASK, reg);
	uint8_t address = FIELD_GET(REG_ADDRESS_MASK, reg);

	if (bank) {
		res = spi_read_mreg(bus, address, bank, data, len);
	} else {
		res = spi_read_register(bus, address, data, len);
	}

	return res;
}

int icm42670_spi_update_register(const struct spi_dt_spec *bus, uint16_t reg, uint8_t mask,
				 uint8_t data)
{
	uint8_t temp = 0;
	int res = icm42670_spi_read(bus, reg, &temp, 1);

	if (res) {
		return res;
	}

	temp &= ~mask;
	temp |= FIELD_PREP(mask, data);

	return icm42670_spi_single_write(bus, reg, temp);
}

int icm42670_spi_single_write(const struct spi_dt_spec *bus, uint16_t reg, uint8_t data)
{
	int res = 0;
	uint8_t bank = FIELD_GET(REG_BANK_MASK, reg);
	uint8_t address = FIELD_GET(REG_ADDRESS_MASK, reg);

	if (bank) {
		res = spi_write_mreg(bus, address, bank, data);
	} else {
		res = spi_write_register(bus, address, data);
	}

	return res;
}
