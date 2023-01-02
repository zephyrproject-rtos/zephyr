/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sensor_virtual

#include <zephyr/drivers/sensor.h>

struct sensor_virtual_config {
};

struct sensor_virtual_data {
};

static int sensor_virtual_sample_fetch(const struct device *dev,
		enum sensor_channel chan)
{
	return -ENOTSUP;
}

static int sensor_virtual_channel_get(const struct device *dev,
		enum sensor_channel chan, struct sensor_value *val)
{
	return -ENOTSUP;
}

static const struct sensor_driver_api sensor_virtual_driver_api = {
	.sample_fetch = sensor_virtual_sample_fetch,
	.channel_get = sensor_virtual_channel_get,
};

static int sensor_virtual_init(const struct device *dev)
{
	return 0;
}

#define SENSOR_VIRTUAL_INIT(n)						\
	static const struct sensor_virtual_config			\
		sensor_virtual_config_##n;				\
									\
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

DT_INST_FOREACH_STATUS_OKAY(SENSOR_VIRTUAL_INIT)
