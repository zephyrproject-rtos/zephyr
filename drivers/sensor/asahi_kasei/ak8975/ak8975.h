/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_AK8975_AK8975_H_
#define ZEPHYR_DRIVERS_SENSOR_AK8975_AK8975_H_

#include <zephyr/device.h>

#define AK8975_REG_CHIP_ID		0x00
#define AK8975_CHIP_ID			0x48

#define AK8975_REG_DATA_START		0x03

#define AK8975_REG_CNTL			0x0A
#define AK8975_MODE_MEASURE		0x01
#define AK8975_MODE_FUSE_ACCESS		0x0F

#define AK8975_REG_ADJ_DATA_START	0x10

#define AK8975_MEASURE_TIME_US		9000
#define AK8975_MICRO_GAUSS_PER_BIT	3000

struct ak8975_data {
	int16_t x_sample;
	int16_t y_sample;
	int16_t z_sample;

	uint8_t x_adj;
	uint8_t y_adj;
	uint8_t z_adj;
};

struct ak8975_config {
	struct i2c_dt_spec i2c;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_AK8975_AK8975_H_ */
