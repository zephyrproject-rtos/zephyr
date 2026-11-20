/*
 * Copyright (c) 2026 Mahendra Sondagar <mahendra@dotcom.co.in>
 * Copyright (c) 2026 Parin Baudhanwala <parin.baudhanwala@dnkmail.in>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RTIO submit implementation of the STHS34PF80 driver.
 *
 * The output registers are read with asynchronous bus (I2C or SPI) transfers queued
 * directly on the bus iodev. No thread or work queue is involved: the raw
 * bytes are read straight into the frame handed to the decoder, and the
 * request is completed from a completion callback.
 */

#include <errno.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/drivers/sensor/sths34pf80.h>
#include <zephyr/logging/log.h>
#include <zephyr/rtio/rtio.h>
#include "sths34pf80.h"
#include "sths34pf80_decoder.h"
#include "sths34pf80_rtio.h"

LOG_MODULE_DECLARE(STHS34PF80, CONFIG_SENSOR_LOG_LEVEL);

#define STHS34PF80_ALL_VALUES BIT_MASK(STHS34PF80_IDX_COUNT)

/**
 * @brief Complete the sensor request once the bus transfers are done.
 *
 * Runs once the last bus transfer of the chain is done. The result of every
 * transfer of the chain is collected from the completion queue.
 *
 * @param r         RTIO context of the device.
 * @param sqe       Callback submission queue entry (unused).
 * @param result    Result of the callback entry (unused).
 * @param arg0      The sensor request (struct rtio_iodev_sqe) to complete.
 */
static void sths34pf80_complete(struct rtio *r, const struct rtio_sqe *sqe, int result, void *arg0)
{
	struct rtio_iodev_sqe *iodev_sqe = arg0;
	struct rtio_cqe *cqe;
	int err = 0;

	ARG_UNUSED(sqe);
	ARG_UNUSED(result);

	do {
		cqe = rtio_cqe_consume(r);
		if (cqe != NULL) {
			if (err == 0) {
				err = cqe->result;
			}
			rtio_cqe_release(r, cqe);
		}
	} while (cqe != NULL);

	if (err != 0) {
		LOG_ERR("Bus transfer failed (%d)", err);
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}

	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

void sths34pf80_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg;
	const struct sensor_chan_spec *channels;
	const struct sths34pf80_config *dev_cfg = dev->config;
	const uint8_t reg_block1 = STHS34PF80_RTIO_REG_BLOCK1 | dev_cfg->read_mask;
	const uint8_t reg_block2 = STHS34PF80_RTIO_REG_BLOCK2 | dev_cfg->read_mask;
	struct sths34pf80_data *data = dev->data;
	const uint32_t min_buf_len = sizeof(struct sths34pf80_rtio_data);
	struct rtio_sqe *wr1, *rd1, *wr2, *rd2, *cb;
	struct sths34pf80_rtio_data *edata;
	uint16_t want = 0;
	uint64_t cycles;
	uint8_t *buf;
	uint32_t buf_len;
	int rc;

	if (iodev_sqe->sqe.op != RTIO_OP_RX) {
		rtio_iodev_sqe_err(iodev_sqe, -EINVAL);
		return;
	}

	cfg = iodev_sqe->sqe.iodev->data;
	channels = cfg->channels;

	if (cfg->is_streaming) {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
		return;
	}

	/* Work out which values were requested */
	for (size_t i = 0; i < cfg->count; i++) {
		int idx;

		if (channels[i].chan_type == SENSOR_CHAN_ALL) {
			want = STHS34PF80_ALL_VALUES;
			break;
		}

		idx = sths34pf80_chan_to_idx(channels[i].chan_type);
		if (idx >= 0) {
			want |= BIT(idx);
		}
	}

	if (want == 0) {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
		return;
	}

	rc = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	if (rc != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buf_len);
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	edata = (struct sths34pf80_rtio_data *)buf;
	edata->valid = want;

	rc = sensor_clock_get_cycles(&cycles);
	if (rc != 0) {
		LOG_ERR("Failed to get sensor clock cycles");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	edata->timestamp = sensor_clock_cycles_to_ns(cycles);

	wr1 = rtio_sqe_acquire(data->rtio_ctx);
	rd1 = rtio_sqe_acquire(data->rtio_ctx);
	wr2 = rtio_sqe_acquire(data->rtio_ctx);
	rd2 = rtio_sqe_acquire(data->rtio_ctx);
	cb = rtio_sqe_acquire(data->rtio_ctx);

	if (wr1 == NULL || rd1 == NULL || wr2 == NULL || rd2 == NULL || cb == NULL) {
		LOG_ERR("Failed to acquire RTIO submission queue entries");
		rtio_sqe_drop_all(data->rtio_ctx);
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	/* Block 1: FUNC_STATUS, TOBJECT, TAMBIENT */
	rtio_sqe_prep_tiny_write(wr1, data->iodev, RTIO_PRIO_NORM, &reg_block1, 1, NULL);
	wr1->flags |= RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(rd1, data->iodev, RTIO_PRIO_NORM, edata->block1,
			   STHS34PF80_RTIO_LEN_BLOCK1, NULL);
	rd1->flags |= RTIO_SQE_CHAINED;

	/* Block 2: TOBJ_COMP, TPRESENCE, TMOTION, TAMB_SHOCK */
	rtio_sqe_prep_tiny_write(wr2, data->iodev, RTIO_PRIO_NORM, &reg_block2, 1, NULL);
	wr2->flags |= RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(rd2, data->iodev, RTIO_PRIO_NORM, edata->block2,
			   STHS34PF80_RTIO_LEN_BLOCK2, NULL);
	rd2->flags |= RTIO_SQE_CHAINED;

	/* Repeated start and stop only exist on I2C */
	if (!dev_cfg->is_spi) {
		rd1->iodev_flags |= RTIO_IODEV_I2C_RESTART | RTIO_IODEV_I2C_STOP;
		rd2->iodev_flags |= RTIO_IODEV_I2C_RESTART | RTIO_IODEV_I2C_STOP;
	}

	/* Complete the sensor request when all the transfers are done */
	rtio_sqe_prep_callback_no_cqe(cb, sths34pf80_complete, iodev_sqe, NULL);

	rtio_submit(data->rtio_ctx, 0);
}
