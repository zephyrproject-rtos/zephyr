/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/rtio/work.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_clock.h>

#include <zephyr/drivers/sensor/admt4000.h>

LOG_MODULE_DECLARE(admt4000, CONFIG_SENSOR_LOG_LEVEL);

static void admt4000_submit_sync(struct rtio_iodev_sqe *iodev_sqe)
{
	uint32_t min_buf_len = sizeof(struct admt4000_encoded_data);
	int rc;
	uint8_t *buf;
	uint32_t buf_len;
	uint64_t cycles;

	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct device *dev = cfg->sensor;
	const struct sensor_chan_spec *const channels = cfg->channels;
	const size_t num_channels = cfg->count;
	struct admt4000_data *dev_data = dev->data;

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

	struct admt4000_encoded_data *edata = (struct admt4000_encoded_data *)buf;

	edata->header.timestamp = sensor_clock_cycles_to_ns(cycles);
	edata->has_angle = 0;
	edata->has_turns = 0;
	edata->has_temp = 0;
	edata->has_cos = 0;
	edata->has_sin = 0;
	edata->has_radius = 0;

	/* Mark which channels were requested */
	for (size_t i = 0; i < num_channels; i++) {
		switch (channels[i].chan_type) {
		case SENSOR_CHAN_ADMT4000_ANGLE:
			edata->has_angle = 1;
			break;
		case SENSOR_CHAN_ROTATION:
			edata->has_turns = 1;
			break;
		case SENSOR_CHAN_AMBIENT_TEMP:
			edata->has_temp = 1;
			break;
		case SENSOR_CHAN_ADMT4000_COS:
			edata->has_cos = 1;
			break;
		case SENSOR_CHAN_ADMT4000_SIN:
			edata->has_sin = 1;
			break;
		case SENSOR_CHAN_ADMT4000_RADIUS:
			edata->has_radius = 1;
			break;
		case SENSOR_CHAN_ALL:
			edata->has_angle = 1;
			edata->has_turns = 1;
			edata->has_temp = 1;
			edata->has_cos = 1;
			edata->has_sin = 1;
			edata->has_radius = 1;
			break;
		default:
			continue;
		}
	}

	/* Fetch into the device's data buffer (sub-getters populate dev->data) */
	rc = admt4000_sample_fetch_helper(dev, SENSOR_CHAN_ALL, dev_data);
	if (rc != 0) {
		LOG_ERR("Failed to fetch samples");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	/* Copy only the sample values into the compact encoded buffer */
	edata->data.quarter_turns = dev_data->quarter_turns;
	edata->data.angle = dev_data->angle[1];
	edata->data.cos_val = dev_data->cos_val;
	edata->data.sin_val = dev_data->sin_val;
	edata->data.converted_temp = dev_data->converted_temp;
	edata->data.converted_radius = dev_data->converted_radius;

	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

void admt4000_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct rtio_work_req *req = rtio_work_req_alloc();

	if (req == NULL) {
		LOG_ERR("RTIO work item allocation failed. Consider increasing "
			"CONFIG_RTIO_WORKQ_POOL_ITEMS.");
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	rtio_work_req_submit(req, iodev_sqe, admt4000_submit_sync);
}
