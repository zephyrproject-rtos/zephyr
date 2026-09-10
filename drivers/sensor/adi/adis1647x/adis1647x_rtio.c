/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/rtio/rtio.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>

#include "adis1647x.h"

LOG_MODULE_DECLARE(adis1647x, CONFIG_SENSOR_LOG_LEVEL);

/*
 * Burst command frame. The transfer length matches the full burst response, so
 * the command occupies the first two bytes and the remainder is don't-care.
 */
static const uint8_t adis1647x_burst_tx[sizeof(struct adis1647x_burst_data)] = {
	ADIS1647X_BURST_CMD,
	0x00,
};

static void adis1647x_complete_result(struct rtio *ctx, const struct rtio_sqe *sqe, int result,
				      void *arg)
{
	ARG_UNUSED(arg);
	struct rtio_iodev_sqe *iodev_sqe = (struct rtio_iodev_sqe *)sqe->userdata;
	struct rtio_cqe *cqe;
	int err = 0;

	do {
		cqe = rtio_cqe_consume(ctx);
		if (cqe != NULL) {
			err = cqe->result;
			rtio_cqe_release(ctx, cqe);
		}
	} while (cqe != NULL);

	if (err != 0) {
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}

	uint8_t *buf;
	uint32_t buf_len;

	if (rtio_sqe_rx_buf(iodev_sqe, sizeof(struct adis1647x_sample_data),
			    sizeof(struct adis1647x_sample_data), &buf, &buf_len) != 0) {
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	struct adis1647x_sample_data *sample = (struct adis1647x_sample_data *)buf;

	err = adis1647x_verify_burst(&sample->burst_data);
	if (err != 0) {
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}

	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

static void adis1647x_submit_one_shot(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct adis1647x_data *data = dev->data;
	uint32_t min_buffer_len = sizeof(struct adis1647x_sample_data);
	uint8_t *buffer;
	uint32_t buffer_len;
	int rc;

	rc = rtio_sqe_rx_buf(iodev_sqe, min_buffer_len, min_buffer_len, &buffer, &buffer_len);
	if (rc != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buffer_len);
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	struct adis1647x_sample_data *sample = (struct adis1647x_sample_data *)buffer;

	sample->accel_scale_num = data->accel_scale_num;
	sample->gyro_scale_num = data->gyro_scale_num;

	struct rtio_sqe *xfer_sqe = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *complete_sqe = rtio_sqe_acquire(data->rtio_ctx);

	if (xfer_sqe == NULL || complete_sqe == NULL) {
		LOG_ERR("Failed to acquire RTIO SQEs");
		rtio_sqe_drop_all(data->rtio_ctx);
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	rtio_sqe_prep_transceive(xfer_sqe, data->iodev, RTIO_PRIO_NORM, adis1647x_burst_tx,
				 (uint8_t *)&sample->burst_data,
				 sizeof(struct adis1647x_burst_data), NULL);
	xfer_sqe->flags |= RTIO_SQE_CHAINED;

	rtio_sqe_prep_callback_no_cqe(complete_sqe, adis1647x_complete_result, (void *)dev,
				      iodev_sqe);

	rtio_submit(data->rtio_ctx, 0);
}

void adis1647x_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg =
		(const struct sensor_read_config *)iodev_sqe->sqe.iodev->data;

	if (!cfg->is_streaming) {
		adis1647x_submit_one_shot(dev, iodev_sqe);
	} else {
#ifdef CONFIG_ADIS1647X_STREAM
		adis1647x_submit_stream(dev, iodev_sqe);
#else
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
#endif
	}
}
