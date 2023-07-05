/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sensing/sensing.h>
#include <zephyr/sensing/sensor.h>
#include <zephyr/logging/log.h>

#include "sensing/internal/sensing.h"

LOG_MODULE_REGISTER(sensing_pipe, CONFIG_SENSING_LOG_LEVEL);

#define DT_DRV_COMPAT zephyr_sensing_pipe

struct sensor_pipe_config {
	const struct sensor_info *parent_info;
};

struct sensor_pipe_data {
	struct rtio_iodev *oneshot_iodev;
};

static int attribute_set(const struct device *dev, enum sensor_channel chan,
			 enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct sensor_pipe_config *cfg = dev->config;

	LOG_DBG("Updating '%s' @%p", cfg->parent_info->dev->name, cfg->parent_info->dev);
	return sensor_attr_set(cfg->parent_info->dev, chan, attr, val);
}

static int submit(const struct device *sensor, struct rtio_iodev_sqe *sqe)
{
	struct sensor_pipe_data *data = sensor->data;

	const struct sensing_sensor_info *info = sqe->sqe.userdata;

	LOG_DBG("Trying to read %s [%d] type=%d", info->info->dev->name,
		(int)(info - STRUCT_SECTION_START(sensing_sensor_info)), info->type);

	int rc = sensor_read(data->oneshot_iodev, &sensing_rtio_ctx, sqe->sqe.userdata);

	if (rc == 0) {
		rtio_iodev_sqe_ok(sqe, 0);
	} else {
		rtio_iodev_sqe_err(sqe, rc);
	}
	return rc;
}

SENSING_DMEM static const struct sensor_driver_api sensor_pipe_api = {
	.attr_set = attribute_set,
	.attr_get = NULL,
	.get_decoder = NULL,
	.submit = submit,
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
	SENSOR_DT_READ_IODEV(underlying_reader_##inst, DT_INST_PHANDLE(inst, dev),                 \
			     SENSOR_CHAN_ALL);                                                     \
	static struct sensor_pipe_data data_##inst = {                                             \
		.oneshot_iodev = &underlying_reader_##inst,                                        \
	};                                                                                         \
	SENSING_SENSOR_DT_INST_DEFINE(inst, sensing_sensor_pipe_init, NULL, &data_##inst,          \
				      &cfg_##inst, APPLICATION, 10, &sensor_pipe_api);

DT_INST_FOREACH_STATUS_OKAY(SENSING_PIPE_INIT)
