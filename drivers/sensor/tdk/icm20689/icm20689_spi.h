/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * Copyright (c) 2022 Esco Medical ApS
 * Copyright (c) 2020 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM20689_SPI_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM20689_SPI_H_

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>

/**
 * @brief Perform a single SPI write to an ICM20689 register.
 *
 * @param bus SPI bus specification
 * @param reg ICM20689 register address
 * @param data Data byte to write
 * @return 0 on success, negative error code otherwise
 */
int icm20689_spi_single_write(const struct spi_dt_spec *bus, uint8_t reg, uint8_t data);

/**
 * @brief Update selected bits in an ICM20689 register.
 *
 * @param bus SPI bus specification
 * @param reg ICM20689 register address
 * @param mask Bits to update
 * @param data New values for the selected bits
 * @return 0 on success, negative error code otherwise
 */
int icm20689_spi_update_register(const struct spi_dt_spec *bus, uint8_t reg, uint8_t mask,
				 uint8_t data);

/**
 * @brief Read one or more consecutive ICM20689 registers.
 *
 * @param bus SPI bus specification
 * @param reg First ICM20689 register address
 * @param data Destination buffer
 * @param len Number of bytes to read
 * @return 0 on success, negative error code otherwise
 */
int icm20689_spi_read(const struct spi_dt_spec *bus, uint8_t reg, uint8_t *data, size_t len);

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM20689_SPI_H_ */
