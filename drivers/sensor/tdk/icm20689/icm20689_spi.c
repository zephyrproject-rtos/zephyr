/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * Copyright (c) 2022 Esco Medical ApS
 * Copyright (c) 2020 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include "icm20689_spi.h"
#include "icm20689_reg.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ICM20689_SPI, CONFIG_SENSOR_LOG_LEVEL);

static inline int icm20689_spi_write_register(const struct spi_dt_spec *bus, uint8_t reg,
					      uint8_t data)
{
	uint8_t addr = reg & 0x7F;

	const struct spi_buf buf[2] = {{
					       .buf = &addr,
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

static inline int icm20689_spi_read_register(const struct spi_dt_spec *bus, uint8_t reg,
					     uint8_t *data, size_t len)
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

int icm20689_spi_read(const struct spi_dt_spec *bus, uint8_t reg, uint8_t *data, size_t len)
{
	return icm20689_spi_read_register(bus, reg, data, len);
}

int icm20689_spi_update_register(const struct spi_dt_spec *bus, uint8_t reg, uint8_t mask,
				 uint8_t data)
{
	uint8_t temp = 0;
	int ret = icm20689_spi_read(bus, reg, &temp, 1);

	if (ret != 0) {
		return ret;
	}

	temp = (temp & ~mask) | (data & mask);

	return icm20689_spi_single_write(bus, reg, temp);
}

int icm20689_spi_single_write(const struct spi_dt_spec *bus, uint8_t reg, uint8_t data)
{
	return icm20689_spi_write_register(bus, reg, data);
}
