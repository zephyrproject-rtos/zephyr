/*
 * Copyright (c) 2025 Nordic Semiconductors ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/sys/check.h>

#include "bme680.h"
#include "bme680_decoder.h"

LOG_MODULE_DECLARE(BME680, CONFIG_SENSOR_LOG_LEVEL);

static void bme680_one_shot_complete(struct rtio *ctx, const struct rtio_sqe *sqe, void *arg0)
{
	struct rtio_iodev_sqe *iodev_sqe = (struct rtio_iodev_sqe *)sqe->userdata;
	struct sensor_read_config *cfg = (struct sensor_read_config *)iodev_sqe->sqe.iodev->data;
	const struct device *dev = (const struct device *)arg0;
	uint8_t *buf;
	uint32_t buf_len;
	struct rtio_cqe *cqe;
	int err = 0;

	do {
		cqe = rtio_cqe_consume(ctx);
		if (cqe == NULL) {
			continue;
		}

		/** Keep looping through results until we get the first error.
		 * Usually this causes the remaining CQEs to result in -ECANCELED.
		 */
		if (err == 0) {
			err = cqe->result;
		}
		rtio_cqe_release(ctx, cqe);
	} while (cqe != NULL);

	if (err != 0) {
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}

	/* We've allocated the data already, just grab the pointer to fill comp-data
	 * now that the bus transfer is complete.
	 */
	err = rtio_sqe_rx_buf(iodev_sqe, 0, 0, &buf, &buf_len);

	CHECKIF(err != 0 || !buf || buf_len < sizeof(struct bme680_encoded_data)) {
		rtio_iodev_sqe_err(iodev_sqe, -EIO);
		return;
	}

	err = bme680_encode(dev, cfg, buf);
	if (err != 0) {
		LOG_ERR("Failed to encode frame: %d", err);
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}

	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

static void bme680_check_status_cb(struct rtio *ctx, const struct rtio_sqe *sqe, void *arg0)
{
	struct rtio_iodev_sqe *iodev_sqe = (struct rtio_iodev_sqe *)sqe->userdata;
	const struct device *dev = (const struct device *)arg0;
	struct bme680_data *data = dev->data;
	uint8_t *delay_cnt = &data->delay_cnt;
	uint8_t *status = &data->status;
	uint8_t *buf;
	uint32_t buf_len;
	int err;

	if (++(*delay_cnt) > BME680_DELAY_TIMEOUT) {
		LOG_ERR("Timed out waiting for new data");
		rtio_iodev_sqe_err(iodev_sqe, -EIO);
		return;
	}

	if (!(*status & BME680_MSK_NEW_DATA)) {
		/* Not ready, chain another delay and status read */
		struct rtio_sqe *delay_sqe = rtio_sqe_acquire(ctx);

		if (!delay_sqe) {
			LOG_ERR("Failed to acquire RTIO SQEs");
			rtio_sqe_drop_all(ctx);
			rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
			return;
		}

		rtio_sqe_prep_delay(delay_sqe, K_MSEC(1), iodev_sqe);
		delay_sqe->flags |= RTIO_SQE_CHAINED;

		struct rtio_sqe *read_sqe = NULL;
		err = bme680_prep_reg_read_async(dev, BME680_REG_MEAS_STATUS, status, 1, &read_sqe);
		if (err < 0 || !read_sqe) {
			LOG_ERR("Failed to prepare MEAS_STATUS read: %d", err);
			rtio_iodev_sqe_err(iodev_sqe, err);
			return;
		}
		read_sqe->flags |= RTIO_SQE_CHAINED;

		rtio_sqe_prep_callback_no_cqe(read_sqe, bme680_check_status_cb, (void *)dev, iodev_sqe);

		rtio_submit(ctx, 0);
		return;
	}

	/* Data ready, read FIELD0 */
	err = rtio_sqe_rx_buf(iodev_sqe, 0, 0, &buf, &buf_len);

	CHECKIF(err != 0 || !buf || buf_len < sizeof(struct bme680_encoded_data)) {
		rtio_iodev_sqe_err(iodev_sqe, -EIO);
		return;
	}

	struct bme680_encoded_data *edata = (struct bme680_encoded_data *)buf;

	struct rtio_sqe *read_sqe = NULL;
	err = bme680_prep_reg_read_async(dev, BME680_REG_FIELD0,
		edata->payload.buf, sizeof(edata->payload.buf), &read_sqe);
	if (err < 0 || !read_sqe) {
		LOG_ERR("Failed to prepare FIELD0 read: %d", err);
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}
	read_sqe->flags |= RTIO_SQE_CHAINED;

	struct rtio_sqe *complete_sqe = rtio_sqe_acquire(ctx);
	if (!complete_sqe) {
		LOG_ERR("Failed to acquire RTIO SQEs");
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}
	rtio_sqe_prep_callback_no_cqe(complete_sqe, bme680_one_shot_complete, (void *)dev, iodev_sqe);

	rtio_submit(ctx, 0);
}

static void bme680_submit_one_shot(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct bme680_config *cfg = dev->config;
	const struct bme680_bus *bus = &cfg->bus;
	struct rtio *ctx = bus->rtio.ctx;
	struct bme680_data *data = dev->data;
	uint8_t *status = &data->status;
	uint32_t min_buf_len = sizeof(struct bme680_encoded_data);
	uint8_t *buf;
	uint32_t buf_len;
	int err;

	err = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	if (err != 0) {
		LOG_ERR("Failed to allocate a read buffer of size %u bytes", min_buf_len);
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}
	
	struct rtio_sqe *delay_sqe = rtio_sqe_acquire(ctx);
	if (!delay_sqe) {
		LOG_ERR("Failed to acquire RTIO SQEs");
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	/* Start with CTRL_MEAS write */
	struct rtio_sqe *write_sqe = NULL;
	err = bme680_prep_reg_write_async(dev, BME680_REG_CTRL_MEAS, BME680_CTRL_MEAS_VAL, &write_sqe);
	if (err < 0 || !write_sqe) {
		LOG_ERR("Failed to prepare CTRL_MEAS write: %d", err);
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}
	write_sqe->flags |= RTIO_SQE_CHAINED;

	/* Chain a delay */
	rtio_sqe_prep_delay(delay_sqe, K_MSEC(1), (void *)NULL);
	delay_sqe->flags |= RTIO_SQE_CHAINED;

	/* Chain first MEAS_STATUS read */
	struct rtio_sqe *read_sqe = NULL;
	err = bme680_prep_reg_read_async(dev, BME680_REG_MEAS_STATUS, status, 1, &read_sqe);
	if (err < 0 || !read_sqe) {
		LOG_ERR("Failed to prepare MEAS_STATUS read: %d", err);
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}
	read_sqe->flags |= RTIO_SQE_CHAINED;

	rtio_sqe_prep_callback_no_cqe(read_sqe, bme680_check_status_cb, (void *)dev, iodev_sqe);

	rtio_submit(ctx, 0);
}

void bme680_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = (const struct sensor_read_config *) iodev_sqe->sqe.iodev->data;

	if (!cfg->is_streaming) {
		bme680_submit_one_shot(dev, iodev_sqe);
	} else {
		LOG_ERR("Streaming mode not supported");
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
	}
}
