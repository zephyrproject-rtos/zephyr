/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/work.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "icm20689.h"
#include "icm20689_decoder.h"
#include "icm20689_reg.h"
#include "icm20689_rtio.h"
#include "icm20689_spi.h"
#include "icm20689_trigger.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ICM20689_RTIO, CONFIG_SENSOR_LOG_LEVEL);

#define ICM20689_OUTPUT_DATA_SIZE                                                                  \
	(ICM20689_ACCEL_DATA_SIZE + ICM20689_TEMP_DATA_SIZE + ICM20689_GYRO_DATA_SIZE)

static int icm20689_channel_mask(struct sensor_chan_spec channel, uint8_t *mask)
{
	if (channel.chan_idx != 0) {
		return -ENOTSUP;
	}

	switch (channel.chan_type) {
	case SENSOR_CHAN_DIE_TEMP:
		*mask |= BIT(ICM20689_ENCODED_CHAN_DIE_TEMP);
		return 0;
	case SENSOR_CHAN_ACCEL_X:
		*mask |= BIT(ICM20689_ENCODED_CHAN_ACCEL_X);
		return 0;
	case SENSOR_CHAN_ACCEL_Y:
		*mask |= BIT(ICM20689_ENCODED_CHAN_ACCEL_Y);
		return 0;
	case SENSOR_CHAN_ACCEL_Z:
		*mask |= BIT(ICM20689_ENCODED_CHAN_ACCEL_Z);
		return 0;
	case SENSOR_CHAN_ACCEL_XYZ:
		*mask |= BIT(ICM20689_ENCODED_CHAN_ACCEL_X) | BIT(ICM20689_ENCODED_CHAN_ACCEL_Y) |
			 BIT(ICM20689_ENCODED_CHAN_ACCEL_Z);
		return 0;
	case SENSOR_CHAN_GYRO_X:
		*mask |= BIT(ICM20689_ENCODED_CHAN_GYRO_X);
		return 0;
	case SENSOR_CHAN_GYRO_Y:
		*mask |= BIT(ICM20689_ENCODED_CHAN_GYRO_Y);
		return 0;
	case SENSOR_CHAN_GYRO_Z:
		*mask |= BIT(ICM20689_ENCODED_CHAN_GYRO_Z);
		return 0;
	case SENSOR_CHAN_GYRO_XYZ:
		*mask |= BIT(ICM20689_ENCODED_CHAN_GYRO_X) | BIT(ICM20689_ENCODED_CHAN_GYRO_Y) |
			 BIT(ICM20689_ENCODED_CHAN_GYRO_Z);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int icm20689_prepare_encoded_data(const struct device *dev,
					 const struct sensor_read_config *read_cfg,
					 struct icm20689_encoded_data *encoded)
{
	const struct icm20689_config *cfg = dev->config;
	struct icm20689_data *data = dev->data;

	if (read_cfg->count == 0) {
		return -EINVAL;
	}

	memset(encoded, 0, sizeof(*encoded));
	for (size_t i = 0; i < read_cfg->count; i++) {
		int ret = icm20689_channel_mask(read_cfg->channels[i], &encoded->channels);

		if (ret != 0) {
			return ret;
		}
	}

	encoded->header.accel_fs = data->active.accel_fs;
	encoded->header.gyro_fs = data->active.gyro_fs;
	memcpy(encoded->header.axis_align, cfg->axis_align, sizeof(encoded->header.axis_align));

	return 0;
}

static int icm20689_read_encoded_data(const struct device *dev,
				      struct icm20689_encoded_data *encoded)
{
	const struct icm20689_config *cfg = dev->config;
	uint8_t raw[ICM20689_OUTPUT_DATA_SIZE];
	static const uint8_t register_offsets[ARRAY_SIZE(encoded->readings)] = {
		6, 0, 2, 4, 8, 10, 12,
	};
	int ret;

	ret = icm20689_spi_read(&cfg->spi, ICM20689_REG_ACCEL_XOUT_H, raw, sizeof(raw));
	if (ret != 0) {
		return ret;
	}

	for (size_t i = 0; i < ARRAY_SIZE(encoded->readings); i++) {
		encoded->readings[i] = (int16_t)sys_get_be16(&raw[register_offsets[i]]);
	}

	encoded->header.timestamp = k_ticks_to_ns_floor64(k_uptime_ticks());
	return 0;
}

static void icm20689_submit_one_shot_sync(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *read_cfg = iodev_sqe->sqe.iodev->data;
	const struct device *dev = read_cfg->sensor;
	uint8_t *buf;
	uint32_t buf_len;
	int ret;

	ret = rtio_sqe_rx_buf(iodev_sqe, sizeof(struct icm20689_encoded_data),
			      sizeof(struct icm20689_encoded_data), &buf, &buf_len);
	if (ret != 0) {
		LOG_ERR("Failed to get encoded sample buffer: %d", ret);
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}

	ARG_UNUSED(buf_len);
	struct icm20689_encoded_data *encoded = (void *)buf;

	icm20689_lock(dev);
	ret = icm20689_prepare_encoded_data(dev, read_cfg, encoded);
	if (ret == 0) {
		ret = icm20689_read_encoded_data(dev, encoded);
	}
	icm20689_unlock(dev);

	if (ret != 0) {
		LOG_ERR("Failed to fetch encoded sample: %d", ret);
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}

	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

static void icm20689_submit_one_shot(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct rtio_work_req *req = rtio_work_req_alloc();

	ARG_UNUSED(dev);
	if (req == NULL) {
		LOG_ERR("RTIO work item allocation failed");
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	rtio_work_req_submit(req, iodev_sqe, icm20689_submit_one_shot_sync);
}

void icm20689_submit(const struct device *sensor, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *read_cfg = iodev_sqe->sqe.iodev->data;

	ARG_UNUSED(sensor);
	if (read_cfg->is_streaming) {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
		return;
	}

	icm20689_submit_one_shot(read_cfg->sensor, iodev_sqe);
}
