/*
 * Copyright (c) 2025, CATIE
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/rtio/work.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor_clock.h>

#include "max30101.h"

LOG_MODULE_REGISTER(MAX30101_ASYNC, CONFIG_SENSOR_LOG_LEVEL);

uint8_t max30101_encode_channels(const struct max30101_data *data, struct max30101_encoded_data *edata, const struct sensor_chan_spec *channels, size_t num_channels)
{
	/* Check if the requested channels are supported */
	for (size_t i = 0; i < num_channels; i++) {
		switch (channels[i].chan_type) {
		case SENSOR_CHAN_RED:
			edata->has_red = data->num_channels[MAX30101_LED_CHANNEL_RED];
			break;
		case SENSOR_CHAN_IR:
			edata->has_ir = data->num_channels[MAX30101_LED_CHANNEL_IR];
			break;
		case SENSOR_CHAN_GREEN:
			edata->has_green = data->num_channels[MAX30101_LED_CHANNEL_GREEN];
			break;
#if CONFIG_MAX30101_DIE_TEMPERATURE
		case SENSOR_CHAN_DIE_TEMP:
			edata->has_temp = 1;
			break;
#endif /* CONFIG_MAX30101_DIE_TEMPERATURE */
		case SENSOR_CHAN_ALL:
			edata->has_red = data->num_channels[MAX30101_LED_CHANNEL_RED];
			edata->has_ir = data->num_channels[MAX30101_LED_CHANNEL_IR];
			edata->has_green = data->num_channels[MAX30101_LED_CHANNEL_GREEN];
#if CONFIG_MAX30101_DIE_TEMPERATURE
			edata->has_temp = 1;
#endif /* CONFIG_MAX30101_DIE_TEMPERATURE */
			break;
		default:
			continue;
			break;
		}
	}

	return edata->has_red + edata->has_ir + edata->has_green;
}

void max30101_submit_sync(struct rtio_iodev_sqe *iodev_sqe)
{
	uint32_t min_buf_len = sizeof(struct max30101_encoded_data);
	int rc;
	uint64_t cycles;
	uint8_t *buf;
	uint32_t buf_len;

	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct device *dev = cfg->sensor;
	const struct max30101_data *data = dev->data;
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

	struct max30101_encoded_data *edata;

	edata = (struct max30101_encoded_data *)buf;
	edata->header.timestamp = sensor_clock_cycles_to_ns(cycles);
	edata->has_red = 0;
	edata->has_ir = 0;
	edata->has_green = 0;
	edata->has_temp = 0;

	/* Check if the requested channels are supported */
	max30101_encode_channels(data, edata, channels, num_channels);

	edata->sensor = dev;
	rc = max30101_read_sample(dev, &edata->reading);
	if (rc != 0) {
		LOG_ERR("Failed to fetch samples");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

void max30101_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg =
			(const struct sensor_read_config *) iodev_sqe->sqe.iodev->data;

	if (cfg->is_streaming) {
#if CONFIG_MAX30101_STREAM
		max30101_submit_stream(dev, iodev_sqe);
		return;
#else
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
		return;
#endif
	}

	struct rtio_work_req *req = rtio_work_req_alloc();

	if (req == NULL) {
		LOG_ERR("RTIO work item allocation failed. Consider to increase "
			"CONFIG_RTIO_WORKQ_POOL_ITEMS.");
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	rtio_work_req_submit(req, iodev_sqe, max30101_submit_sync);
}
