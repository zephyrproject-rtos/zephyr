/*
 * Copyright (c) 2020 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef ZEPHYR_DRIVERS_SENSOR_ICM42605_ICM42605_SPI_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM42605_ICM42605_SPI_H_

#include <device.h>

int inv_spi_single_write(uint8_t reg, uint8_t *data);
int inv_spi_read(uint8_t reg, uint8_t *data, size_t len);
int icm42605_spi_init(const struct device *spi_device,
		      const struct spi_config *spi_config);

#endif /* __SENSOR_ICM42605_ICM42605_SPI__ */
