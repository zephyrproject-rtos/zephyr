/*
 * Copyright (c) 2025 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/rtio/rtio.h>
#include <zephyr/rtio/work.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/logging/log.h>

#include "bme680.h"

LOG_MODULE_DECLARE(bme680, CONFIG_SENSOR_LOG_LEVEL);

/* Process sensor reading for RTIO and encode data */
static void bme680_process_reading(const struct device *dev,
				   struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct sensor_chan_spec *const channels = cfg->channels;
	const size_t num_channels = cfg->count;
	const uint32_t min_buf_len = sizeof(struct bme680_encoded_data);
	struct bme680_encoded_data *edata;
	struct bme680_reading reading;
	uint8_t *buf;
	uint64_t cycles;
	uint32_t buf_len;
	bool fetch_temp = false;
	bool fetch_press = false;
	bool fetch_humidity = false;
	bool fetch_gas = false;
	int rc;

	/* Get the buffer for the frame */
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

	edata = (struct bme680_encoded_data *)buf;
	edata->header.timestamp = sensor_clock_cycles_to_ns(cycles);
	edata->has_temp = 0;
	edata->has_press = 0;
	edata->has_humidity = 0;
	edata->has_gas = 0;

	/* Determine which channels to fetch */
	for (size_t i = 0; i < num_channels; i++) {
		switch (channels[i].chan_type) {
		case SENSOR_CHAN_ALL:
			fetch_temp = true;
			fetch_press = true;
			fetch_humidity = true;
			fetch_gas = true;
			break;
		case SENSOR_CHAN_AMBIENT_TEMP:
			fetch_temp = true;
			break;
		case SENSOR_CHAN_PRESS:
			fetch_press = true;
			break;
		case SENSOR_CHAN_HUMIDITY:
			fetch_humidity = true;
			break;
		case SENSOR_CHAN_GAS_RES:
			fetch_gas = true;
			break;
		default:
			break;
		}
	}

	rc = bme680_sample_fetch_helper(dev, SENSOR_CHAN_ALL, &reading);
	if (rc < 0) {
		LOG_ERR("Failed to fetch sample: %d", rc);
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	if (fetch_temp) {
		edata->has_temp = 1;
		edata->reading.comp_temp = reading.comp_temp;
	}

	if (fetch_press) {
		edata->has_press = 1;
		edata->reading.comp_press = reading.comp_press;
	}

	if (fetch_humidity) {
		edata->has_humidity = 1;
		edata->reading.comp_humidity = reading.comp_humidity;
	}

	if (fetch_gas) {
		edata->has_gas = 1;
		edata->reading.comp_gas = reading.comp_gas;
	}

	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

/* Synchronous RTIO submission handler */
static void bme680_submit_sync(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct device *dev = cfg->sensor;

	if (!cfg->is_streaming) {
		bme680_process_reading(dev, iodev_sqe);
	} else {
		/* BME680 does not support streaming mode */
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
	}
}

/* Submit sensor read request to RTIO work queue */
void bme680_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	ARG_UNUSED(dev);

	struct rtio_work_req *req = rtio_work_req_alloc();

	if (req == NULL) {
		LOG_ERR("RTIO work item allocation failed");
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	rtio_work_req_submit(req, iodev_sqe, bme680_submit_sync);
}
