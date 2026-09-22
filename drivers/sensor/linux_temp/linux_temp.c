/*
 * Copyright 2026 Bayrem Gharsellaoui
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_native_linux_temp

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#include "linux_temp_bottom.h"

struct linux_temp_data {
	int32_t temperature_mc;
};

struct linux_temp_config {
	const char *path;
};

static int linux_temp_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct linux_temp_config *config = dev->config;
	struct linux_temp_data *data = dev->data;

	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_DIE_TEMP)) {
		return -ENOTSUP;
	}

	if (linux_temp_read(config->path, &data->temperature_mc) < 0) {
		return -EIO;
	}

	return 0;
}

static int linux_temp_channel_get(const struct device *dev, enum sensor_channel chan,
				  struct sensor_value *val)
{
	struct linux_temp_data *data = dev->data;

	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_DIE_TEMP)) {
		return -ENOTSUP;
	}

	return sensor_value_from_milli(val, data->temperature_mc);
}

static DEVICE_API(sensor, linux_temp_api) = {
	.sample_fetch = linux_temp_sample_fetch,
	.channel_get = linux_temp_channel_get,
};

#define LINUX_TEMP_INIT(inst)                                                                      \
	static struct linux_temp_data linux_temp_data_##inst;                                      \
	static const struct linux_temp_config linux_temp_config_##inst = {                         \
		.path = DT_INST_PROP(inst, path),                                                  \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &linux_temp_data_##inst,                    \
				     &linux_temp_config_##inst, POST_KERNEL,                       \
				     CONFIG_SENSOR_INIT_PRIORITY, &linux_temp_api);

DT_INST_FOREACH_STATUS_OKAY(LINUX_TEMP_INIT)
