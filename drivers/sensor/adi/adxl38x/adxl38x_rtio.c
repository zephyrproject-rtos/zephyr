/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adxl38x.h"
#include <zephyr/logging/log.h>
#include <zephyr/rtio/work.h>
#include <zephyr/drivers/sensor.h>

LOG_MODULE_DECLARE(ADXL38X);

/**
 * @brief Submit a fetch read request
 *
 * @param iodev_sqe IODEV SQE pointer
 */
static void adxl38x_submit_fetch(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *config =
		(const struct sensor_read_config *)iodev_sqe->sqe.iodev->data;
	const struct device *dev = config->sensor;
	uint32_t min_buffer_len = sizeof(struct adxl38x_sample);
	uint8_t *buffer;
	uint32_t buffer_len;
	int rc;

	rc = rtio_sqe_rx_buf(iodev_sqe, min_buffer_len, min_buffer_len, &buffer, &buffer_len);
	if (rc != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buffer_len);
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	struct adxl38x_sample *sample = (struct adxl38x_sample *)buffer;

	sample->is_fifo = 0;

	rc = adxl38x_read_accel_data(dev, sample);
	if (rc != 0) {
		LOG_ERR("Failed to fetch samples");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	rc = adxl38x_read_temp_raw(dev, &sample->temp);
	if (rc != 0) {
		LOG_ERR("Failed to fetch temperature");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

/**
 * @brief Submit ADXL38X read request
 *
 * @param dev Device pointer
 * @param iodev_sqe IODEV SQE pointer
 */
void adxl38x_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg =
		(const struct sensor_read_config *)iodev_sqe->sqe.iodev->data;

	if (!cfg->is_streaming) {
		struct rtio_work_req *req = rtio_work_req_alloc();

		__ASSERT_NO_MSG(req);

		rtio_work_req_submit(req, iodev_sqe, adxl38x_submit_fetch);
	} else if (IS_ENABLED(CONFIG_ADXL38X_STREAM)) {
		adxl38x_submit_stream(dev, iodev_sqe);
	} else {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
	}
}
