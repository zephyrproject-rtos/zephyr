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
#define PHY_3D_SENSOR_SLOPE_DURATION 2

struct phy_3d_sensor_custom {
	const enum sensor_channel chan_all;
	const int8_t shift;
	void (*q31_to_sensor_value)(q31_t q31, struct sensor_value *val);
	q31_t (*sensor_value_to_q31)(struct sensor_value *val);
};

struct phy_3d_sensor_context {
	const struct device *dev;
	const struct device *hw_dev;
	const int32_t sensor_type;
	const struct phy_3d_sensor_custom *custom;
	struct sensor_trigger trig;
	bool data_ready_enabled;
	bool data_ready_support;
	uint32_t interval;
	uint32_t sensitivities[PHY_3D_SENSOR_CHANNEL_NUM];
};

#endif
