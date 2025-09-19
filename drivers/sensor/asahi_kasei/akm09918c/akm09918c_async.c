/*
 * Copyright (c) 2023 Google LLC
 * Copyright (c) 2024 Croxel Inc.
 * Copyright (c) 2024 Florian Weber <Florian.Weber@live.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/sys/byteorder.h>

#include "akm09918c.h"

LOG_MODULE_DECLARE(AKM09918C, CONFIG_SENSOR_LOG_LEVEL);

static int akm09918c_flush_cqes(struct rtio *rtio_ctx)
{
	/* Flush completions */
	struct rtio_cqe *cqe;
	int res = 0;

	do {
		cqe = rtio_cqe_consume(rtio_ctx);
		if (cqe != NULL) {
			if ((cqe->result < 0 && res == 0)) {
				LOG_ERR("Bus error: %d", cqe->result);
				res = cqe->result;
			}
			rtio_cqe_release(rtio_ctx, cqe);
		}
	} while (cqe != NULL);
	return res;
}

void akm09918c_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	struct akm09918c_data *data = dev->data;
	const struct sensor_chan_spec *const channels = cfg->channels;
	const size_t num_channels = cfg->count;

	/* Check if the requested channels are supported */
	for (size_t i = 0; i < num_channels; i++) {
		switch (channels[i].chan_type) {
		case SENSOR_CHAN_MAGN_X:
		case SENSOR_CHAN_MAGN_Y:
		case SENSOR_CHAN_MAGN_Z:
		case SENSOR_CHAN_MAGN_XYZ:
		case SENSOR_CHAN_ALL:
			break;
		default:
			LOG_ERR("Unsupported channel type %d", channels[i].chan_type);
			rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
			return;
		}
	}
	struct rtio_sqe *writeByte_sqe = i2c_rtio_copy_reg_write_byte(
		data->rtio_ctx, data->iodev, AKM09918C_REG_CNTL2, AKM09918C_CNTL2_SINGLE_MEASURE);
	struct rtio_sqe *cb_sqe = rtio_sqe_acquire(data->rtio_ctx);

	if (writeByte_sqe != NULL && cb_sqe != NULL) {
		writeByte_sqe->flags |= RTIO_SQE_CHAINED;
		rtio_sqe_prep_callback_no_cqe(cb_sqe, akm09918_after_start_cb, (void *)iodev_sqe,
					      NULL);
		rtio_submit(data->rtio_ctx, 0);
	} else {
		rtio_sqe_drop_all(data->rtio_ctx);
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
	}
}

void akm09918_after_start_cb(struct rtio *rtio_ctx, const struct rtio_sqe *sqe,
			     int result, void *arg0)
{
	ARG_UNUSED(result);

	const struct rtio_iodev_sqe *parent_iodev_sqe = (struct rtio_iodev_sqe *)arg0;
	const struct sensor_read_config *cfg = parent_iodev_sqe->sqe.iodev->data;
	const struct device *dev = cfg->sensor;
	struct akm09918c_data *data = dev->data;
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

	rc = akm09918c_flush_cqes(data->rtio_ctx);
	if (rc != 0) {
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	rc = k_work_schedule(&data->work_ctx.async_fetch_work, K_USEC(AKM09918C_MEASURE_TIME_US));
	if (rc == 0) {
		LOG_ERR("The last fetch has not finished yet. "
			"Try again later when the last sensor read operation has finished.");
		rtio_iodev_sqe_err(iodev_sqe, -EBUSY);
	}
	return;
}

void akm09918_async_fetch(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct akm09918c_async_fetch_ctx *ctx =
		CONTAINER_OF(dwork, struct akm09918c_async_fetch_ctx, async_fetch_work);
	const struct sensor_read_config *cfg = ctx->iodev_sqe->sqe.iodev->data;
	const struct device *dev = cfg->sensor;
	struct akm09918c_data *data = dev->data;
	uint32_t req_buf_len = sizeof(struct akm09918c_encoded_data);
	uint32_t buf_len;
	uint8_t *buf;
	struct akm09918c_encoded_data *edata;
	int rc;

	/* Get the buffer for the frame, it may be allocated dynamically by the rtio context */
	rc = rtio_sqe_rx_buf(ctx->iodev_sqe, req_buf_len, req_buf_len, &buf, &buf_len);
	if (rc != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", req_buf_len);
		rtio_iodev_sqe_err(ctx->iodev_sqe, rc);
		return;
	}
	edata = (struct akm09918c_encoded_data *)buf;

	struct rtio_sqe *burstRead_sqe =
		i2c_rtio_copy_reg_burst_read(data->rtio_ctx, data->iodev, AKM09918C_REG_ST1,
					     &(edata->reading), sizeof(edata->reading));

	edata->header.timestamp = ctx->timestamp;

	struct rtio_sqe *cb_sqe = rtio_sqe_acquire(data->rtio_ctx);

	if (burstRead_sqe == NULL || cb_sqe == NULL) {
		rtio_sqe_drop_all(data->rtio_ctx);
		rtio_iodev_sqe_err(ctx->iodev_sqe, -ENOMEM);
		return;
	}

	rtio_sqe_prep_callback_no_cqe(cb_sqe, akm09918_complete_cb, (void *)ctx->iodev_sqe, NULL);

	burstRead_sqe->flags |= RTIO_SQE_CHAINED;
	rtio_submit(data->rtio_ctx, 0);
}

void akm09918_complete_cb(struct rtio *rtio_ctx, const struct rtio_sqe *sqe, int result, void *arg0)
{
	ARG_UNUSED(result);

	struct rtio_iodev_sqe *parent_iodev_sqe = (struct rtio_iodev_sqe *)arg0;
	struct rtio_sqe *parent_sqe = &parent_iodev_sqe->sqe;
	struct akm09918c_encoded_data *edata =
		(struct akm09918c_encoded_data *)(parent_sqe->rx.buf);
	int rc;

	rc = akm09918c_flush_cqes(rtio_ctx);
	if (rc != 0) {
		rtio_iodev_sqe_err(parent_iodev_sqe, rc);
		return;
	}

	if (FIELD_GET(AKM09918C_ST1_DRDY, edata->reading.st1) == 0) {
		LOG_ERR("Data not ready, st1=0x%02x", edata->reading.st1);
		rtio_iodev_sqe_err(parent_iodev_sqe, -EBUSY);
		return;
	}

	edata->reading.data[0] = sys_le16_to_cpu(edata->reading.data[0]);
	edata->reading.data[1] = sys_le16_to_cpu(edata->reading.data[1]);
	edata->reading.data[2] = sys_le16_to_cpu(edata->reading.data[2]);

	rtio_iodev_sqe_ok(parent_iodev_sqe, 0);
}
