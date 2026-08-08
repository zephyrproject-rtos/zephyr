/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include "vl53l5cx.h"
#include "vl53l5cx_rtio.h"
#include "vl53l5cx_decoder.h"
#include <zephyr/rtio/work.h>
#include <zephyr/drivers/sensor_clock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(vl53l5cx_rtio, CONFIG_SENSOR_LOG_LEVEL);

#define DATA_READY_TIMEOUT 1000

static void vl53l5cx_submit_sample(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	uint32_t min_buf_len = sizeof(struct vl53l5cx_rtio_data);
	uint64_t cycles;
	int rc = 0;
	uint8_t *buf;
	uint32_t buf_len;
	struct vl53l5cx_rtio_data *edata;
	uint32_t start_tick;
	uint8_t ready = 0;
	uint8_t resolution;
	uint8_t hal_ret;

	struct vl53l5cx_data *data = dev->data;

	rc = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	if (rc != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buf_len);
		goto err;
	}

	edata = (struct vl53l5cx_rtio_data *)buf;

	hal_ret = vl53l5cx_get_resolution(&data->vl53l5cx, &resolution);
	if (hal_ret != VL53L5CX_STATUS_OK) {
		rtio_iodev_sqe_err(iodev_sqe, -EIO);
		return;
	}

	hal_ret = vl53l5cx_start_ranging(&data->vl53l5cx);
	if (hal_ret != VL53L5CX_STATUS_OK) {
		rtio_iodev_sqe_err(iodev_sqe, -EIO);
		return;
	}

	start_tick = k_uptime_get_32();

	do {
		hal_ret = vl53l5cx_check_data_ready(&data->vl53l5cx, &ready);
		if (hal_ret != VL53L5CX_STATUS_OK) {
			rtio_iodev_sqe_err(iodev_sqe, -EIO);
			return;
		}
	} while (ready != 1 && (k_uptime_get_32() - start_tick) < DATA_READY_TIMEOUT);

	if (ready != 1) {
		rtio_iodev_sqe_err(iodev_sqe, -ETIMEDOUT);
		return;
	}

	hal_ret = vl53l5cx_get_ranging_data(&data->vl53l5cx, &edata->data);
	if (hal_ret != VL53L5CX_STATUS_OK) {
		rtio_iodev_sqe_err(iodev_sqe, -EIO);
		return;
	}

	hal_ret = vl53l5cx_stop_ranging(&data->vl53l5cx);
	if (hal_ret != VL53L5CX_STATUS_OK) {
		rtio_iodev_sqe_err(iodev_sqe, -EIO);
		return;
	}

	edata->distance_idx_num = resolution == VL53L5CX_RESOLUTION_4X4 ? 16 : 64;

	rc = sensor_clock_get_cycles(&cycles);
	if (rc != 0) {
		LOG_ERR("Failed to get sensor clock cycles");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		goto err;
	}

	edata->timestamp = sensor_clock_cycles_to_ns(cycles);

	rtio_iodev_sqe_ok(iodev_sqe, 0);

err:
	/* Check that the fetch succeeded */
	if (rc != 0) {
		LOG_ERR("Failed to fetch samples");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}
}

void vl53l5cx_submit_sync(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct device *dev = cfg->sensor;

	if (!cfg->is_streaming) {
		vl53l5cx_submit_sample(dev, iodev_sqe);
	} else {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
	}
}

void vl53l5cx_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct rtio_work_req *req = rtio_work_req_alloc();

	if (req == NULL) {
		LOG_ERR("RTIO work item allocation failed. Consider to increase "
			"CONFIG_RTIO_WORKQ_POOL_ITEMS.");
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	rtio_work_req_submit(req, iodev_sqe, vl53l5cx_submit_sync);
}
