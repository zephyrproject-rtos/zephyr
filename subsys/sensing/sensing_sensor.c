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
#include "sensor_mgmt.h"

LOG_MODULE_DECLARE(sensing, CONFIG_SENSING_LOG_LEVEL);

int sensing_sensor_notify_data_ready(const struct device *dev)
{
	struct sensing_sensor *sensor = get_sensor_by_dev(dev);
	struct sensing_context *ctx = get_sensing_ctx();

	__ASSERT(sensor, "sensing sensor notify data ready, sensing_sensor is NULL");

	LOG_INF("sensor:%s notify data ready, sensor_mode:%d", sensor->dev->name, sensor->mode);

	if (sensor->mode != SENSOR_TRIGGER_MODE_DATA_READY) {
		LOG_ERR("sensor:%s not in data ready mode", sensor->dev->name);
		return -EINVAL;
	}

	atomic_set_bit(&sensor->flag, SENSOR_DATA_READY_BIT);

	atomic_set_bit(&ctx->event_flag, EVENT_DATA_READY);
	k_sem_give(&ctx->event_sem);

	return 0;
}

int sensing_sensor_set_data_ready(const struct device *dev, bool data_ready)
{
	struct sensing_sensor *sensor = get_sensor_by_dev(dev);

	__ASSERT(sensor, "sensing sensor set data ready, sensing_sensor is NULL");

	sensor->mode = data_ready ? SENSOR_TRIGGER_MODE_DATA_READY : SENSOR_TRIGGER_MODE_POLLING;
	LOG_INF("set data ready, sensor:%s, data_ready:%d, trigger_mode:%d",
		sensor->dev->name, data_ready, sensor->mode);

	return 0;
}

int sensing_sensor_post_data(const struct device *dev, void *buf, int size)
{
	return -ENOTSUP;
}
