/*
 * Copyright (c) 2024 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include "bme280.h"

LOG_MODULE_DECLARE(BME280, CONFIG_SENSOR_LOG_LEVEL);

void bme280_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	uint32_t min_buf_len = sizeof(struct bme280_encoded_data);
	int rc;
	uint8_t *buf;
	uint32_t buf_len;

	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct sensor_chan_spec *const channels = cfg->channels;
	const size_t num_channels = cfg->count;

	/* Check if the requested channels are supported */
	for (size_t i = 0; i < num_channels; i++) {
		switch (channels[i].chan_type) {
		case SENSOR_CHAN_AMBIENT_TEMP:
		case SENSOR_CHAN_HUMIDITY:
		case SENSOR_CHAN_PRESS:
		case SENSOR_CHAN_ALL:
			break;
		default:
			LOG_ERR("Unsupported channel type %d", channels[i].chan_type);
			rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
			return;
		}
	}

	rc = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	if (rc != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buf_len);
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	struct bme280_encoded_data *edata;

	edata = (struct bme280_encoded_data *)buf;

	edata->header.timestamp = k_ticks_to_ns_floor64(k_uptime_ticks());

	rc = bme280_sample_fetch_helper(dev, SENSOR_CHAN_ALL, &edata->reading);
	if (rc != 0) {
		LOG_ERR("Failed to fetch samples");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	rtio_iodev_sqe_ok(iodev_sqe, 0);
}
