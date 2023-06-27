/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sensing/sensing.h>
#include <zephyr/sensing/sensor.h>

#define DT_DRV_COMPAT zephyr_sensing_pipe

static const struct sensor_driver_api sensor_pipe_api = {
	.attr_set = NULL,
	.attr_get = NULL,
	.get_decoder = NULL,
	.submit = NULL,
};

static int sensing_sensor_pipe_init(const struct device *dev)
{
	return 0;
}

#define SENSING_PIPE_INIT(inst)                                                                    \
	SENSING_SENSOR_DT_INST_DEFINE(inst, sensing_sensor_pipe_init, NULL, NULL, NULL,            \
				      APPLICATION, 10, &sensor_pipe_api);

DT_INST_FOREACH_STATUS_OKAY(SENSING_PIPE_INIT)
