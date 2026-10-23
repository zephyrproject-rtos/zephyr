/*
 * Copyright (c) 2023 Google LLC
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/sys/check.h>
#include <zephyr/rtio/rtio.h>

#include "ictx53xx.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ICTX53XX_ASYNC, CONFIG_SENSOR_LOG_LEVEL);

static uint32_t ictx53xx_reg_to_us(uint8_t reg)
{
	switch (reg) {
	case INV_ICT_MODE_CTRL_REG_ODR_320_HZ:
		return 3125;
	case INV_ICT_MODE_CTRL_REG_ODR_200_HZ:
		return  5000;
	case INV_ICT_MODE_CTRL_REG_ODR_100_HZ:
		return 10000;
	case INV_ICT_MODE_CTRL_REG_ODR_50_HZ:
		return 20000;
	case INV_ICT_MODE_CTRL_REG_ODR_20_HZ:
		return 50000;
	case INV_ICT_MODE_CTRL_REG_ODR_10_HZ:
		return 100000;
	case INV_ICT_MODE_CTRL_REG_ODR_5_HZ:
		return 200000;
	}
	return -EINVAL;
}

void ictx53xx_complete_cb(struct rtio *rtio_ctx, const struct rtio_sqe *sqe, int result, void *arg0)
{
	ARG_UNUSED(result);

	struct rtio_iodev_sqe *parent_iodev_sqe = (struct rtio_iodev_sqe *)arg0;
	int rc = rtio_flush_completion_queue(rtio_ctx);

	if (rc != 0) {
		rtio_iodev_sqe_err(parent_iodev_sqe, rc);
		return;
	}

	rtio_iodev_sqe_ok(parent_iodev_sqe, 0);
}

uint8_t ictx53xx_encode_channel(enum sensor_channel chan)
{
	uint8_t encode_bmask = 0;

	switch (chan) {
	case SENSOR_CHAN_DIE_TEMP:
		encode_bmask = BIT(0);
		break;
	case SENSOR_CHAN_MAGN_X:
		encode_bmask = BIT(1);
		break;
	case SENSOR_CHAN_MAGN_Y:
		encode_bmask = BIT(2);
		break;
	case SENSOR_CHAN_MAGN_Z:
		encode_bmask = BIT(3);
		break;
	case SENSOR_CHAN_MAGN_XYZ:
		encode_bmask = BIT(1) | BIT(2) | BIT(3);
		break;
	case SENSOR_CHAN_ALL:
		encode_bmask = BIT(0) | BIT(1) | BIT(2) | BIT(3);
		break;
	default:
		break;
	}

	return encode_bmask;
}

void ictx53xx_async_fetch(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct ictx53xx_async_fetch_ctx *ctx =
		CONTAINER_OF(dwork, struct ictx53xx_async_fetch_ctx, async_fetch_work);
	const struct sensor_read_config *cfg = ctx->iodev_sqe->sqe.iodev->data;
	const struct sensor_chan_spec *const channels = cfg->channels;
	const size_t num_channels = cfg->count;
	const struct device *dev = cfg->sensor;
	struct ictx53xx_data *data = dev->data;
	uint32_t req_buf_len = sizeof(struct ictx53xx_encoded_data);
	uint32_t buf_len;
	uint8_t *buf;
	struct ictx53xx_encoded_data *edata;
	int rc, wait_count;
	struct rtio_sqe *out_data_sqe;

	/* Get the buffer for the frame, it may be allocated dynamically by the rtio context */
	rc = rtio_sqe_rx_buf(ctx->iodev_sqe, req_buf_len, req_buf_len, &buf, &buf_len);
	if (rc != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", req_buf_len);
		rtio_iodev_sqe_err(ctx->iodev_sqe, rc);
		return;
	}
	edata = (struct ictx53xx_encoded_data *)buf;
	edata->header.timestamp = ctx->timestamp;
	edata->header.channels = 0;
	for (size_t i = 0; i < num_channels; i++) {
		edata->header.channels |= ictx53xx_encode_channel(channels[i].chan_type);
	}
	if (data->driver.id == ICT25349) {
		edata->header.sensitivity = INV_ICT_4_9MT_SENSITIVITY;
	} else {
		edata->header.sensitivity = INV_ICT_2_4MT_SENSITIVITY;
	}

	wait_count = ictx53xx_i2c_prep_reg_read_rtio_async(&data->bus, INV_ICT_TEMP_DATA_LSB,
					edata->readings, sizeof(edata->readings), &out_data_sqe);

	out_data_sqe->flags |= RTIO_SQE_CHAINED;

	struct rtio_sqe *cb_sqe = rtio_sqe_acquire(data->bus.rtio_ctx);

	if (wait_count < 0 || cb_sqe == NULL) {
		rtio_sqe_drop_all(data->bus.rtio_ctx);
		rtio_iodev_sqe_err(ctx->iodev_sqe, -ENOMEM);
		return;
	}

	rtio_sqe_prep_callback_no_cqe(cb_sqe, ictx53xx_complete_cb, (void *)ctx->iodev_sqe, NULL);
	rtio_submit(data->bus.rtio_ctx, wait_count);
}

