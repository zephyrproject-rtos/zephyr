/*
 * Copyright (c) 2024 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/rtio/work.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>

#include "adxl345.h"

LOG_MODULE_DECLARE(ADXL345, CONFIG_SENSOR_LOG_LEVEL);

static void adxl345_submit_fetch(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg =
			(const struct sensor_read_config *) iodev_sqe->sqe.iodev->data;
	const struct device *dev = cfg->sensor;
	uint32_t min_buffer_len = sizeof(struct adxl345_dev_data);
	uint8_t *buffer;
	uint32_t buffer_len;
	struct adxl345_xyz_accel_data *data;
	int rc;

	rc = rtio_sqe_rx_buf(iodev_sqe, min_buffer_len, min_buffer_len,
			     &buffer, &buffer_len);
	if (rc) {
		LOG_ERR("Failed to get a read buffer of size %u bytes",
			min_buffer_len);
		goto err;
	}

	data = (struct adxl345_xyz_accel_data *)buffer;

	rc = adxl345_get_accel_data(dev, data);
	if (rc) {
		LOG_ERR("Failed to fetch samples");
		goto err;
	}

	rtio_iodev_sqe_ok(iodev_sqe, 0);

	return;
err:
	rtio_iodev_sqe_err(iodev_sqe, rc);
}

void adxl345_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg =
			(const struct sensor_read_config *) iodev_sqe->sqe.iodev->data;

	if (!cfg->is_streaming) {
		struct rtio_work_req *req = rtio_work_req_alloc();

		__ASSERT_NO_MSG(req);

		rtio_work_req_submit(req, iodev_sqe, adxl345_submit_fetch);
	} else if (IS_ENABLED(CONFIG_ADXL345_STREAM)) {
		adxl345_submit_stream(dev, iodev_sqe);
	} else {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
	}
}
