/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>

struct sensor_virtual_config {
};

struct sensor_virtual_data {
};

static const struct sensor_driver_api sensor_virtual_driver_api;

static int sensor_virtual_init(const struct device *dev)
{
	return 0;
}

#define SENSOR_VIRTUAL_INIT(n)						\
	static const struct sensor_virtual_config			\
		sensor_virtual_config_##n;				\
	static struct sensor_virtual_data sensor_virtual_data_##n;	\
									\
	SENSOR_DEVICE_DT_INST_DEFINE(n,					\
				     sensor_virtual_init,		\
				     NULL,				\
				     &sensor_virtual_data_##n,		\
				     &sensor_virtual_config_##n,	\
				     POST_KERNEL,			\
				     CONFIG_SENSOR_INIT_PRIORITY,	\
				     &sensor_virtual_driver_api);

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_sensor_hinge_angle
DT_INST_FOREACH_STATUS_OKAY(SENSOR_VIRTUAL_INIT)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_sensor_kalman_filter
DT_INST_FOREACH_STATUS_OKAY(SENSOR_VIRTUAL_INIT)