void ictx53xx_after_start_cb(struct rtio *rtio_ctx, const struct rtio_sqe *sqe,
			     int result, void *arg0)
{
	ARG_UNUSED(result);

	const struct rtio_iodev_sqe *parent_iodev_sqe = (struct rtio_iodev_sqe *)arg0;
	const struct sensor_read_config *cfg = parent_iodev_sqe->sqe.iodev->data;
	const struct device *dev = cfg->sensor;
	struct ictx53xx_data *data = dev->data;
	struct rtio_iodev_sqe *iodev_sqe = (struct rtio_iodev_sqe *)arg0;
	uint64_t cycles;
	int rc;

	rc = sensor_clock_get_cycles(&cycles);
	if (rc != 0) {
		LOG_ERR("Failed to get sensor clock cycles");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	/* save information for the work item */
	data->work_ctx.timestamp = sensor_clock_cycles_to_ns(cycles);
	data->work_ctx.iodev_sqe = iodev_sqe;

	rc = rtio_flush_completion_queue(data->bus.rtio_ctx);
	if (rc != 0) {
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	rc = k_work_schedule(&data->work_ctx.async_fetch_work,
			K_USEC(ICTX53XX_MEASUREMENT_TIME_US +
			       6 * ICTX53XX_MEASUREMENT_EXTRA_TIME_US));
	if (rc == 0) {
		LOG_ERR("The last fetch has not finished yet. "
			"Try again later when the last sensor read operation has finished.");
		rtio_iodev_sqe_err(iodev_sqe, -EBUSY);
	}
}

static void ictx53xx_submit_one_shot(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct ictx53xx_data *data = (struct ictx53xx_data *)dev->data;
	uint8_t val = INV_ICT_MODE_CTRL_REG_MODE_SINGLE_SHOT;

	int wait_count = ictx53xx_i2c_prep_reg_write_rtio_async(&data->bus, INV_ICT_MODE_CTRL_REG,
			&val, sizeof(val), NULL);
	struct rtio_sqe *cb_sqe = rtio_sqe_acquire(data->bus.rtio_ctx);

	if (wait_count >= 0 && cb_sqe != NULL) {
		rtio_sqe_prep_callback_no_cqe(cb_sqe, ictx53xx_after_start_cb, (void *)iodev_sqe,
					      NULL);
		/* The device returns to power-down mode after a single-shot measurement */
		data->mode = INV_ICT_MODE_CTRL_REG_MODE_SLEEP;
		rtio_submit(data->bus.rtio_ctx, wait_count);
	} else {
		rtio_sqe_drop_all(data->bus.rtio_ctx);
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
	}
}

static void ictx53xx_submit_pulsed(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct ictx53xx_data *data = (struct ictx53xx_data *)dev->data;
	uint64_t cycles;
	int rc;

	rc = sensor_clock_get_cycles(&cycles);
	if (rc != 0) {
		LOG_ERR("Failed to get sensor clock cycles");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	/* save information for the work item */
	data->work_ctx.timestamp = sensor_clock_cycles_to_ns(cycles);
	data->work_ctx.iodev_sqe = iodev_sqe;

	rc = k_work_schedule(&data->work_ctx.async_fetch_work,
			K_USEC(ictx53xx_reg_to_us(data->odr)));
	if (rc == 0) {
		LOG_ERR("The last fetch has not finished yet. "
			"Try again later when the last sensor read operation has finished.");
		rtio_iodev_sqe_err(iodev_sqe, -EBUSY);
	}
}

void ictx53xx_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct ictx53xx_data *data = (struct ictx53xx_data *)dev->data;

	if (data->mode == INV_ICT_MODE_CTRL_REG_MODE_PULSED) {
		ictx53xx_submit_pulsed(dev, iodev_sqe);
	} else {
		ictx53xx_submit_one_shot(dev, iodev_sqe);
	}
}
