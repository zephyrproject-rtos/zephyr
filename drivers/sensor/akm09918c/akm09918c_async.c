/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include "akm09918c.h"

LOG_MODULE_DECLARE(AKM09918C, CONFIG_SENSOR_LOG_LEVEL);

int akm09918c_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	uint32_t min_buf_len = sizeof(struct akm09918c_encoded_data);
	int rc;
	uint8_t *buf;
	uint32_t buf_len;
	struct akm09918c_encoded_data *edata;

	/* Get the buffer for the frame, it may be allocated dynamically by the rtio context */
	rc = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	if (rc != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buf_len);
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return rc;
	}

	edata = (struct akm09918c_encoded_data *)buf;
	edata->header.timestamp = k_ticks_to_ns_floor64(k_uptime_ticks());

	rc = akm09918c_sample_fetch_helper(dev, SENSOR_CHAN_MAGN_XYZ, &edata->readings[0],
					   &edata->readings[1], &edata->readings[2]);
	if (rc != 0) {
		LOG_ERR("Failed to fetch samples");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return rc;
	}

	rtio_iodev_sqe_ok(iodev_sqe, 0);

	return 0;
}
