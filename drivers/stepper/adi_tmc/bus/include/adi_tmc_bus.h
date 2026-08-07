/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Dipak Shetty
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_STEPPER_ADI_TMC_BUS_H_
#define ZEPHYR_DRIVERS_STEPPER_ADI_TMC_BUS_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>

/* Communication interface types */
#define TMC_COMM_SPI  0
#define TMC_COMM_UART 1

union tmc_bus {
	struct spi_dt_spec spi;
	const struct device *uart;
};

/**
 * @brief Function pointer type for bus check function.
 * @param bus Pointer to the bus structure.
 * @param comm_type Communication type (SPI or UART).
 * @return 0 if bus is ready, negative error code otherwise.
 */
typedef int (*tmc_bus_check_fn)(const union tmc_bus *bus, uint8_t comm_type);

/**
 * @brief Function pointer type for register read function.
 * @param dev Pointer to the device structure.
 * @param reg Register address to read.
 * @param val Pointer to store the read value.
 * @return 0 on success, negative error code otherwise.
 */
typedef int (*tmc_reg_read_fn)(const struct device *dev, uint8_t reg, uint32_t *val);

/**
 * @brief Function pointer type for register write function.
 * @param dev Pointer to the device structure.
 * @param reg Register address to write.
 * @param val Value to write to the register.
 * @return 0 on success, negative error code otherwise.
 */
typedef int (*tmc_reg_write_fn)(const struct device *dev, uint8_t reg, uint32_t val);

/**
 * @brief Structure for bus I/O operations.
 * Contains function pointers for bus check, register read, and register write.
 */
struct tmc_bus_io {
	tmc_bus_check_fn check; /* Function to check if the bus is ready */
	tmc_reg_read_fn read;   /* Function to read a register */
	tmc_reg_write_fn write; /* Function to write to a register */
};

#endif /* ZEPHYR_DRIVERS_STEPPER_ADI_TMC_BUS_H_ */
