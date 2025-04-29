/*
 * Copyright (c) 2024 Cienet
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bma4xx

#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/work.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/sensor_clock.h>

#include "bma4xx.h"
#include "bma4xx_decoder.h"
#include "bma4xx_defs.h"
#include "bma4xx_rtio.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bma4xx, CONFIG_SENSOR_LOG_LEVEL);

static void bma4xx_complete_result(struct rtio *ctx, const struct rtio_sqe *sqe, void *arg)
{
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
	LOG_DBG("bma4xx_submit_fetch completed");
}

/*
 * RTIO submit and encoding
 */

static void bma4xx_submit_one_shot(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct bma4xx_data *bma4xx = dev->data;
	const struct bma4xx_config *drv_cfg = dev->config;
	struct bma4xx_encoded_data *edata;
	uint8_t *buf;
	uint32_t buf_len;
	uint32_t min_buf_len = sizeof(struct bma4xx_encoded_data);
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
	edata = (struct bma4xx_encoded_data *)buf;

	if (IS_ENABLED(CONFIG_BMA4XX_STREAM)) {
		edata->header.is_fifo = false;
	}

	edata->header.accel_fs = bma4xx->cfg.accel_fs_range;

	rc = sensor_clock_get_cycles(&cycles);
	if (rc != 0) {
		LOG_ERR("Failed to get sensor clock cycles");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	edata->header.timestamp = sensor_clock_cycles_to_ns(cycles);

	struct rtio_sqe *write_accel_sqe = rtio_sqe_acquire(bma4xx->r);
	struct rtio_sqe *read_accel_sqe = rtio_sqe_acquire(bma4xx->r);

	if (!write_accel_sqe || !read_accel_sqe) {
		LOG_ERR("Failed to acquire RTIO SQEs");
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	const uint8_t reg_accel = BMA4XX_REG_DATA_8;

	rtio_sqe_prep_tiny_write(write_accel_sqe, bma4xx->iodev, RTIO_PRIO_HIGH, &reg_accel, 1,
				 NULL);
	write_accel_sqe->flags |= RTIO_SQE_TRANSACTION;

	rtio_sqe_prep_read(read_accel_sqe, bma4xx->iodev, RTIO_PRIO_HIGH, edata->accel_xyz_raw_data,
			   BMA4XX_REG_DATA_13 - BMA4XX_REG_DATA_8 + 1, NULL);
	read_accel_sqe->flags |= RTIO_SQE_CHAINED;

	if (drv_cfg->bus_type == BMA4XX_BUS_I2C) {
		read_accel_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
	}

#ifdef CONFIG_BMA4XX_TEMPERATURE
	struct rtio_sqe *write_temp_sqe = rtio_sqe_acquire(bma4xx->r);
	struct rtio_sqe *read_temp_sqe = rtio_sqe_acquire(bma4xx->r);

	if (!write_temp_sqe || !read_temp_sqe) {
		LOG_ERR("Failed to acquire RTIO SQEs");
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	const uint8_t reg_temp = BMA4XX_REG_TEMPERATURE;

	rtio_sqe_prep_tiny_write(write_temp_sqe, bma4xx->iodev, RTIO_PRIO_HIGH, &reg_temp, 1, NULL);
	write_temp_sqe->flags |= RTIO_SQE_TRANSACTION;

	rtio_sqe_prep_read(read_temp_sqe, bma4xx->iodev, RTIO_PRIO_HIGH, &edata->temp, 1, NULL);
	read_temp_sqe->flags |= RTIO_SQE_CHAINED;

	if (drv_cfg->bus_type == BMA4XX_BUS_I2C) {
		read_temp_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
	}
#endif /* CONFIG_BMA4XX_TEMPERATURE */

	struct rtio_sqe *complete_sqe = rtio_sqe_acquire(bma4xx->r);

	if (!complete_sqe) {
		LOG_ERR("Failed to acquire RTIO SQEs");
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	rtio_sqe_prep_callback_no_cqe(complete_sqe, bma4xx_complete_result, (void *)dev, iodev_sqe);
	rtio_submit(bma4xx->r, 0);
}

void bma4xx_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;

	if (!cfg->is_streaming) {
		bma4xx_submit_one_shot(dev, iodev_sqe);
	} else if (IS_ENABLED(CONFIG_BMA4XX_STREAM)) {
		bma4xx_submit_stream(dev, iodev_sqe);
	} else {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
	}
}
