/*
 * Copyright (c) 2023 Google LLC
 * Copyright (c) 2024 Croxel Inc.
 * Copyright (c) 2024 Florian Weber <Florian.Weber@live.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/rtio/work.h>

#include "akm09918c.h"

LOG_MODULE_DECLARE(AKM09918C, CONFIG_SENSOR_LOG_LEVEL);

void akm09918c_submit_sync(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct device *dev = cfg->sensor;
	struct akm09918c_data *data = dev->data;
	const struct sensor_chan_spec *const channels = cfg->channels;
	const size_t num_channels = cfg->count;
	int rc;

	/* Check if the requested channels are supported */
	for (size_t i = 0; i < num_channels; i++) {
		switch (channels[i].chan_type) {
		case SENSOR_CHAN_MAGN_X:
		case SENSOR_CHAN_MAGN_Y:
		case SENSOR_CHAN_MAGN_Z:
		case SENSOR_CHAN_MAGN_XYZ:
		case SENSOR_CHAN_ALL:
			break;
		default:
			LOG_ERR("Unsupported channel type %d", channels[i].chan_type);
			rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
			return;
		}
	}

	/* start the measurement in the sensor */
	rc = akm09918c_start_measurement(dev, SENSOR_CHAN_MAGN_XYZ);
	if (rc != 0) {
		LOG_ERR("Failed to fetch samples.");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	/* save information for the work item */
	data->work_ctx.timestamp = k_ticks_to_ns_floor64(k_uptime_ticks());
	data->work_ctx.iodev_sqe = iodev_sqe;

	rc = k_work_schedule(&data->work_ctx.async_fetch_work, K_USEC(AKM09918C_MEASURE_TIME_US));
	if (rc == 0) {
		LOG_ERR("The last fetch has not finished yet. "
			"Try again later when the last sensor read operation has finished.");
		rtio_iodev_sqe_err(iodev_sqe, -EBUSY);
	}
	return;
}

void akm09918c_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct rtio_work_req *req = rtio_work_req_alloc();

	if (req == NULL) {
		LOG_ERR("RTIO work item allocation failed. Consider to increase "
			"CONFIG_RTIO_WORKQ_POOL_ITEMS.");
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	rtio_work_req_submit(req, iodev_sqe, akm09918c_submit_sync);
}

void akm09918_async_fetch(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct akm09918c_async_fetch_ctx *ctx =
		CONTAINER_OF(dwork, struct akm09918c_async_fetch_ctx, async_fetch_work);
	const struct sensor_read_config *cfg = ctx->iodev_sqe->sqe.iodev->data;
	const struct device *dev = cfg->sensor;
	uint32_t req_buf_len = sizeof(struct akm09918c_encoded_data);
	uint32_t buf_len;
	uint8_t *buf;
	struct akm09918c_encoded_data *edata;
	int rc;

	/* Get the buffer for the frame, it may be allocated dynamically by the rtio context */
	rc = rtio_sqe_rx_buf(ctx->iodev_sqe, req_buf_len, req_buf_len, &buf, &buf_len);
	if (rc != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", req_buf_len);
		rtio_iodev_sqe_err(ctx->iodev_sqe, rc);
		return;
	}
	edata = (struct akm09918c_encoded_data *)buf;
	rc = akm09918c_fetch_measurement(dev, &edata->readings[0], &edata->readings[1],
					 &edata->readings[2]);
	if (rc != 0) {
		rtio_iodev_sqe_err(ctx->iodev_sqe, rc);
		return;
	}
	rtio_iodev_sqe_ok(ctx->iodev_sqe, 0);
}
