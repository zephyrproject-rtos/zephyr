/**
 * @file drivers/stepper/adi/stepper.h
 *
 * @brief Private API for Trinamic SPI bus
 *
 */

/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_STEPPER_ADI_TMC_SPI_H_
#define ZEPHYR_DRIVERS_STEPPER_ADI_TMC_SPI_H_

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

#endif /* ZEPHYR_DRIVERS_STEPPER_ADI_TMC_SPI_H_ */
