/*
 * Copyright (c) 2023 Google LLC
 * Copyright (c) 2024 Croxel Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/work.h>
#include "icm42688.h"
#include "icm42688_decoder.h"
#include "icm42688_reg.h"
#include "icm42688_rtio.h"
#include "icm42688_spi.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ICM42688_RTIO, CONFIG_SENSOR_LOG_LEVEL);

static int icm42688_rtio_sample_fetch(const struct device *dev, int16_t readings[7])
{
	uint8_t status;
	const struct icm42688_dev_cfg *cfg = dev->config;
	uint8_t *buffer = (uint8_t *)readings;

	int res = icm42688_spi_read(&cfg->spi, REG_INT_STATUS, &status, 1);

	if (res) {
		return res;
	}

	if (!FIELD_GET(BIT_INT_STATUS_DATA_RDY, status)) {
		return -EBUSY;
	}

	res = icm42688_read_all(dev, buffer);

	if (res) {
		return res;
	}

	for (int i = 0; i < 7; i++) {
		readings[i] = sys_le16_to_cpu((buffer[i * 2] << 8) | buffer[i * 2 + 1]);
	}

	return 0;
}

static void icm42688_submit_one_shot(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct sensor_chan_spec *const channels = cfg->channels;
	const size_t num_channels = cfg->count;
	uint32_t min_buf_len = sizeof(struct icm42688_encoded_data);
	int rc;
	uint8_t *buf;
	uint32_t buf_len;
	struct icm42688_encoded_data *edata;

	/* Get the buffer for the frame, it may be allocated dynamically by the rtio context */
	rc = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	if (rc != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buf_len);
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	edata = (struct icm42688_encoded_data *)buf;

	rc = icm42688_encode(dev, channels, num_channels, buf);
	if (rc != 0) {
		LOG_ERR("Failed to encode sensor data");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	rc = icm42688_rtio_sample_fetch(dev, edata->readings);
	/* Check that the fetch succeeded */
	if (rc != 0) {
		LOG_ERR("Failed to fetch samples");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

void icm42688_submit_sync(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct device *dev = cfg->sensor;

	if (!cfg->is_streaming) {
		icm42688_submit_one_shot(dev, iodev_sqe);
	} else if (IS_ENABLED(CONFIG_ICM42688_STREAM)) {
		icm42688_submit_stream(dev, iodev_sqe);
	} else {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
	}
}

void icm42688_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct rtio_work_req *req = rtio_work_req_alloc();

	if (req == NULL) {
		LOG_ERR("RTIO work item allocation failed. Consider to increase "
			"CONFIG_RTIO_WORKQ_POOL_ITEMS.");
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	rtio_work_req_submit(req, iodev_sqe, icm42688_submit_sync);
}

BUILD_ASSERT(sizeof(struct icm42688_decoder_header) == 9);
