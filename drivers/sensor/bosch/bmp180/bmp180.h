/*
 * Driver for Bosch BMP180 digital pressure and temperature sensor
 *
 * Copyright (C) 2024 Jakov Jovic <jovic.jakov@outlook.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMP180_BMP180_H_
#define ZEPHYR_DRIVERS_SENSOR_BMP180_BMP180_H_

#include <zephyr/sys/util.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>

/* Generic log string */
#define ERROR_MSG_FMT "Failed to %s the %s."

struct bmp180_data {
	int16_t ac1;
	int16_t ac2;
	int16_t ac3;
	uint16_t ac4;
	uint16_t ac5;
	uint16_t ac6;
	int16_t b1;
	int16_t b2;
	int16_t mb;
	int16_t mc;
	int16_t md;

	int16_t uncomp_temp;
	int32_t uncomp_press;
	int32_t temp;
	int32_t press;
	int32_t alt;
};

enum bmp180_oversampling_setting {
	BMP180_MODE_ULTRA_LOW_POWER = 0,   /* 1 sample, 4.5 ms */
	BMP180_MODE_STANDARD,              /* 2 samples, 7.5 ms */
	BMP180_MODE_HIGH_RESOLUTION,       /* 4 samples, 13.5 ms */
	BMP180_MODE_ULTRA_HIGH_RESOLUTION, /* 8 samples, 25.5 ms */
};

struct bmp180_config {
	struct i2c_dt_spec i2c;
	enum bmp180_oversampling_setting oversampling;
};

/* Register definitions */
#define BMP180_REG_OUT_XLSB   0xF8
#define BMP180_REG_OUT_LSB    0xF7
#define BMP180_REG_OUT_MSB    0xF6
#define BMP180_REG_CTRL_MEAS  0xF4
#define BMP180_REG_CTRL_RESET 0xE0
#define BMP180_REG_CHIP_ID    0xD0
#define BMP180_REG_CALIB      0xAA

/* Chip ID value stored in BMP180_REG_CHIP_ID */
#define BMP180_CHIP_ID 0x55

/* Value for BMP180_REG_CTRL_RESET */
#define BMP180_RESET_VALUE 0xB6

/* Read values for BMP180_REG_CTRL_MEAS */
#define BMP180_CMD_READ_TEMP  0x2E
#define BMP180_CMD_READ_PRESS 0x34

#endif /* ZEPHYR_DRIVERS_SENSOR_BMP180_BMP180_H_ */
