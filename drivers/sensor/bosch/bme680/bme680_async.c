/*
 * Copyright (c) 2025 Nordic Semiconductors ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/rtio/work.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor_clock.h>

#include "bme680.h"

LOG_MODULE_DECLARE(bme680, CONFIG_SENSOR_LOG_LEVEL);

void bme680_submit_sync(struct rtio_iodev_sqe *iodev_sqe)
{
	uint32_t min_buf_len = sizeof(struct bme680_encoded_data);
	int rc;
	uint64_t cycles;
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

	rc = sensor_clock_get_cycles(&cycles);
	if (rc != 0) {
		LOG_ERR("Failed to get sensor clock cycles");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	struct bme680_encoded_data *edata;

	edata = (struct bme680_encoded_data *)buf;
	edata->header.timestamp = sensor_clock_cycles_to_ns(cycles);
	edata->header.has_temp = 0;
	edata->header.has_humidity = 0;
	edata->header.has_press = 0;
	edata->header.has_gas_res = 0;

	/* Check if the requested channels are supported */
	for (size_t i = 0; i < num_channels; i++) {
		switch (channels[i].chan_type) {
		case SENSOR_CHAN_AMBIENT_TEMP:
			edata->header.has_temp = 1;
			break;
		case SENSOR_CHAN_HUMIDITY:
			edata->header.has_humidity = 1;
			break;
		case SENSOR_CHAN_PRESS:
			edata->header.has_press = 1;
			break;
		case SENSOR_CHAN_GAS_RES:
			edata->header.has_gas_res = 1;
			break;
		case SENSOR_CHAN_ALL:
			edata->header.has_temp = 1;
			edata->header.has_humidity = 1;
			edata->header.has_press = 1;
			edata->header.has_gas_res = 1;
			break;
		default:
			continue;
			break;
		}
	}

	rc = bme680_sample_fetch(dev, SENSOR_CHAN_ALL);
	if (rc != 0) {
		LOG_ERR("Failed to fetch samples");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

void bme680_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg =
			(const struct sensor_read_config *) iodev_sqe->sqe.iodev->data;

	if (!cfg->is_streaming) {
		struct rtio_work_req *req = rtio_work_req_alloc();

		if (req == NULL) {
			LOG_ERR("RTIO work item allocation failed. Consider to increase "
				"CONFIG_RTIO_WORKQ_POOL_ITEMS.");
			rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
			return;
		}
		rtio_work_req_submit(req, iodev_sqe, bme680_submit_sync);
	} else {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
	}
}
