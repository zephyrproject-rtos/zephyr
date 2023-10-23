/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_sensing_phy_3d_sensor

#include <stdlib.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sensing/sensing.h>
#include <zephyr/sensing/sensing_sensor.h>
#include <zephyr/sys/util.h>

#include "phy_3d_sensor.h"

LOG_MODULE_REGISTER(phy_3d_sensor, CONFIG_SENSING_LOG_LEVEL);

static int phy_3d_sensor_init(const struct device *dev)
{
	const struct phy_3d_sensor_config *cfg = dev->config;

	LOG_INF("%s: Underlying device: %s", dev->name, cfg->hw_dev->name);

	return 0;
}

static int phy_3d_sensor_attr_set(const struct device *dev,
		enum sensor_channel chan,
		enum sensor_attribute attr,
		const struct sensor_value *val)
{
	return 0;
}

static int phy_3d_sensor_submit(const struct device *dev,
		struct rtio_iodev_sqe *sqe)
{
	return 0;
}

static const struct sensor_driver_api phy_3d_sensor_api = {
	.attr_set = phy_3d_sensor_attr_set,
	.submit = phy_3d_sensor_submit,
};

static const struct sensing_sensor_register_info phy_3d_sensor_reg = {
	.flags = SENSING_SENSOR_FLAG_REPORT_ON_CHANGE,
	.sample_size = sizeof(struct sensing_sensor_value_3d_q31),
	.sensitivity_count = PHY_3D_SENSOR_CHANNEL_NUM,
	.version.value = SENSING_SENSOR_VERSION(0, 8, 0, 0),
};

#define SENSING_PHY_3D_SENSOR_DT_DEFINE(_inst)				\
	static struct phy_3d_sensor_data _CONCAT(data, _inst);		\
	static const struct phy_3d_sensor_config _CONCAT(cfg, _inst) = {\
		.hw_dev = DEVICE_DT_GET(				\
				DT_PHANDLE(DT_DRV_INST(_inst),		\
				underlying_device)),			\
		.sensor_type = DT_PROP(DT_DRV_INST(_inst), sensor_type),\
	};								\
	SENSING_SENSOR_DT_INST_DEFINE(_inst, &phy_3d_sensor_reg, NULL,	\
		&phy_3d_sensor_init, NULL,				\
		&_CONCAT(data, _inst), &_CONCAT(cfg, _inst),		\
		POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,		\
		&phy_3d_sensor_api);

DT_INST_FOREACH_STATUS_OKAY(SENSING_PHY_3D_SENSOR_DT_DEFINE);
