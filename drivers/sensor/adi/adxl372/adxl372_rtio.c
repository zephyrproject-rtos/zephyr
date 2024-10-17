/*
 * Copyright (c) 2024 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/rtio/work.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>

#include "adxl372.h"

LOG_MODULE_DECLARE(ADXL372, CONFIG_SENSOR_LOG_LEVEL);

static void adxl372_submit_fetch(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg =
			(const struct sensor_read_config *)iodev_sqe->sqe.iodev->data;
	const struct device *dev = cfg->sensor;
	int rc;
	uint32_t min_buffer_len = sizeof(struct adxl372_xyz_accel_data);
	uint8_t *buffer;
	uint32_t buffer_len;

	const struct adxl372_dev_config *cfg_372 = (const struct adxl372_dev_config *)dev->config;

	rc = rtio_sqe_rx_buf(iodev_sqe, min_buffer_len, min_buffer_len, &buffer, &buffer_len);
	if (rc != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buffer_len);
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	struct adxl372_xyz_accel_data *data = (struct adxl372_xyz_accel_data *)buffer;

	rc = adxl372_get_accel_data(dev, cfg_372->max_peak_detect_mode, data);
	if (rc != 0) {
		LOG_ERR("Failed to fetch samples");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

void adxl372_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg =
		(const struct sensor_read_config *)iodev_sqe->sqe.iodev->data;

	if (!cfg->is_streaming) {
		struct rtio_work_req *req = rtio_work_req_alloc();

		__ASSERT_NO_MSG(req);

		rtio_work_req_submit(req, iodev_sqe, adxl372_submit_fetch);
	} else if (IS_ENABLED(CONFIG_ADXL372_STREAM)) {
		adxl372_submit_stream(dev, iodev_sqe);
	} else {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
	}
}
