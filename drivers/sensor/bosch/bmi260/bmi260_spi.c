/*
 * Copyright (c) 2026 Shreyas Acharya
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * SPI implementation of the BMI260 bus interface.
 */

#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

#include "bmi260.h"
#include "bmi260_reg_map.h"

LOG_MODULE_DECLARE(bmi260, CONFIG_SENSOR_LOG_LEVEL);

static int bmi260_bus_check_spi(const union bmi260_bus *bus)
{
	return spi_is_ready_dt(&bus->spi) ? 0 : -ENODEV;
}

static int bmi260_reg_read_spi(const union bmi260_bus *bus, uint8_t start, uint8_t *data,
			       uint16_t len)
{
	uint8_t addr = start | BMI260_SPI_READ_BIT;
	uint8_t discarded[1 + BMI260_SPI_DUMMY_BYTES];
	const struct spi_buf tx_buf = {
		.buf = &addr,
		.len = sizeof(addr),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};
	/*
	 * A read puts the address on the wire, then a byte of padding, and only
	 * then the register contents, so the first two bytes clocked back are
	 * thrown away.
	 */
	struct spi_buf rx_buf[2] = {
		{
			.buf = discarded,
			.len = sizeof(discarded),
		},
		{
			.buf = data,
			.len = len,
		},
	};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(rx_buf),
	};

	return spi_transceive_dt(&bus->spi, &tx, &rx);
}

static int bmi260_reg_write_spi(const union bmi260_bus *bus, uint8_t start, const uint8_t *data,
				uint16_t len)
{
	uint8_t addr = start & BMI260_REG_MASK;
	const struct spi_buf tx_buf[2] = {
		{
			.buf = &addr,
			.len = sizeof(addr),
		},
		{
			/* Cast away const: the SPI API does not write to tx buffers. */
			.buf = (uint8_t *)data,
			.len = len,
		},
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf),
	};

	return spi_write_dt(&bus->spi, &tx);
}

static int bmi260_bus_init_spi(const union bmi260_bus *bus)
{
	uint8_t discarded;

	/*
	 * The device listens on I2C until its chip select is first asserted,
	 * so one throwaway read moves it to SPI. It has to be repeated after a
	 * soft reset, which returns the device to I2C.
	 */
	return bmi260_reg_read_spi(bus, BMI260_REG_CHIP_ID, &discarded, sizeof(discarded));
}

const struct bmi260_bus_io bmi260_bus_io_spi = {
	.check = bmi260_bus_check_spi,
	.read = bmi260_reg_read_spi,
	.write = bmi260_reg_write_spi,
	.init = bmi260_bus_init_spi,
};
