/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SPHY_3D_SENSOR_H_
#define ZEPHYR_INCLUDE_SPHY_3D_SENSOR_H_

#include <zephyr/device.h>

#define PHY_3D_SENSOR_CHANNEL_NUM 3

struct phy_3d_sensor_data {
	struct sensor_value sensitivities[PHY_3D_SENSOR_CHANNEL_NUM];
};

struct phy_3d_sensor_config {
	const struct device *hw_dev;
	const int32_t sensor_type;
};

#endif
