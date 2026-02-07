/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adxl355.h"
#include <zephyr/logging/log.h>
#include <zephyr/rtio/work.h>
#include <zephyr/drivers/sensor.h>

LOG_MODULE_DECLARE(ADXL355);

/**
 * @brief Submit a fetch read request
 *
 * @param iodev_sqe IODEV SQE pointer
 */
static void adxl355_submit_fetch(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *config =
		(const struct sensor_read_config *)iodev_sqe->sqe.iodev->data;
	int rc;
	uint8_t *buffer;
	uint32_t buffer_len;
	uint32_t min_buffer_len = sizeof(struct adxl355_fifo_data) + (config->count * 9);

	rc = rtio_sqe_rx_buf(iodev_sqe, min_buffer_len, min_buffer_len, &buffer, &buffer_len);
	if (rc != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buffer_len);
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}
	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

/**
 * @brief Submit ADXL355 read request
 *
 * @param dev Device pointer
 * @param iodev_sqe IODEV SQE pointer
 */
void adxl355_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg =
		(const struct sensor_read_config *)iodev_sqe->sqe.iodev->data;

	if (!cfg->is_streaming) {
		struct rtio_work_req *req = rtio_work_req_alloc();

		__ASSERT_NO_MSG(req);

		rtio_work_req_submit(req, iodev_sqe, adxl355_submit_fetch);
	} else if (IS_ENABLED(CONFIG_ADXL355_STREAM)) {
		adxl355_submit_stream(dev, iodev_sqe);
	} else {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
	}
}
