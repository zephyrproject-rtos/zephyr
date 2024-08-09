/*
 * Copyright (c) 2024 Chaim Zax
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADXL34X_SPI_H_
#define ZEPHYR_DRIVERS_SENSOR_ADXL34X_SPI_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#define ADXL34X_SPI_CFG                                                                            \
	(SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_TRANSFER_MSB)
#define ADXL34X_SPI_READ_BIT BIT(7) /**< Address value has a read bit */

#define ADXL34X_CONFIG_SPI(i)                                                                      \
	.spi = SPI_DT_SPEC_INST_GET(i, ADXL34X_SPI_CFG, 0U), .bus_init = &adxl34x_spi_init,        \
	.bus_write = &adxl34x_spi_write, .bus_read = &adxl34x_spi_read,                            \
	.bus_read_buf = &adxl34x_spi_read_buf

int adxl34x_spi_init(const struct device *dev);
int adxl34x_spi_write(const struct device *dev, uint8_t reg_addr, uint8_t reg_data);
int adxl34x_spi_read(const struct device *dev, uint8_t reg_addr, uint8_t *reg_data);
int adxl34x_spi_read_buf(const struct device *dev, uint8_t reg_addr, uint8_t *rx_buf, uint8_t size);

#endif /* ZEPHYR_DRIVERS_SENSOR_ADXL34X_SPI_H_ */
