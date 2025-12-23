/*
 * Copyright (c) 2025 Croxel, Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/check.h>

#define DT_DRV_COMPAT bosch_bmi08x_gyro
#include "bmi08x.h"
#include "bmi08x_bus.h"
#include "bmi08x_gyro_stream.h"
#include "bmi08x_gyro_decoder.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(BMI08X_GYRO_ASYNC, CONFIG_SENSOR_LOG_LEVEL);

static void bmi08x_complete_result(struct rtio *ctx, const struct rtio_sqe *sqe, int result,
				   void *arg)
{
	struct rtio_iodev_sqe *iodev_sqe = (struct rtio_iodev_sqe *)arg;

	(void)rtio_flush_completion_queue(ctx);
	if (result >= 0) {
		rtio_iodev_sqe_ok(iodev_sqe, 0);
	} else {
		rtio_iodev_sqe_err(iodev_sqe, result);
	}
}

static void bmi08x_submit_one_shot(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	uint32_t buf_len_req = sizeof(struct bmi08x_gyro_encoded_data);
	struct bmi08x_gyro_encoded_data *edata;
	uint32_t buf_len;
	int err;

	err = rtio_sqe_rx_buf(iodev_sqe, buf_len_req, buf_len_req, (uint8_t **)&edata, &buf_len);
	CHECKIF(err < 0 || buf_len < buf_len_req || !edata) {
		LOG_ERR("Failed to get a read-buffer of size %u bytes", buf_len_req);
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}
	bmi08x_gyro_encode_header(dev, edata, false);

	struct rtio_sqe *out_sqe;
	struct rtio_sqe *complete_sqe;
	const struct bmi08x_gyro_config *config = dev->config;

	err = bmi08x_prep_reg_read_rtio_async(&config->rtio_bus, BMI08X_REG_GYRO_X_LSB,
					      (uint8_t *)edata->frame.payload,
					      sizeof(edata->frame.payload), &out_sqe, false);
	if (err < 0) {
		LOG_ERR("Failed to perpare async read operation");
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}
	out_sqe->flags |= RTIO_SQE_CHAINED;

	complete_sqe = rtio_sqe_acquire(config->rtio_bus.ctx);
	if (!complete_sqe) {
		LOG_ERR("Failed to perpare async read operation");
		rtio_sqe_drop_all(config->rtio_bus.ctx);
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}

	rtio_sqe_prep_callback_no_cqe(complete_sqe, bmi08x_complete_result, iodev_sqe, (void *)dev);
	rtio_submit(config->rtio_bus.ctx, 0);
}

void bmi08x_gyro_async_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;

	if (!cfg->is_streaming) {
		bmi08x_submit_one_shot(dev, iodev_sqe);
	} else if (IS_ENABLED(CONFIG_BMI08X_GYRO_STREAM)) {
		bmi08x_gyro_stream_submit(dev, iodev_sqe);
	} else {
		LOG_ERR("Streaming not enabled");
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
	}
}
