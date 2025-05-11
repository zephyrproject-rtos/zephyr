/**
 * @file drivers/stepper/adi/stepper.h
 *
 * @brief Private API for Trinamic SPI bus
 *
 */

/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
 * SPDX-FileCopyrightText: Copyright (c) 2024 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_STEPPER_CONTROL_ADI_TMC_SPI_H_
#define ZEPHYR_INCLUDE_DRIVERS_STEPPER_CONTROL_ADI_TMC_SPI_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief TMC SPI INTERFACE
 * @ingroup io_priv_interfaces
 * @{
 *
 */

#include <zephyr/drivers/spi.h>

int tmc50xx_read(const struct device *dev, const uint8_t reg_addr, uint32_t *reg_val);

int tmc50xx_write(const struct device *dev, const uint8_t reg_addr, const uint32_t reg_val);

/**
 * @brief Read a register from the TMC module using the SPI Bus.
 *
 * @param bus SPI DT information of the bus.
 * @param read_address_mask Address Mask for read operation.
 * @param register_address Register.
 * @param data Pointer to read value.
 *
 * @return a value from spi_transceive().
 */
int tmc_spi_read_register(const struct spi_dt_spec *bus, const uint8_t read_address_mask,
			  const uint8_t register_address, uint32_t *data);

/**
 * @brief Write into a register in the TMC module using the SPI Bus.
 *
 * @param bus SPI DT information of the bus.
 * @param write_bit Write bit for write operation.
 * @param register_address Register.
 * @param data Value to be written in the register.
 *
 * @return a value from spi_transceive().
 */
int tmc_spi_write_register(const struct spi_dt_spec *bus, const uint8_t write_bit,
			   const uint8_t register_address, const uint32_t data);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_STEPPER_CONTROL_ADI_TMC_SPI_H_ */
