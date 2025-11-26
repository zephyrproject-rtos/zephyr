/*
 * Copyright (c) 2025 LCKFB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_QMC5883L_QMC5883L_H_
#define ZEPHYR_DRIVERS_SENSOR_QMC5883L_QMC5883L_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

/* QMC5883L I2C address */
#define QMC5883L_I2C_ADDR		0x0D

/* QMC5883L register addresses */
#define QMC5883L_REG_XOUT_L		0x00
#define QMC5883L_REG_XOUT_H		0x01
#define QMC5883L_REG_YOUT_L		0x02
#define QMC5883L_REG_YOUT_H		0x03
#define QMC5883L_REG_ZOUT_L		0x04
#define QMC5883L_REG_ZOUT_H		0x05
#define QMC5883L_REG_STATUS		0x06
#define QMC5883L_REG_TOUT_L		0x07
#define QMC5883L_REG_TOUT_H		0x08
#define QMC5883L_REG_CTRL1		0x09
#define QMC5883L_REG_CTRL2		0x0A
#define QMC5883L_REG_FBR		0x0B
#define QMC5883L_REG_CHIPID		0x0D

/* QMC5883L CHIPID value */
#define QMC5883L_CHIPID_VAL		0xFF

/* STATUS: Data ready bit */
#define QMC5883L_STATUS_DRDY		BIT(0)

/* CTRL2: Reset bit */
#define QMC5883L_CTRL2_RESET		BIT(7)

/* CTRL1: Continuous mode, 50Hz */
#define QMC5883L_CTRL1_CONT_50HZ	0x05

struct qmc5883l_data {
	int16_t mag_x;
	int16_t mag_y;
	int16_t mag_z;
};

struct qmc5883l_config {
	struct i2c_dt_spec i2c;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_QMC5883L_QMC5883L_H_ */

