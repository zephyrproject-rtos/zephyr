/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Dipak Shetty
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_STEPPER_ADI_TMC_TMC5XXX_BUS_H
#define ZEPHYR_DRIVERS_STEPPER_ADI_TMC_TMC5XXX_BUS_H

#include <zephyr/drivers/gpio.h>
#include <adi_tmc_bus.h>
#include <zephyr/devicetree.h>

/* 
 * Custom macros to check if a compatible is on a specific bus 
 * These use the Kconfig variables that are already set by the build system
 */
#define TMC5XXX_BUS_SPI_CHECK(compat) \
    ((DT_HAS_COMPAT_STATUS_OKAY(compat)) && IS_ENABLED(CONFIG_STEPPER_ADI_TMC_SPI))

#define TMC5XXX_BUS_UART_CHECK(compat) \
    ((DT_HAS_COMPAT_STATUS_OKAY(compat)) && IS_ENABLED(CONFIG_STEPPER_ADI_TMC_UART))

/* Shared bus I/O operations for TMC5xxx devices */
#if defined(CONFIG_STEPPER_ADI_TMC_SPI)
extern const struct tmc_bus_io tmc5xxx_spi_bus_io;
#endif

#if defined(CONFIG_STEPPER_ADI_TMC_UART)
extern const struct tmc_bus_io tmc5xxx_uart_bus_io;
#endif

#endif /* ZEPHYR_DRIVERS_STEPPER_ADI_TMC_TMC5XXX_BUS_H */
