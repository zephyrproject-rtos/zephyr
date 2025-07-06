/*
 * Copyright (c) 2024 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/rtio/work.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>

#include "adxl367.h"

LOG_MODULE_DECLARE(ADXL367, CONFIG_SENSOR_LOG_LEVEL);

static void adxl367_submit_fetch(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg =
			(const struct sensor_read_config *) iodev_sqe->sqe.iodev->data;
	const struct device *dev = cfg->sensor;
	struct adxl367_data *data = dev->data;
	int rc;
	uint32_t min_buffer_len = sizeof(struct adxl367_sample_data);
	uint8_t *buffer;
	uint32_t buffer_len;

	rc = rtio_sqe_rx_buf(iodev_sqe, min_buffer_len, min_buffer_len, &buffer, &buffer_len);
	if (rc != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buffer_len);
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	struct adxl367_sample_data *enc_data = (struct adxl367_sample_data *)buffer;

#ifdef CONFIG_ADXL367_STREAM
	enc_data->is_fifo = 0;
#endif /*CONFIG_ADXL367_STREAM*/

	rc = adxl367_get_accel_data(dev, &enc_data->xyz);
	if (rc != 0) {
		LOG_ERR("Failed to fetch xyz samples");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	enc_data->xyz.range = data->range;

	rc = adxl367_get_temp_data(dev, &enc_data->raw_temp);
	if (rc != 0) {
		LOG_ERR("Failed to fetch temp samples");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

void adxl367_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg =
			(const struct sensor_read_config *) iodev_sqe->sqe.iodev->data;

	if (!cfg->is_streaming) {
		struct rtio_work_req *req = rtio_work_req_alloc();

		__ASSERT_NO_MSG(req);

		rtio_work_req_submit(req, iodev_sqe, adxl367_submit_fetch);
	} else if (IS_ENABLED(CONFIG_ADXL367_STREAM)) {
		adxl367_submit_stream(dev, iodev_sqe);
	} else {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
	}
}
