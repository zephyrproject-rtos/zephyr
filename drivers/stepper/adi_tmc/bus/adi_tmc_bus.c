/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Dipak Shetty
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <adi_tmc_bus.h>
#ifdef CONFIG_STEPPER_ADI_TMC_SPI
#include <adi_tmc_spi.h>
#endif
#ifdef CONFIG_STEPPER_ADI_TMC_UART
#include <adi_tmc_uart.h>
#endif

LOG_MODULE_REGISTER(tmc_bus, CONFIG_STEPPER_LOG_LEVEL);

/*
 * Generic SPI Bus Adapter - works for all TMC series
 */

#ifdef CONFIG_STEPPER_ADI_TMC_SPI

static int tmc_bus_check_spi(const union tmc_bus *bus, uint8_t comm_type)
{
	if (comm_type != TMC_COMM_SPI) {
		return -ENOTSUP;
	}
	return spi_is_ready_dt(&bus->spi) ? 0 : -ENODEV;
}

static int tmc_bus_read_spi(const struct device *dev, const uint8_t reg_addr, uint32_t *reg_val)
{
	const struct tmc_common_config *config = dev->config;
	int err;

	err = tmc_spi_read_register(&config->bus.spi, reg_addr, reg_val);

	if (err < 0) {
		LOG_ERR("Failed to read register 0x%x", reg_addr);
	}
	return err;
}

static int tmc_bus_write_spi(const struct device *dev, const uint8_t reg_addr,
			     const uint32_t reg_val)
{
	const struct tmc_common_config *config = dev->config;
	int err;

	err = tmc_spi_write_register(&config->bus.spi, reg_addr, reg_val);

	if (err < 0) {
		LOG_ERR("Failed to write register 0x%x with value 0x%x", reg_addr, reg_val);
	}
	return err;
}

const struct tmc_bus_io tmc_spi_bus_io = {
	.check = tmc_bus_check_spi,
	.read = tmc_bus_read_spi,
	.write = tmc_bus_write_spi,
};

#endif /* CONFIG_STEPPER_ADI_TMC_SPI */

/*
 * Generic UART Bus Adapter - works for all TMC series
 */

#ifdef CONFIG_STEPPER_ADI_TMC_UART

/* Forward declaration for UART-specific config extraction */
struct tmc_uart_device_config {
	struct tmc_common_config common;
	/* UART-specific fields that generic adapter needs */
	uint8_t uart_addr;
};

static int tmc_bus_check_uart(const union tmc_bus *bus, uint8_t comm_type)
{
	if (comm_type != TMC_COMM_UART) {
		return -ENOTSUP;
	}
	return device_is_ready(bus->uart) ? 0 : -ENODEV;
}

static int tmc_bus_read_uart(const struct device *dev, const uint8_t reg_addr, uint32_t *reg_val)
{
	const struct tmc_uart_device_config *config = dev->config;
	int err;

	/* Route to the adi_tmc_uart.h implementation */
	err = tmc_uart_read_register(config->common.bus.uart, config->uart_addr, reg_addr,
				      reg_val);

	if (err < 0) {
		LOG_ERR("Failed to read register 0x%x", reg_addr);
	}
	/* Wait for the read to complete */
	k_sleep(K_MSEC(1));
	return err;
}

static int tmc_bus_write_uart(const struct device *dev, const uint8_t reg_addr,
			      const uint32_t reg_val)
{
	const struct tmc_uart_device_config *config = dev->config;
	int err;

	/* Route to the adi_tmc_uart.h implementation */
	err = tmc_uart_write_register(config->common.bus.uart, config->uart_addr, reg_addr,
				       reg_val);

	if (err < 0) {
		LOG_ERR("Failed to write register 0x%x with value 0x%x", reg_addr, reg_val);
	}
	/* Wait for the write to complete */
	k_sleep(K_MSEC(1));
	return err;
}

const struct tmc_bus_io tmc_uart_bus_io = {
	.check = tmc_bus_check_uart,
	.read = tmc_bus_read_uart,
	.write = tmc_bus_write_uart,
};

#endif /* CONFIG_STEPPER_ADI_TMC_UART */
