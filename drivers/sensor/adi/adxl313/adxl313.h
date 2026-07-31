/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADXL313_ADXL313_H_
#define ZEPHYR_DRIVERS_SENSOR_ADXL313_ADXL313_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/dt-bindings/sensor/adxl313.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#define ADXL313_REG_DEVID_AD     0x00
#define ADXL313_REG_DEVID1_AD    0x01
#define ADXL313_REG_PARTID       0x02
#define ADXL313_REG_SOFT_RESET   0x18
#define ADXL313_REG_BW_RATE      0x2c
#define ADXL313_REG_POWER_CTL    0x2d
#define ADXL313_REG_DATA_FORMAT  0x31
#define ADXL313_REG_DATAX0       0x32

#define ADXL313_DEVID_AD_VAL     0xad
#define ADXL313_DEVID1_AD_VAL    0x1d
#define ADXL313_PARTID_VAL       0xcb
#define ADXL313_RESET_KEY        0x52

#define ADXL313_POWER_CTL_MEASURE BIT(3)

#define ADXL313_DATA_FORMAT_FULL_RES BIT(3)
#define ADXL313_DATA_FORMAT_RANGE_MSK GENMASK(1, 0)

#define ADXL313_ODR_MSK GENMASK(3, 0)

/* Full-resolution scale: 1024 LSB/g for all ranges */
#define ADXL313_FULL_RES_LSB_PER_G 1024

enum adxl313_odr {
	ADXL313_ODR_6_25HZ = ADXL313_DT_ODR_6_25,
	ADXL313_ODR_12_5HZ = ADXL313_DT_ODR_12_5,
	ADXL313_ODR_25HZ = ADXL313_DT_ODR_25,
	ADXL313_ODR_50HZ = ADXL313_DT_ODR_50,
	ADXL313_ODR_100HZ = ADXL313_DT_ODR_100,
	ADXL313_ODR_200HZ = ADXL313_DT_ODR_200,
	ADXL313_ODR_400HZ = ADXL313_DT_ODR_400,
	ADXL313_ODR_800HZ = ADXL313_DT_ODR_800,
	ADXL313_ODR_1600HZ = ADXL313_DT_ODR_1600,
	ADXL313_ODR_3200HZ = ADXL313_DT_ODR_3200,
};

enum adxl313_range {
	ADXL313_RANGE_0_5G = ADXL313_DT_RANGE_0_5G,
	ADXL313_RANGE_1G = ADXL313_DT_RANGE_1G,
	ADXL313_RANGE_2G = ADXL313_DT_RANGE_2G,
	ADXL313_RANGE_4G = ADXL313_DT_RANGE_4G,
};

struct adxl313_data {
	int16_t x;
	int16_t y;
	int16_t z;
	enum adxl313_range range;
	enum adxl313_odr odr;
};

struct adxl313_config {
	struct i2c_dt_spec i2c;
	enum adxl313_range range;
	enum adxl313_odr odr;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_ADXL313_ADXL313_H_ */
