/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Dipak Shetty
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include "tmc51xx.h"
#include "adi_tmc5xxx_core.h"
#include <adi_tmc_bus.h>
#include <adi_tmc_uart.h>

#if CONFIG_STEPPER_ADI_TMC_UART
LOG_MODULE_DECLARE(tmc5xxx, CONFIG_STEPPER_LOG_LEVEL);

static int tmc5xxx_bus_check_uart(const union tmc_bus *bus, uint8_t comm_type)
{
	if (comm_type != TMC_COMM_UART) {
		return -ENOTSUP;
	}
	return device_is_ready(bus->uart) ? 0 : -ENODEV;
}

static int tmc5xxx_reg_write_uart(const struct device *dev, const uint8_t reg_addr,
				  const uint32_t reg_val)
{
	const struct tmc5xxx_controller_config *config = dev->config;
	int err;

	/* Route to the adi_tmc_uart.h implementation */
	err = tmc_uart_write_register(config->bus.uart, config->uart_addr, reg_addr, reg_val);

	if (err < 0) {
		LOG_ERR("Failed to write register 0x%x with value 0x%x", reg_addr, reg_val);
	}
	/* Wait for the write to complete */
	k_sleep(K_MSEC(1));
	return err;
}

static int tmc5xxx_reg_read_uart(const struct device *dev, const uint8_t reg_addr,
				 uint32_t *reg_val)
{
	const struct tmc5xxx_controller_config *config = dev->config;
	int err;

	/* Route to the adi_tmc_uart.h implementation */
	err = tmc_uart_read_register(config->bus.uart, config->uart_addr, reg_addr, reg_val);

	if (err < 0) {
		LOG_ERR("Failed to read register 0x%x", reg_addr);
	}
	/* Wait for the read to complete */
	k_sleep(K_MSEC(1));
	return err;
}

const struct tmc_bus_io tmc5xxx_uart_bus_io = {
	.check = tmc5xxx_bus_check_uart,
	.read = tmc5xxx_reg_read_uart,
	.write = tmc5xxx_reg_write_uart,
};
#endif /* CONFIG_STEPPER_ADI_TMC_UART */
