/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sensing/sensing.h>
#include <zephyr/sensing/sensor.h>
#include <zephyr/sensing/transform.h>
#include <zephyr/logging/log.h>

#include "sensing/internal/sensing.h"

LOG_MODULE_REGISTER(sensing_pipe, CONFIG_SENSING_LOG_LEVEL);

#define DT_DRV_COMPAT zephyr_sensing_pipe

struct sensor_pipe_config {
	const struct sensor_info *parent_info;
	struct rtio *rtio_ctx;
	const struct sensor_decoder_api *decoder;
};

struct sensor_pipe_data {
	struct rtio_iodev *oneshot_iodev;
	uint8_t *data;
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
	const struct sensor_pipe_config *config = sensor->config;
	struct sensor_pipe_data *data = sensor->data;

	const struct sensing_sensor_info *info = sqe->sqe.userdata;

	LOG_DBG("Trying to read %s [%d] type=%d", info->info->dev->name,
		(int)(info - STRUCT_SECTION_START(sensing_sensor_info)), info->type);

	LOG_DBG("RTIO ctx %p", (void*)config->rtio_ctx);
	k_msleep(50);
	int rc = sensor_read(data->oneshot_iodev, config->rtio_ctx, sqe);

	if (rc != 0) {
		rtio_iodev_sqe_err(sqe, rc);
		return 0;
	}

	struct rtio_cqe cqe;

	rc = rtio_cqe_copy_out(config->rtio_ctx, &cqe, 1, K_FOREVER);
	if (rc != 1) {
		rtio_iodev_sqe_err(sqe, rc);
		return 0;
	}

	uint8_t *read_data;
	uint32_t read_data_len;

	rc = rtio_cqe_get_mempool_buffer(&sensing_rtio_ctx, &cqe, &read_data, &read_data_len);
	if (rc != 0) {
		rtio_iodev_sqe_err(sqe, rc);
		return 0;
	}

	uint8_t *buffer;
	uint32_t buffer_len;

	/* Decode the data here and post it as a result to the cqe */
	switch (info->type) {
	case SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D:
	case SENSING_SENSOR_TYPE_MOTION_UNCALIB_ACCELEROMETER_3D:
	case SENSING_SENSOR_TYPE_MOTION_GYROMETER_3D:
		rc = rtio_sqe_rx_buf(sqe, sizeof(struct sensing_sensor_three_axis_data),
				     sizeof(struct sensing_sensor_three_axis_data), &buffer,
				     &buffer_len);
		if (rc != 0) {
			rtio_iodev_sqe_err(sqe, rc);
			break;
		}
		rc = decode_three_axis_data(info->type,
					    (struct sensing_sensor_three_axis_data *)buffer,
					    config->decoder, read_data);
		if (rc != 0) {
			LOG_ERR("Failed to decode");
			rtio_iodev_sqe_err(sqe, rc);
		} else {
			rtio_iodev_sqe_ok(sqe, 0);
		}
		break;
	case SENSING_SENSOR_TYPE_MOTION_HINGE_ANGLE:
		rc = rtio_sqe_rx_buf(sqe, sizeof(struct sensing_sensor_float_data),
				     sizeof(struct sensing_sensor_float_data), &buffer,
				     &buffer_len);
		if (rc != 0) {
			rtio_iodev_sqe_err(sqe, rc);
			break;
		}
		rc = decode_float_data(info->type, (struct sensing_sensor_float_data *)buffer,
				       config->decoder, read_data);
		if (rc != 0) {
			LOG_ERR("Failed to decode");
			rtio_iodev_sqe_err(sqe, rc);
		} else {
			rtio_iodev_sqe_ok(sqe, 0);
		}
		break;
	default:
		LOG_ERR("Sensor type not supported");
		rtio_iodev_sqe_err(sqe, -ENOTSUP);
	}
	rtio_release_buffer(&sensing_rtio_ctx, read_data, read_data_len);
	return 0;
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
	RTIO_DEFINE_WITH_EXT_MEMPOOL(rtio_##inst, 4, 4, sensing_rtio_block_pool);                  \
	static const struct sensor_pipe_config cfg_##inst = {                                      \
		.parent_info = &SENSOR_INFO_DT_NAME(DT_INST_PHANDLE(inst, dev)),                   \
		.rtio_ctx = &rtio_##inst,                                                          \
		.decoder = SENSOR_DECODER_DT_GET(DT_INST_PHANDLE(inst, dev)),                      \
	};                                                                                         \
	SENSOR_DT_READ_IODEV(underlying_reader_##inst, DT_INST_PHANDLE(inst, dev),                 \
			     SENSOR_CHAN_ALL);                                                     \
	static struct sensor_pipe_data data_##inst = {                                             \
		.oneshot_iodev = &underlying_reader_##inst,                                        \
	};                                                                                         \
	SENSING_SENSOR_DT_INST_DEFINE(inst, sensing_sensor_pipe_init, NULL, &data_##inst,          \
				      &cfg_##inst, APPLICATION, 10, &sensor_pipe_api);

DT_INST_FOREACH_STATUS_OKAY(SENSING_PIPE_INIT)
