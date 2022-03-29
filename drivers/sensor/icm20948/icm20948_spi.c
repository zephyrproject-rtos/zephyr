/*
 * Copyright (c) 2020 Michael Pollind
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include "icm20948.h"
#include <logging/log.h>
#include <drivers/spi.h>

#ifdef DT_TDK_ICM20948_BUS_SPI

#define ICM20948_SPI_READ (1 << 7)

LOG_MODULE_DECLARE(ICM20948, CONFIG_SENSOR_LOG_LEVEL);

static int icm20948_bus_check_spi(const union icm20948_bus *bus)
{
	return device_is_ready(bus->i2c.bus) ? 0 : -ENODEV;
}

static int
icm20948_raw_read(struct icm20948_bus *data, uint8_t reg_addr, uint8_t *value,
		  uint8_t len)
{
	uint8_t buffer_tx[2] = { reg_addr | ICM20948_SPI_READ, 0 };
	const struct spi_buf tx_buf = {
		.buf = buffer_tx,
		.len = 2,
	};
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };
	const struct spi_buf rx_buf[2] = { {
						   .buf = NULL,
						   .len = 1,
					   },
					   {
						   .buf = value,
						   .len = len,
					   } };
	const struct spi_buf_set rx = { .buffers = rx_buf, .count = 2 };

	if (len > 64) {
		return -EIO;
	}

	if (spi_transceive_dt(&data->spi, &tx, &rx)) {
		return -EIO;
	}

	return 0;
}

static int icm20948_raw_write(struct icm20948_bus *bus, uint8_t reg_addr, uint8_t *value, uint8_t len)
{
	uint8_t buffer_tx[1] = { reg_addr & ~ICM20948_SPI_READ };
	const struct spi_buf tx_buf[2] = { {
						   .buf = buffer_tx,
						   .len = 1,
					   },
					   {
						   .buf = value,
						   .len = len,
					   } };
	const struct spi_buf_set tx = { .buffers = tx_buf, .count = 2 };

	if (len > 64) {
		return -EIO;
	}

	if (spi_write_dt(&bus->spi, &tx)) {
		return -EIO;
	}

	return 0;
}

static inline int icm20948_change_bank(struct icm20948_bus *bus, uint16_t reg_bank_addr)
{
	uint8_t bank = (uint8_t)(reg_bank_addr >> 7);

	if (bank != bus->active_bank) {
		bus->active_bank = bank; // save active bank
		uint8_t tmp_val = (bank << 4);
		return icm20948_raw_write(bus, ICM20948_REG_BANK_SEL, &tmp_val, 1);
	}
	return 0;
}

static int icm20948_spi_read_data(struct icm20948_bus *bus, uint16_t reg_bank_addr, uint8_t *value,
				  uint8_t len)
{
	if (icm20948_change_bank(bus, reg_bank_addr)) {
		return -EIO;
	}
	return icm20948_raw_read(bus, reg_addr, value, len);
}

static int icm20948_spi_write_data(struct icm20948_bus *bus, uint16_t reg_bank_addr, uint8_t *value,
				   uint8_t len)
{
	if (icm20948_change_bank(bus, reg_bank_addr)) {
		return -EIO;
	}
	return icm20948_raw_write(bus, reg_addr, value, len);
}

static int icm20948_spi_read_reg(struct icm20948_bus *bus, uint16_t reg_bank_addr, uint8_t *value)
{
	if (icm20948_change_bank(bus, reg_bank_addr)) {
		return -EIO;
	}
	return icm20948_raw_read(bus, reg_addr, value, 1);
}

static int icm20948_spi_write_reg(struct icm20948_bus *bus, uint16_t reg_bank_addr, uint8_t value)
{
	if (icm20948_change_bank(bus, reg_bank_addr)) {
		return -EIO;
	}
	uint8_t tmp_val = value;

	return icm20948_raw_write(bus, reg_addr, &tmp_val, 1);
}

static int icm20948_spi_update_reg(struct icm20948_bus *bus, uint16_t reg_bank_addr, uint8_t mask,
				   uint8_t value)
{
	if (icm20948_change_bank(bus, reg_bank_addr)) {
		return -EIO;
	}
	uint8_t tmp_val;

	icm20948_raw_read(bus, reg_addr, &tmp_val, 1);
	tmp_val = (tmp_val & ~mask) | ((value << __builtin_ctz(mask)) & mask);

	return icm20948_raw_write(bus, reg_addr, &tmp_val, 1);
}

static const struct icm20948_bus_io icm20948_bus_io_spi = {
	.check = icm20948_bus_check_spi,
	.read_data = icm20948_spi_read_data,
	.write_data = icm20948_spi_write_data,
	.read_reg = icm20948_spi_read_reg,
	.write_reg = icm20948_spi_write_reg,
	.update_reg = icm20948_spi_update_reg,
};

#endif