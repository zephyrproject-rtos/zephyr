/*
 * Bosch BMA400 3-axis accelerometer driver
 * SPDX-FileCopyrightText: Copyright 2026 Luca Gessi lucagessi90@gmail.com
 * SPDX-FileCopyrightText: Copyright 2026 The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bma400

#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/work.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/sensor_clock.h>

#include "bma400.h"
#include "bma400_decoder.h"
#include "bma400_defs.h"
#include "bma400_rtio.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bma400, CONFIG_SENSOR_LOG_LEVEL);

static void bma400_complete_result(struct rtio *ctx, const struct rtio_sqe *sqe, int result,
				   void *arg)
{
	ARG_UNUSED(result);

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
	if (err) {
		rtio_iodev_sqe_err(iodev_sqe, err);
	} else {
		rtio_iodev_sqe_ok(iodev_sqe, 0);
	}
	LOG_DBG("bma400_submit_fetch completed");
}

/*
 * RTIO submit and encoding
 */

static void bma400_submit_one_shot(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct bma400_data *bma400 = dev->data;
	struct bma400_encoded_data *edata;
	uint8_t *buf;
	uint32_t buf_len;
	uint32_t min_buf_len = sizeof(struct bma400_encoded_data);
	uint64_t cycles;
	int rc;

	/* Get the buffer for the frame, it may be allocated dynamically by the rtio context */
	rc = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	if (rc != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buf_len);
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	/* Prepare response */
	edata = (struct bma400_encoded_data *)buf;

	if (IS_ENABLED(CONFIG_BMA400_STREAM)) {
		edata->header.is_fifo = false;
	}

	edata->header.accel_fs = bma400->cfg.accel_fs_range;

	rc = sensor_clock_get_cycles(&cycles);
	if (rc != 0) {
		LOG_ERR("Failed to get sensor clock cycles");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	edata->header.timestamp = sensor_clock_cycles_to_ns(cycles);

	struct rtio_sqe *write_accel_sqe = rtio_sqe_acquire(bma400->r);
	struct rtio_sqe *read_accel_sqe = rtio_sqe_acquire(bma400->r);
	struct rtio_sqe *complete_sqe = rtio_sqe_acquire(bma400->r);

	if (!write_accel_sqe || !read_accel_sqe || !complete_sqe) {
		LOG_ERR("Failed to acquire RTIO SQEs");
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	const uint8_t reg = 0x80 | BMA400_REG_ACC_X_LSB;
	/* reg + dummy byte in SPI */
	rtio_sqe_prep_tiny_write(write_accel_sqe, bma400->iodev, RTIO_PRIO_NORM, &reg, 1, NULL);
	write_accel_sqe->flags |= RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_accel_sqe, bma400->iodev, RTIO_PRIO_NORM,
			   &(edata->accel_xyz_raw_data[0]),
			   (BMA400_REG_ACC_Z_MSB - BMA400_REG_ACC_X_LSB) + 1, NULL);
	read_accel_sqe->flags |= RTIO_SQE_CHAINED;

	rtio_sqe_prep_callback_no_cqe(complete_sqe, bma400_complete_result, NULL, iodev_sqe);
	rtio_submit(bma400->r, 0);
}

void bma400_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;

	if (!cfg->is_streaming) {
		bma400_submit_one_shot(dev, iodev_sqe);
	} else if (IS_ENABLED(CONFIG_BMA400_STREAM)) {
		bma400_submit_stream(dev, iodev_sqe);
	} else {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
	}
}
