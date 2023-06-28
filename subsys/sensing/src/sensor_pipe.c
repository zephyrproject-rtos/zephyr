/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sensing/sensing.h>
#include <zephyr/sensing/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sensing_pipe, CONFIG_SENSING_LOG_LEVEL);

#define DT_DRV_COMPAT zephyr_sensing_pipe

struct sensor_pipe_config {
	const struct sensor_info *parent_info;
};

static int attribute_set(const struct device *dev, enum sensor_channel chan,
			 enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct sensor_pipe_config *cfg = dev->config;

	LOG_DBG("Updating '%s' @%p", cfg->parent_info->dev->name, cfg->parent_info->dev);
	return sensor_attr_set(cfg->parent_info->dev, chan, attr, val);
}

static const struct sensor_driver_api sensor_pipe_api = {
	.attr_set = attribute_set,
	.attr_get = NULL,
	.get_decoder = NULL,
	.submit = NULL,
};

static int sensing_sensor_pipe_init(const struct device *dev)
{
	const struct sensor_pipe_config *cfg = dev->config;

	LOG_DBG("Initializing %p with underlying device %p", dev, cfg->parent_info->dev);
	return 0;
}

#define SENSING_PIPE_INIT(inst)                                                                    \
	static const struct sensor_pipe_config cfg_##inst = {                                      \
		.parent_info = &SENSOR_INFO_DT_NAME(DT_INST_PHANDLE(inst, dev)),                   \
	};                                                                                         \
	SENSING_SENSOR_DT_INST_DEFINE(inst, sensing_sensor_pipe_init, NULL, NULL, &cfg_##inst,     \
				      APPLICATION, 10, &sensor_pipe_api);

DT_INST_FOREACH_STATUS_OKAY(SENSING_PIPE_INIT)
