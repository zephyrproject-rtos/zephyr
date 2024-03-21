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

static void sensing_iodev_submit(struct rtio_iodev_sqe *iodev_sqe)
{
	struct sensing_sensor *sensor = (struct sensing_sensor *)iodev_sqe->sqe.userdata;
	const struct device *dev = sensor->dev;
	const struct sensor_driver_api *api = dev->api;

	if (api->submit != NULL) {
		api->submit(dev, iodev_sqe);
	} else {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
	}
}

const struct rtio_iodev_api __sensing_iodev_api = {
	.submit = sensing_iodev_submit,
};

int sensing_sensor_get_reporters(const struct device *dev, int type,
		sensing_sensor_handle_t *reporter_handles,
		int max_handles)
{
	struct sensing_sensor *sensor = get_sensor_by_dev(dev);
	int i, num = 0;

	for (i = 0; i < sensor->reporter_num && num < max_handles; ++i) {
		if (type == sensor->conns[i].source->info->type
				|| type == SENSING_SENSOR_TYPE_ALL) {
			reporter_handles[num] = &sensor->conns[i];
			num++;
		}
	}

	return num;
}

int sensing_sensor_get_reporters_count(const struct device *dev, int type)
{
	struct sensing_sensor *sensor = get_sensor_by_dev(dev);
	int i, num = 0;

	for (i = 0; i < sensor->reporter_num; ++i) {
		if (type == sensor->conns[i].source->info->type
				|| type == SENSING_SENSOR_TYPE_ALL) {
			num++;
		}
	}

	return num;
}
