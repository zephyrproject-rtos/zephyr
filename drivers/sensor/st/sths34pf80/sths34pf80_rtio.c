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
 * The output registers are read with asynchronous bus (I2C or SPI) transfers
 * queued on the bus iodev by rtio_read_regs_async(). No thread or work queue
 * is involved: the raw bytes are read straight into the frame handed to the
 * decoder, and the request is completed from a completion callback.
 */

#include <errno.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/drivers/sensor/sths34pf80.h>
#include <zephyr/logging/log.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/rtio/regmap.h>
#include "sths34pf80.h"
#include "sths34pf80_decoder.h"
#include "sths34pf80_rtio.h"

LOG_MODULE_DECLARE(STHS34PF80, CONFIG_SENSOR_LOG_LEVEL);

#define STHS34PF80_ALL_VALUES BIT_MASK(STHS34PF80_IDX_COUNT)

/**
 * @brief Complete the sensor request once the bus transfers are done.
 *
 * Called by RTIO after the last bus transfer. The result of every transfer
 * is collected from the completion queue of the driver RTIO context.
 *
 * @param r       RTIO context of the device.
 * @param sqe     Callback submission queue entry; its userdata is the sensor request.
 * @param result  Result of the callback entry (unused).
 * @param arg0    Sensor device (unused).
 */
static void sths34pf80_complete(struct rtio *r, const struct rtio_sqe *sqe, int result, void *arg0)
{
	struct rtio_iodev_sqe *iodev_sqe = sqe->userdata;
	struct rtio_cqe *cqe;
	int err = 0;

	ARG_UNUSED(result);
	ARG_UNUSED(arg0);

	while ((cqe = rtio_cqe_consume(r)) != NULL) {
		if (err == 0) {
			err = cqe->result;
		}
		rtio_cqe_release(r, cqe);
	}

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
	struct sths34pf80_data *data = dev->data;
	const uint32_t min_buf_len = sizeof(struct sths34pf80_rtio_data);
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

	/*
	 * Two register blocks, each read into the frame. The read mask sets the
	 * SPI read bit (0x80); it is 0x00 on I2C.
	 *
	 * STdC equivalent: block 1 = FUNC_STATUS + TOBJECT + TAMBIENT,
	 *                  block 2 = TOBJ_COMP + TPRESENCE + TMOTION + TAMB_SHOCK.
	 */
	struct rtio_regs_list regs_list[] = {
		{
			.reg_addr = STHS34PF80_RTIO_REG_BLOCK1 | dev_cfg->read_mask,
			.bufp = edata->block1,
			.len = STHS34PF80_RTIO_LEN_BLOCK1,
		},
		{
			.reg_addr = STHS34PF80_RTIO_REG_BLOCK2 | dev_cfg->read_mask,
			.bufp = edata->block2,
			.len = STHS34PF80_RTIO_LEN_BLOCK2,
		},
	};
	struct rtio_regs regs = {
		.rtio_regs_num = ARRAY_SIZE(regs_list),
		.rtio_regs_list = regs_list,
	};

	/* Queues write+read per block, then the callback. On failure it completes iodev_sqe. */
	rtio_read_regs_async(data->rtio_ctx, data->iodev, dev_cfg->bus_type, &regs, iodev_sqe, dev,
			     sths34pf80_complete);
}
