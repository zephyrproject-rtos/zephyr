/*
 * Copyright (c) 2024 Florian Weber <Florian.Weber@live.de>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT melexis_mlx90394

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor_clock.h>

#include "mlx90394.h"
#include "mlx90394_reg.h"

#include <stddef.h>

LOG_MODULE_DECLARE(MLX90394, CONFIG_SENSOR_LOG_LEVEL);

void mlx90394_async_fetch(struct k_work *work)
{
	int rc;
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct mlx90394_data *data = CONTAINER_OF(dwork, struct mlx90394_data, async_fetch_work);
	const struct device *dev = data->dev;
	const struct sensor_read_config *cfg =
		data->work_ctx.iodev_sqe->sqe.iodev->data;
	struct mlx90394_encoded_data *edata;
	uint32_t buf_len = sizeof(struct mlx90394_encoded_data);
	uint8_t *buf;

	rc = mlx90394_sample_fetch_internal(dev, cfg->channels->chan_type);
	if (rc != 0) {
		LOG_ERR("Failed to fetch samples");
		rtio_iodev_sqe_err(data->work_ctx.iodev_sqe, rc);
		return;
	}
	/* Get the buffer for the frame, it may be allocated dynamically by the rtio context */
	rc = rtio_sqe_rx_buf(data->work_ctx.iodev_sqe, buf_len, buf_len, &buf, &buf_len);
	if (rc != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", buf_len);
		rtio_iodev_sqe_err(data->work_ctx.iodev_sqe, rc);
		return;
	}

	edata = (struct mlx90394_encoded_data *)buf;

	/* buffered from submit */
	edata->header.timestamp = data->work_ctx.timestamp;
	edata->header.config_val = data->work_ctx.config_val;

	switch (cfg->channels->chan_type) {
	case SENSOR_CHAN_MAGN_X: {
		edata->readings[0] =
			(int16_t)((uint16_t)data->sample.x_l | (uint16_t)(data->sample.x_h << 8));
	} break;
	case SENSOR_CHAN_MAGN_Y: {
		edata->readings[1] =
			(int16_t)((uint16_t)data->sample.y_l | (uint16_t)(data->sample.y_h << 8));
	} break;
	case SENSOR_CHAN_MAGN_Z: {
		edata->readings[2] =
			(int16_t)((uint16_t)data->sample.z_l | (uint16_t)(data->sample.z_h << 8));
	} break;
	case SENSOR_CHAN_AMBIENT_TEMP: {
		edata->readings[3] = (int16_t)((uint16_t)data->sample.temp_l |
					       (uint16_t)(data->sample.temp_h << 8));
	} break;
	case SENSOR_CHAN_MAGN_XYZ: {
		edata->readings[0] =
			(int16_t)((uint16_t)data->sample.x_l | (uint16_t)(data->sample.x_h << 8));
		edata->readings[1] =
			(int16_t)((uint16_t)data->sample.y_l | (uint16_t)(data->sample.y_h << 8));
		edata->readings[2] =
			(int16_t)((uint16_t)data->sample.z_l | (uint16_t)(data->sample.z_h << 8));
	} break;
	case SENSOR_CHAN_ALL: {
		edata->readings[0] =
			(int16_t)((uint16_t)data->sample.x_l | (uint16_t)(data->sample.x_h << 8));
		edata->readings[1] =
			(int16_t)((uint16_t)data->sample.y_l | (uint16_t)(data->sample.y_h << 8));
		edata->readings[2] =
			(int16_t)((uint16_t)data->sample.z_l | (uint16_t)(data->sample.z_h << 8));
		edata->readings[3] = (int16_t)((uint16_t)data->sample.temp_l |
					       (uint16_t)(data->sample.temp_h << 8));
	} break;
	default: {
		LOG_DBG("Invalid channel %d", cfg->channels->chan_type);
		rtio_iodev_sqe_err(data->work_ctx.iodev_sqe, -ENOTSUP);
		return;
	}
	}
	rtio_iodev_sqe_ok(data->work_ctx.iodev_sqe, 0);
}

void mlx90394_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	int rc;
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	struct mlx90394_data *data = dev->data;
	uint64_t cycles;

	rc = mlx90394_trigger_measurement_internal(dev, cfg->channels->chan_type);
	if (rc != 0) {
		LOG_ERR("Failed to trigger measurement");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	rc = sensor_clock_get_cycles(&cycles);
	if (rc != 0) {
		LOG_ERR("Failed to get sensor clock cycles");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	/* save information for the work item */
	data->work_ctx.timestamp = sensor_clock_cycles_to_ns(cycles);
	data->work_ctx.iodev_sqe = iodev_sqe;
	data->work_ctx.config_val = data->config_val;

	/* schedule work to read out sensor and inform the executor about completion with success */
	k_work_schedule(&data->async_fetch_work, K_USEC(data->measurement_time_us));
}
