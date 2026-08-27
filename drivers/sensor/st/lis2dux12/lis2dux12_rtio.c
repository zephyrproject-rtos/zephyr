/* ST Microelectronics LIS2DUX12 6-axis IMU sensor driver
 *
 * Copyright (c) 2023 Google LLC
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include "lis2dux12.h"
#include "lis2dux12_rtio.h"
#include "lis2dux12_decoder.h"
#include <zephyr/rtio/work.h>
#include <zephyr/drivers/sensor_clock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LIS2DUX12_RTIO, CONFIG_SENSOR_LOG_LEVEL);

static void lis2dux12_submit_sample(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct sensor_chan_spec *const channels = cfg->channels;
	const size_t num_channels = cfg->count;
	uint32_t min_buf_len = sizeof(struct lis2dux12_rtio_data);
	uint64_t cycles;
	int rc = 0;
	uint8_t *buf;
	uint32_t buf_len;
	struct lis2dux12_rtio_data *edata;
	struct lis2dux12_data *data = dev->data;

	const struct lis2dux12_config *config = dev->config;
	const struct lis2dux12_chip_api *chip_api = config->chip_api;

	/* Get the buffer for the frame, it may be allocated dynamically by the rtio context */
	rc = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	if (rc != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buf_len);
		goto err;
	}

	edata = (struct lis2dux12_rtio_data *)buf;

	edata->has_accel = 0;
	edata->has_temp = 0;

	for (int i = 0; i < num_channels; i++) {
		switch (channels[i].chan_type) {
		case SENSOR_CHAN_ACCEL_X:
		case SENSOR_CHAN_ACCEL_Y:
		case SENSOR_CHAN_ACCEL_Z:
		case SENSOR_CHAN_ACCEL_XYZ:
			edata->has_accel = 1;

			rc = chip_api->rtio_read_accel(dev, edata->acc);
			if (rc  < 0) {
				LOG_DBG("Failed to read accel sample");
				goto err;
			}
			break;
#if defined(CONFIG_LIS2DUX12_ENABLE_TEMP)
		case SENSOR_CHAN_DIE_TEMP:
			edata->has_temp = 1;

			rc = chip_api->rtio_read_temp(dev, &edata->temp);
			if (rc < 0) {
				LOG_DBG("Failed to read temp sample");
				goto err;
			}
			break;
#endif
		case SENSOR_CHAN_ALL:
			edata->has_accel = 1;

			rc = chip_api->rtio_read_accel(dev, edata->acc);
			if (rc  < 0) {
				LOG_DBG("Failed to read accel sample");
				goto err;
			}

#if defined(CONFIG_LIS2DUX12_ENABLE_TEMP)
			edata->has_temp = 1;

			rc = chip_api->rtio_read_temp(dev, &edata->temp);
			if (rc < 0) {
				LOG_DBG("Failed to read temp sample");
				goto err;
			}
#endif
			break;
		default:
			continue;
		}
	}

	rc = sensor_clock_get_cycles(&cycles);
	if (rc != 0) {
		LOG_ERR("Failed to get sensor clock cycles");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		goto err;
	}

	edata->header.is_fifo = false;
	edata->header.range = data->range;
	edata->header.timestamp = sensor_clock_cycles_to_ns(cycles);

	rtio_iodev_sqe_ok(iodev_sqe, 0);

err:
	/* Check that the fetch succeeded */
	if (rc != 0) {
		LOG_ERR("Failed to fetch samples");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}
}

void lis2dux12_submit_sync(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct device *dev = cfg->sensor;

	if (!cfg->is_streaming) {
		lis2dux12_submit_sample(dev, iodev_sqe);
	} else if (IS_ENABLED(CONFIG_LIS2DUX12_STREAM)) {
		lis2dux12_submit_stream(dev, iodev_sqe);
	} else {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
	}
}

void lis2dux12_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct rtio_work_req *req = rtio_work_req_alloc();

	if (req == NULL) {
		LOG_ERR("RTIO work item allocation failed. Consider to increase "
			"CONFIG_RTIO_WORKQ_POOL_ITEMS.");
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	rtio_work_req_submit(req, iodev_sqe, lis2dux12_submit_sync);
}
