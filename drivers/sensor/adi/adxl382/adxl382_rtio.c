/*
 * Copyright (c) 2026 Deere & Company
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/rtio/work.h>

#include "adxl382.h"

LOG_MODULE_DECLARE(adxl382, CONFIG_SENSOR_LOG_LEVEL);

static void adxl382_submit_fetch(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct device *dev = cfg->sensor;
	const uint32_t min_buffer_len = sizeof(struct adxl382_encoded_data);
	struct adxl382_encoded_data *edata;
	uint8_t *buffer;
	uint32_t buffer_len;
	int rc;

	rc = rtio_sqe_rx_buf(iodev_sqe, min_buffer_len, min_buffer_len, &buffer, &buffer_len);
	if (rc != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buffer_len);
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	edata = (struct adxl382_encoded_data *)buffer;
	edata->timestamp = k_ticks_to_ns_floor64(k_uptime_ticks());
	rc = adxl382_get_accel_data(dev, &edata->sample);
	if (rc != 0) {
		LOG_ERR("Failed to fetch samples");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

void adxl382_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	struct rtio_work_req *req;

	ARG_UNUSED(dev);

	if (cfg->is_streaming) {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
		return;
	}

	req = rtio_work_req_alloc();
	if (req == NULL) {
		LOG_ERR("RTIO work item pool exhausted");
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	rtio_work_req_submit(req, iodev_sqe, adxl382_submit_fetch);
}
