/*
 * Copyright (c) 2026 T3 Gemstone
 *	T3 Gemstone Developer Team <support@t3gemstone.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>

#include "icm20948.h"
#include "icm20948_reg.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)

static int icm20948_spi_write(const union icm20948_bus *bus, uint8_t addr, uint8_t val)
{
	uint8_t cmd[2] = {addr & ~REG_SPI_READ_BIT, val};
	const struct spi_buf buf = {.buf = cmd, .len = sizeof(cmd)};
	const struct spi_buf_set tx = {.buffers = &buf, .count = 1};

	return spi_write_dt(&bus->spi, &tx);
}

static int icm20948_spi_read(const union icm20948_bus *bus, uint8_t addr, uint8_t *buf, size_t len)
{
	uint8_t cmd = addr | REG_SPI_READ_BIT;
	const struct spi_buf tx_buf = {.buf = &cmd, .len = sizeof(cmd)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	struct spi_buf rx_buf[2] = {
		{.buf = NULL, .len = sizeof(cmd)},
		{.buf = buf, .len = len},
	};
	const struct spi_buf_set rx = {.buffers = rx_buf, .count = ARRAY_SIZE(rx_buf)};

	return spi_transceive_dt(&bus->spi, &tx, &rx);
}

static int icm20948_spi_check(const union icm20948_bus *bus)
{
	return spi_is_ready_dt(&bus->spi) ? 0 : -ENODEV;
}

const struct icm20948_bus_io icm20948_bus_io_spi = {
	.check = icm20948_spi_check,
	.read = icm20948_spi_read,
	.write = icm20948_spi_write,
};

#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
