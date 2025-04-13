/* ST Microelectronics LSM6DSV16X 6-axis IMU sensor driver
 *
 * Copyright (c) 2023 Google LLC
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include "lsm6dsv16x.h"
#include "lsm6dsv16x_rtio.h"
#include "lsm6dsv16x_decoder.h"
#include <zephyr/rtio/work.h>
#include <zephyr/drivers/sensor_clock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LSM6DSV16X_RTIO, CONFIG_SENSOR_LOG_LEVEL);

static void lsm6dsv16x_submit_sample(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct sensor_chan_spec *const channels = cfg->channels;
	const size_t num_channels = cfg->count;
	uint32_t min_buf_len = sizeof(struct lsm6dsv16x_rtio_data);
	uint64_t cycles;
	int rc = 0;
	uint8_t *buf;
	uint32_t buf_len;
	struct lsm6dsv16x_rtio_data *edata;
	struct lsm6dsv16x_data *data = dev->data;

	const struct lsm6dsv16x_config *config = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&config->ctx;

	/* Get the buffer for the frame, it may be allocated dynamically by the rtio context */
	rc = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	if (rc != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buf_len);
		goto err;
	}

	edata = (struct lsm6dsv16x_rtio_data *)buf;

	edata->has_accel = 0;
	edata->has_gyro = 0;
	edata->has_temp = 0;

	for (int i = 0; i < num_channels; i++) {
		switch (channels[i].chan_type) {
		case SENSOR_CHAN_ACCEL_X:
		case SENSOR_CHAN_ACCEL_Y:
		case SENSOR_CHAN_ACCEL_Z:
		case SENSOR_CHAN_ACCEL_XYZ:
			edata->has_accel = 1;

			rc = lsm6dsv16x_acceleration_raw_get(ctx, edata->acc);
			if (rc  < 0) {
				LOG_DBG("Failed to read accel sample");
				goto err;
			}
			break;
		case SENSOR_CHAN_GYRO_X:
		case SENSOR_CHAN_GYRO_Y:
		case SENSOR_CHAN_GYRO_Z:
		case SENSOR_CHAN_GYRO_XYZ:
			edata->has_gyro = 1;

			rc = lsm6dsv16x_angular_rate_raw_get(ctx, edata->gyro);
			if (rc  < 0) {
				LOG_DBG("Failed to read gyro sample");
				goto err;
			}
			break;
#if defined(CONFIG_LSM6DSV16X_ENABLE_TEMP)
		case SENSOR_CHAN_DIE_TEMP:
			edata->has_temp = 1;

			rc = lsm6dsv16x_temperature_raw_get(ctx, &edata->temp);
			if (rc < 0) {
				LOG_DBG("Failed to read temp sample");
				goto err;
			}
			break;
#endif
		case SENSOR_CHAN_ALL:
			edata->has_accel = 1;

			rc = lsm6dsv16x_acceleration_raw_get(ctx, edata->acc);
			if (rc  < 0) {
				LOG_DBG("Failed to read accel sample");
				goto err;
			}

			edata->has_gyro = 1;

			rc = lsm6dsv16x_angular_rate_raw_get(ctx, edata->gyro);
			if (rc  < 0) {
				LOG_DBG("Failed to read gyro sample");
				goto err;
			}

#if defined(CONFIG_LSM6DSV16X_ENABLE_TEMP)
			edata->has_temp = 1;

			rc = lsm6dsv16x_temperature_raw_get(ctx, &edata->temp);
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
	edata->header.accel_fs_idx =
		LSM6DSV16X_ACCEL_FS_VAL_TO_FS_IDX(config->accel_fs_map[data->accel_fs]);
	edata->header.gyro_fs = data->gyro_fs;
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

void lsm6dsv16x_submit_sync(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct device *dev = cfg->sensor;

	if (!cfg->is_streaming) {
		lsm6dsv16x_submit_sample(dev, iodev_sqe);
	} else if (IS_ENABLED(CONFIG_LSM6DSV16X_STREAM)) {
		lsm6dsv16x_submit_stream(dev, iodev_sqe);
	} else {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
	}
}

void lsm6dsv16x_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	if (!lsm6dsv16x_is_active(dev)) {
		return;
	}

	struct rtio_work_req *req = rtio_work_req_alloc();

	if (req == NULL) {
		LOG_ERR("RTIO work item allocation failed. Consider to increase "
			"CONFIG_RTIO_WORKQ_POOL_ITEMS.");
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	rtio_work_req_submit(req, iodev_sqe, lsm6dsv16x_submit_sync);
}
