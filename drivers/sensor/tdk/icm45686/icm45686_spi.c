/*
 * Copyright (c) 2024 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include "icm45686_spi.h"
#include "icm45686_reg.h"

static inline int spi_write_register(const struct spi_dt_spec *bus, uint8_t reg, uint8_t data)
{
	const struct spi_buf buf[2] = {{
					       .buf = &reg,
					       .len = 1,
				       },
				       {
					       .buf = &data,
					       .len = 1,
				       }};

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

	struct spi_buf rx_buf[2] = {{
					    .buf = NULL,
					    .len = 1,
				    },
				    {
					    .buf = data,
					    .len = len,
				    }};

	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = 2,
	};

	return spi_transceive_dt(bus, &tx, &rx);
}

int icm45686_read(const struct spi_dt_spec *bus, uint16_t reg, uint8_t *data, size_t len)
{
	int res = 0;
	uint8_t address = FIELD_GET(REG_ADDRESS_MASK, reg);

	res = spi_read_register(bus, address, data, len);

	return res;
}

int icm45686_update_register(const struct spi_dt_spec *bus, uint16_t reg, uint8_t mask,
			     uint8_t data)
{
	uint8_t temp = 0;
	int res = icm45686_read(bus, reg, &temp, 1);

	if (res) {
		return res;
	}

	temp &= ~mask;
	temp |= FIELD_PREP(mask, data);

	return icm45686_single_write(bus, reg, temp);
}

int icm45686_single_write(const struct spi_dt_spec *bus, uint16_t reg, uint8_t data)
{
	int res = 0;
	uint8_t address = FIELD_GET(REG_ADDRESS_MASK, reg);

	res = spi_write_register(bus, address, data);

	return res;
}
