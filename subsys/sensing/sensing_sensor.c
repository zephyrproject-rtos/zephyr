/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sensing/sensing.h>
#include <zephyr/sensing/sensing_sensor.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(sensing, CONFIG_SENSING_LOG_LEVEL);

int sensing_sensor_notify_data_ready(const struct device *dev)
{
	return -ENOTSUP;
}

int sensing_sensor_set_data_ready(const struct device *dev, bool data_ready)
{
	return -ENOTSUP;
}

int sensing_sensor_post_data(const struct device *dev, void *buf, int size)
{
	return -ENOTSUP;
}
