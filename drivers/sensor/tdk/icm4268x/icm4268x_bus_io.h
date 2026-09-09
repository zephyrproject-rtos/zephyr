/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM4268X_BUS_IO_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM4268X_BUS_IO_H_

#include <zephyr/device.h>

#define ICM4268X_BUS_SPI DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(invensense_icm4268x, spi)
#define ICM4268X_BUS_I2C DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(invensense_icm4268x, i2c)

#if ICM4268X_BUS_SPI
#include <zephyr/drivers/spi.h>
#endif
#if ICM4268X_BUS_I2C
#include <zephyr/drivers/i2c.h>
#endif

union icm4268x_bus_cfg {
#if ICM4268X_BUS_SPI
	struct spi_dt_spec spi;
#endif
#if ICM4268X_BUS_I2C
	struct i2c_dt_spec i2c;
#endif
};

struct icm4268x_bus_io {
	int (*check)(const union icm4268x_bus_cfg *bus);
	int (*read)(const union icm4268x_bus_cfg *bus, uint16_t reg, uint8_t *data, size_t len);
	int (*write)(const union icm4268x_bus_cfg *bus, uint16_t reg, uint8_t data);
	int (*update)(const union icm4268x_bus_cfg *bus, uint16_t reg, uint8_t mask, uint8_t data);
};

#if ICM4268X_BUS_SPI
extern const struct icm4268x_bus_io icm4268x_bus_io_spi;
#endif
#if ICM4268X_BUS_I2C
extern const struct icm4268x_bus_io icm4268x_bus_io_i2c;
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM4268X_BUS_IO_H_ */
