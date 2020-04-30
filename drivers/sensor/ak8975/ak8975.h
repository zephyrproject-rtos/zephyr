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

#if DT_NODE_HAS_PROP(DT_INST(0, invensense_mpu9150), reg)
#if DT_REG_ADDR(DT_INST(0, asahi_kasei_ak8975)) != 0x0C
#error "I2C address must be 0x0C when AK8975 is part of a MPU9150 chip"
#endif

#define MPU9150_I2C_ADDR		DT_REG_ADDR(DT_INST(0, invensense_mpu9150))

#define MPU9150_REG_BYPASS_CFG		0x37
#define MPU9150_I2C_BYPASS_EN		BIT(1)

#define MPU9150_REG_PWR_MGMT1		0x6B
#define MPU9150_SLEEP_EN		BIT(6)

#endif /* DT_NODE_HAS_PROP(DT_INST(0, invensense_mpu9150), reg) */


struct ak8975_data {
	const struct device *i2c;

	int16_t x_sample;
	int16_t y_sample;
	int16_t z_sample;

	uint8_t x_adj;
	uint8_t y_adj;
	uint8_t z_adj;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_AK8975_AK8975_H_ */
