/*
 * Copyright (c) 2024 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/rtio/work.h>

#include "mmc56x3.h"

LOG_MODULE_DECLARE(MMC56X3, CONFIG_SENSOR_LOG_LEVEL);

void mmc56x3_submit_sync(struct rtio_iodev_sqe *iodev_sqe)
{
	uint32_t min_buf_len = sizeof(struct mmc56x3_encoded_data);
	int rc;
	uint8_t *buf;
	uint32_t buf_len;

	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct device *dev = cfg->sensor;
	const struct sensor_chan_spec *const channels = cfg->channels;
	const size_t num_channels = cfg->count;

	rc = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	if (rc != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buf_len);
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	struct mmc56x3_encoded_data *edata;

	edata = (struct mmc56x3_encoded_data *)buf;
	edata->header.timestamp = k_ticks_to_ns_floor64(k_uptime_ticks());
	edata->has_temp = 0;
	edata->has_magn_x = 0;
	edata->has_magn_y = 0;
	edata->has_magn_z = 0;

	/* Check if the requested channels are supported */
	for (size_t i = 0; i < num_channels; i++) {
		switch (channels[i].chan_type) {
		case SENSOR_CHAN_AMBIENT_TEMP:
			edata->has_temp = 1;
			break;
		case SENSOR_CHAN_MAGN_X:
			edata->has_magn_x = 1;
			break;
		case SENSOR_CHAN_MAGN_Y:
			edata->has_magn_y = 1;
			break;
		case SENSOR_CHAN_MAGN_Z:
			edata->has_magn_z = 1;
			break;
		case SENSOR_CHAN_ALL:
			edata->has_temp = 1;
		case SENSOR_CHAN_MAGN_XYZ:
			edata->has_magn_x = 1;
			edata->has_magn_y = 1;
			edata->has_magn_z = 1;
			break;
		default:
			continue;
			break;
		}
	}

	rc = mmc56x3_sample_fetch_helper(dev, SENSOR_CHAN_ALL, &edata->data);
	if (rc != 0) {
		LOG_ERR("Failed to fetch samples");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

void mmc56x3_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct rtio_work_req *req = rtio_work_req_alloc();

	__ASSERT_NO_MSG(req);

	rtio_work_req_submit(req, iodev_sqe, mmc56x3_submit_sync);
}
