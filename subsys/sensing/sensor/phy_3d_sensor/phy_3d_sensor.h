/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SPHY_3D_SENSOR_H_
#define ZEPHYR_INCLUDE_SPHY_3D_SENSOR_H_

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#define PHY_3D_SENSOR_CHANNEL_NUM 3

struct phy_3d_sensor_custom {
	const enum sensor_channel chan_all;
	const int8_t shift;
	void (*q31_to_sensor_value)(q31_t q31, struct sensor_value *val);
	q31_t (*sensor_value_to_q31)(struct sensor_value *val);
};

struct phy_3d_sensor_data {
	struct sensor_value sensitivities[PHY_3D_SENSOR_CHANNEL_NUM];
	const struct phy_3d_sensor_custom **customs;
	struct rtio_iodev_sqe *sqes;
};

struct phy_3d_sensor_config {
	const struct device *hw_dev;
	const int sensor_num;
	const int32_t sensor_types[];
};

#endif
