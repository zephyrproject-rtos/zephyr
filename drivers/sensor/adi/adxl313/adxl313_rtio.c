/*
 * Copyright (c) 2025 Lothar Rubusch <l.rubusch@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/rtio/work.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>

#include "adxl313.h"

LOG_MODULE_DECLARE(ADXL313, CONFIG_SENSOR_LOG_LEVEL);

/*
 * To avoid using `#if defined` preprocessor conditionals which would exclude
 * disabled sources directly from the build, the Zephyr project prefers using
 * IS_ENABLED(). Because IS_ENABLED() is evaluated later during compilation, a
 * forward declaration is required for the code to compile. When the option is
 * disabled, IS_ENABLED() prevents execution by ensuring the function
 * implementation is not linked.
 */
void adxl313_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);

static void adxl313_submit_fetch(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg =
		(const struct sensor_read_config *)iodev_sqe->sqe.iodev->data;
	const struct device *dev = cfg->sensor;
	int rc;
	uint32_t min_buffer_len = sizeof(struct adxl313_dev_data);
	uint8_t *buffer;
	uint32_t buffer_len;

	rc = rtio_sqe_rx_buf(iodev_sqe, min_buffer_len, min_buffer_len, &buffer, &buffer_len);
	if (rc != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buffer_len);
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	struct adxl313_xyz_accel_data *data = (struct adxl313_xyz_accel_data *)buffer;

	rc = adxl313_read_sample(dev, data);
	if (rc != 0) {
		LOG_ERR("Failed to fetch samples");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

void adxl313_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;

	if (!cfg->is_streaming) {
		struct rtio_work_req *req = rtio_work_req_alloc();

		__ASSERT_NO_MSG(req);

		rtio_work_req_submit(req, iodev_sqe, adxl313_submit_fetch);
	} else if (IS_ENABLED(CONFIG_ADXL313_STREAM)) {
		adxl313_submit_stream(dev, iodev_sqe);
	} else {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
	}
}
