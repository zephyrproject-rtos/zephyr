/*
 * Copyright (c) 2025 LCKFB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_QMI8658C_QMI8658C_H_
#define ZEPHYR_DRIVERS_SENSOR_QMI8658C_QMI8658C_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

/* QMI8658C I2C address */
#define QMI8658C_I2C_ADDR		0x6A

/* QMI8658C register addresses */
#define QMI8658C_REG_WHO_AM_I		0x00
#define QMI8658C_REG_REVISION_ID	0x01
#define QMI8658C_REG_CTRL1		0x02
#define QMI8658C_REG_CTRL2		0x03
#define QMI8658C_REG_CTRL3		0x04
#define QMI8658C_REG_CTRL4		0x05
#define QMI8658C_REG_CTRL5		0x06
#define QMI8658C_REG_CTRL6		0x07
#define QMI8658C_REG_CTRL7		0x08
#define QMI8658C_REG_CTRL8		0x09
#define QMI8658C_REG_CTRL9		0x0A
#define QMI8658C_REG_STATUS0		0x2E
#define QMI8658C_REG_AX_L		0x35
#define QMI8658C_REG_AX_H		0x36
#define QMI8658C_REG_AY_L		0x37
#define QMI8658C_REG_AY_H		0x38
#define QMI8658C_REG_AZ_L		0x39
#define QMI8658C_REG_AZ_H		0x3A
#define QMI8658C_REG_GX_L		0x3B
#define QMI8658C_REG_GX_H		0x3C
#define QMI8658C_REG_GY_L		0x3D
#define QMI8658C_REG_GY_H		0x3E
#define QMI8658C_REG_GZ_L		0x3F
#define QMI8658C_REG_GZ_H		0x40
#define QMI8658C_REG_RESET		0x60

/* QMI8658C WHO_AM_I value */
#define QMI8658C_WHO_AM_I_VAL		0x05

/* QMI8658C reset value */
#define QMI8658C_RESET_VAL		0xB0

/* CTRL1: Auto increment address */
#define QMI8658C_CTRL1_AUTO_INC		BIT(6)

/* CTRL7: Enable accelerometer and gyroscope */
#define QMI8658C_CTRL7_ACC_EN		BIT(0)
#define QMI8658C_CTRL7_GYR_EN		BIT(1)

/* CTRL2: Accelerometer configuration bits */
#define QMI8658C_CTRL2_ACC_FS_MASK	GENMASK(1, 0)
#define QMI8658C_CTRL2_ACC_FS_SHIFT	0
#define QMI8658C_CTRL2_ACC_ODR_MASK	GENMASK(5, 2)
#define QMI8658C_CTRL2_ACC_ODR_SHIFT	2

/* CTRL3: Gyroscope configuration bits */
#define QMI8658C_CTRL3_GYR_FS_MASK	GENMASK(1, 0)
#define QMI8658C_CTRL3_GYR_FS_SHIFT	0
#define QMI8658C_CTRL3_GYR_ODR_MASK	GENMASK(5, 2)
#define QMI8658C_CTRL3_GYR_ODR_SHIFT	2

/* Accelerometer full-scale range values (in g) */
#define QMI8658C_ACC_FS_2G		2
#define QMI8658C_ACC_FS_4G		4
#define QMI8658C_ACC_FS_8G		8
#define QMI8658C_ACC_FS_16G		16

/* Gyroscope full-scale range values (in dps) */
#define QMI8658C_GYR_FS_125DPS		125
#define QMI8658C_GYR_FS_250DPS		250
#define QMI8658C_GYR_FS_512DPS		512
#define QMI8658C_GYR_FS_1000DPS		1000

/* ODR values (in Hz) */
#define QMI8658C_ODR_125HZ		125
#define QMI8658C_ODR_250HZ		250
#define QMI8658C_ODR_500HZ		500
#define QMI8658C_ODR_1000HZ		1000

/* STATUS0: Data ready bits */
#define QMI8658C_STATUS0_ACC_DRDY	BIT(0)
#define QMI8658C_STATUS0_GYR_DRDY	BIT(1)

struct qmi8658c_data {
	int16_t acc_x;
	int16_t acc_y;
	int16_t acc_z;
	int16_t gyr_x;
	int16_t gyr_y;
	int16_t gyr_z;
};

struct qmi8658c_config {
	struct i2c_dt_spec i2c;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_QMI8658C_QMI8658C_H_ */