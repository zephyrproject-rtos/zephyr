/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Dipak Shetty
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include <adi_tmc_bus.h>
#include <adi_tmc_spi.h>
#include <adi_tmc_reg.h>

#include "tmc51xx.h"

#if TMC51XX_BUS_SPI
LOG_MODULE_DECLARE(tmc51xx, CONFIG_STEPPER_LOG_LEVEL);

static int tmc51xx_bus_check_spi(const union tmc_bus *bus, uint8_t comm_type)
{
	if (comm_type != TMC_COMM_SPI) {
		return -ENOTSUP;
	}
	return spi_is_ready_dt(&bus->spi) ? 0 : -ENODEV;
}

static int tmc51xx_reg_write_spi(const struct device *dev, const uint8_t reg_addr,
				 const uint32_t reg_val)
{
	const struct tmc51xx_config *config = dev->config;
	int err;

	err = tmc_spi_write_register(&config->bus.spi, TMC5XXX_WRITE_BIT, reg_addr, reg_val);

	if (err < 0) {
		LOG_ERR("Failed to write register 0x%x with value 0x%x", reg_addr, reg_val);
	}
	return err;
}

static int tmc51xx_reg_read_spi(const struct device *dev, const uint8_t reg_addr, uint32_t *reg_val)
{
	const struct tmc51xx_config *config = dev->config;
	int err;

	err = tmc_spi_read_register(&config->bus.spi, TMC5XXX_ADDRESS_MASK, reg_addr, reg_val);

	if (err < 0) {
		LOG_ERR("Failed to read register 0x%x", reg_addr);
	}
	return err;
}

const struct tmc_bus_io tmc51xx_spi_bus_io = {
	.check = tmc51xx_bus_check_spi,
	.read = tmc51xx_reg_read_spi,
	.write = tmc51xx_reg_write_spi,
};
#endif /* TMC51XX_BUS_SPI */
