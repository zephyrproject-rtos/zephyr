/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_AK8975_AK8975_H_
#define ZEPHYR_DRIVERS_SENSOR_AK8975_AK8975_H_

#include <device.h>

#define AK8975_REG_CHIP_ID		0x00
#define AK8975_CHIP_ID			0x48

#define AK8975_REG_DATA_START		0x03

#define AK8975_REG_CNTL			0x0A
#define AK8975_MODE_MEASURE		0x01
#define AK8975_MODE_FUSE_ACCESS		0x0F

#define AK8975_REG_ADJ_DATA_START	0x10

#define AK8975_MEASURE_TIME_US		9000
#define AK8975_MICRO_GAUSS_PER_BIT	3000

#ifdef CONFIG_MPU9150
#if CONFIG_AK8975_I2C_ADDR != 0x0C
#error "I2C address must be 0x0C when AK8975 is part of a MPU9150 chip"
#endif

#ifdef CONFIG_MPU9150_I2C_ADDR
#define MPU9150_I2C_ADDR		CONFIG_MPU9150_I2C_ADDR
#else
#define MPU9150_I2C_ADDR		CONFIG_MPU6050_I2C_ADDR
#endif

#define MPU9150_REG_BYPASS_CFG		0x37
#define MPU9150_I2C_BYPASS_EN		BIT(1)

#define MPU9150_REG_PWR_MGMT1		0x6B
#define MPU9150_SLEEP_EN		BIT(6)

#endif /* CONFIG_MPU9150 */


struct ak8975_data {
	struct device *i2c;

	s16_t x_sample;
	s16_t y_sample;
	s16_t z_sample;

	u8_t x_adj;
	u8_t y_adj;
	u8_t z_adj;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_AK8975_AK8975_H_ */
