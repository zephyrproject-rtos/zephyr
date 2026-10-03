/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Cherrence Sarip <Cherrence.sarip@analog.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/spi.h>

#include <adi_tmc_spi.h>
#include "tmc522x.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(tmc522x, CONFIG_STEPPER_LOG_LEVEL);

static int tmc522x_bus_check_spi(const union tmc522x_bus *bus)
{
	return spi_is_ready_dt(&bus->spi) ? 0 : -ENODEV;
}

static int tmc522x_bus_init_spi(const union tmc522x_bus *bus)
{
	/* SPI is initialized by the SPI driver, nothing special needed */
	return tmc522x_bus_check_spi(bus);
}

static int tmc522x_reg_read_spi(const union tmc522x_bus *bus, uint8_t reg_addr, uint32_t *reg_val)
{
	int ret;

	if (reg_val == NULL) {
		return -EINVAL;
	}

	ret = tmc_spi_read_register(&bus->spi, TMC522X_ADDRESS_MASK, reg_addr, reg_val);
	if (ret < 0) {
		LOG_ERR("SPI read failed: reg=0x%02X, err=%d", reg_addr, ret);
		return ret;
	}

	return 0;
}

static int tmc522x_reg_write_spi(const union tmc522x_bus *bus, uint8_t reg_addr, uint32_t reg_val)
{
	int ret;

	ret = tmc_spi_write_register(&bus->spi, TMC522X_WRITE_BIT, reg_addr, reg_val);
	if (ret < 0) {
		LOG_ERR("SPI write failed: reg=0x%02X val=0x%08X, err=%d", reg_addr, reg_val, ret);
		return ret;
	}

	return 0;
}

const struct tmc522x_bus_io tmc522x_bus_io_spi = {
	.check = tmc522x_bus_check_spi,
	.init = tmc522x_bus_init_spi,
	.read = tmc522x_reg_read_spi,
	.write = tmc522x_reg_write_spi,
};
