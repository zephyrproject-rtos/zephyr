/*
 * Copyright (c) 2026 Sensirion
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_STS4x_STS4x_H_
#define ZEPHYR_DRIVERS_SENSOR_STS4x_STS4x_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

#define STS4X_I2C_ADDR_44 0x44
#define STS4X_I2C_ADDR_45 0x45
#define STS4X_I2C_ADDR_46 0x46

typedef enum {
	STS4X_MEASURE_TEMPERATURE_HIGH_PRECISION_CMD_ID = 0xfd,
	STS4X_MEASURE_TEMPERATURE_MEDIUM_PRECISION_CMD_ID = 0xf6,
	STS4X_MEASURE_TEMPERATURE_LOW_PRECISION_CMD_ID = 0xe0,
	STS4X_READ_SERIAL_NUMBER_CMD_ID = 0x89,
	STS4X_SOFT_RESET_CMD_ID = 0x94,
} STS4X_CMD_ID;

struct sts4x_config {
	struct i2c_dt_spec bus;
	uint8_t repeatability;
};

struct sts4x_data {
	uint16_t temperature_ticks;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_STS4x_STS4x_H_*/
