/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_sensing_phy_3d_sensor

#include <stdlib.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sensing/sensing.h>
#include <zephyr/sensing/sensing_sensor.h>
#include <zephyr/sys/util.h>

#include "phy_3d_sensor.h"

LOG_MODULE_REGISTER(phy_3d_sensor, CONFIG_SENSING_LOG_LEVEL);

static int phy_3d_sensor_init(const struct device *dev,
		const struct sensing_sensor_info *info,
		const sensing_sensor_handle_t *reporter_handles,
		int reporters_count)
{
	return 0;
}

static int phy_3d_sensor_deinit(const struct device *dev)
{
	return 0;
}

static int phy_3d_sensor_read_sample(const struct device *dev,
		void *buf, int size)
{
	return 0;
}

static int phy_3d_sensor_sensitivity_test(const struct device *dev,
		int index, uint32_t sensitivity,
		void *last_sample_buf, int last_sample_size,
		void *current_sample_buf, int current_sample_size)
{
	return 0;
}

static int phy_3d_sensor_set_interval(const struct device *dev, uint32_t value)
{
	return 0;
}

static int phy_3d_sensor_get_interval(const struct device *dev,
		uint32_t *value)
{
	return 0;
}

static int phy_3d_sensor_set_sensitivity(const struct device *dev,
		int index, uint32_t value)
{
	return 0;
}

static int phy_3d_sensor_get_sensitivity(const struct device *dev,
		int index, uint32_t *value)
{
	return 0;
}

static const struct sensing_sensor_api phy_3d_sensor_api = {
	.init = phy_3d_sensor_init,
	.deinit = phy_3d_sensor_deinit,
	.set_interval = phy_3d_sensor_set_interval,
	.get_interval = phy_3d_sensor_get_interval,
	.set_sensitivity = phy_3d_sensor_set_sensitivity,
	.get_sensitivity = phy_3d_sensor_get_sensitivity,
	.read_sample = phy_3d_sensor_read_sample,
	.sensitivity_test = phy_3d_sensor_sensitivity_test,
};

static const struct sensing_sensor_register_info phy_3d_sensor_reg = {
	.flags = SENSING_SENSOR_FLAG_REPORT_ON_CHANGE,
	.sample_size = sizeof(struct sensing_sensor_value_3d_q31),
	.sensitivity_count = PHY_3D_SENSOR_CHANNEL_NUM,
	.version.value = SENSING_SENSOR_VERSION(0, 8, 0, 0),
};

#define SENSING_PHY_3D_SENSOR_DT_DEFINE(_inst)				\
	static struct phy_3d_sensor_context _CONCAT(ctx, _inst) = {	\
		.hw_dev = DEVICE_DT_GET(				\
				DT_PHANDLE(DT_DRV_INST(_inst),		\
				underlying_device)),			\
		.sensor_type = DT_PROP(DT_DRV_INST(_inst), sensor_type),\
	};								\
	SENSING_SENSOR_DT_DEFINE(DT_DRV_INST(_inst),			\
		&phy_3d_sensor_reg, &_CONCAT(ctx, _inst),		\
		&phy_3d_sensor_api);

DT_INST_FOREACH_STATUS_OKAY(SENSING_PHY_3D_SENSOR_DT_DEFINE);
