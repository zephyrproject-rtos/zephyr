/*
 * Copyright (c) 2025 Sentry Technologies ApS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "iim4623x.h"
#include "iim4623x_bus.h"
#include "iim4623x_decoder.h"
#include "iim4623x_stream.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(iim4623x_stream, CONFIG_SENSOR_LOG_LEVEL);

/* Get the packet size of a streaming data packet given the enabled channels */
#define IIM4623X_STRM_PCK_LEN(chans)                                                               \
	(sizeof(struct iim4623x_pck_strm) - (chans.temp ? 0 : 4) - (chans.accel ? 0 : 12) -        \
	 (chans.gyro ? 0 : 12) - (chans.delta_vel ? 0 : 12) - (chans.delta_angle ? 0 : 12))

static void iim4623x_stream_event_complete(struct rtio *ctx, const struct rtio_sqe *sqe, void *arg)
{
	const struct device *dev = arg;
	struct iim4623x_data *data = dev->data;
	struct rtio_iodev_sqe *iodev_sqe = data->stream.iodev_sqe;
	struct iim4623x_encoded_data *edata = sqe->userdata;
	int ret;

	if (data->stream.data_opt == SENSOR_STREAM_DATA_DROP) {
		/* Data has been consumed from the iim4623x but should just be dropped */
		/**
		 * TODO: it seems undocumented, but other sensor drivers still provide timestamp and
		 * event type (e.g. data-ready) so let's do the same for now
		 */
		edata->header.chans.msk = 0x00;
		(void)memset(&edata->payload, 0, sizeof(struct iim4623x_pck_strm_payload));

		goto out;
	};

	/* Parse/check reply */
	struct iim4623x_pck_strm *packet = (struct iim4623x_pck_strm *)data->trx_buf;
	struct iim4623x_pck_postamble *postamble;
	uint16_t checksum;

	if (sys_be16_to_cpu(packet->preamble.header) != IIM4623X_PCK_HEADER_RX) {
		LOG_ERR("Bad reply header");
		ret = -EIO;
		goto out;
	}

	if (packet->preamble.type != IIM4623X_STRM_PCK_TYPE) {
		LOG_ERR("Bad reply type");
		ret = -EIO;
		goto out;
	}

	/* Locate postamble by advancing past the reply reg_val buffer */
	postamble = IIM4623X_GET_POSTAMBLE(packet);

	/* Verify checksum */
	checksum = iim4623x_calc_checksum((uint8_t *)packet);
	if (checksum != sys_be16_to_cpu(postamble->checksum)) {
		LOG_ERR("Bad checksum, exp: 0x%.4x, got: 0x%.4x", checksum,
			sys_be16_to_cpu(postamble->checksum));
		ret = -EIO;
		goto out;
	}

	/* Copy register contents */
	(void)memcpy(&edata->payload, &packet->payload, sizeof(struct iim4623x_pck_strm_payload));

	/* Convert wire endianness to cpu */
	iim4623x_payload_be_to_cpu(&edata->payload);

	edata->header.data_ready = true;

out:
	ret = rtio_flush_completion_queue(ctx);
	if (ret) {
		rtio_iodev_sqe_err(iodev_sqe, ret);
	} else {
		rtio_iodev_sqe_ok(iodev_sqe, 0);
	}
}

static void iim4623x_stream_stop_complete(struct rtio *ctx, const struct rtio_sqe *sqe, void *arg)
{
	const struct device *dev = arg;
	struct iim4623x_data *data = dev->data;
	int ret;

	data->stream.iodev_sqe = NULL;

	/**
	 * Note that there is no good way to check that the stop-streaming cmd was received. The
	 * datasheet specifically mentions:
	 *
	 * > A ‘no’ response for more than ODR rate is a good indicator that the STOP sequence
	 * > is obtained
	 *
	 */
	atomic_cas(&data->busy, 1, 0);

	ret = rtio_flush_completion_queue(ctx);
	if (ret) {
		LOG_ERR("Failed completing stream-stop, ret: %d", ret);
	}
}

static void iim4623x_stream_stop(const struct device *dev, size_t read_len)
{
	struct iim4623x_data *data = dev->data;
	struct rtio_sqe *comp_sqe;
	int ret;

	/**
	 * iim4623x requires the stop_streaming cmd to arrive while data is ready. It also
	 * requires all of the available data to be read otherwise it becomes unresponsive.
	 */
	(void)memset(data->trx_buf, 0, read_len);
	ret = iim4623x_prepare_cmd(dev, IIM4623X_CMD_STOP_STREAMING, NULL, 0);
	if (ret < 0) {
		LOG_ERR("Failed preparing stop streaming");
		return;
	}

	ret = iim4623x_bus_prep_write(dev, data->trx_buf, read_len, &comp_sqe);
	if (ret < 0) {
		LOG_ERR("Failed preparing to send stop streaming");
		return;
	}

	rtio_sqe_prep_callback_no_cqe(comp_sqe, iim4623x_stream_stop_complete, (void *)dev, NULL);

	rtio_submit(data->rtio.ctx, 0);
}

void iim4623x_stream_event(const struct device *dev)
{
	struct iim4623x_data *data = dev->data;
	struct rtio_iodev_sqe *iodev_sqe = data->stream.iodev_sqe;
	uint32_t min_buf_len = sizeof(struct iim4623x_encoded_data);
	struct iim4623x_encoded_data *edata;
	struct rtio_sqe *comp_sqe;
	size_t read_len = IIM4623X_STRM_PCK_LEN(data->edata.header.chans);
	uint32_t buf_len;
	uint8_t *buf;
	int ret;

	if (!iodev_sqe || FIELD_GET(RTIO_SQE_CANCELED, iodev_sqe->sqe.flags)) {
		/* No active stream sqe, leave streaming mode */
		iim4623x_stream_stop(dev, read_len);
		return;
	}

	/* Fetch data async and complete iodev_sqe within completion callback */
	ret = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	if (ret || !buf || buf_len < min_buf_len) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buf_len);
		goto err_out;
	}

	edata = (struct iim4623x_encoded_data *)buf;

	ret = iim4623x_encode(dev, edata);
	if (ret) {
		LOG_ERR("Failed to encode");
		goto err_out;
	}

	ret = iim4623x_bus_prep_read(dev, data->trx_buf, read_len, &comp_sqe);
	if (ret < 0) {
		LOG_ERR("Prepping read, ret: %d", ret);
		goto err_out;
	}

	rtio_sqe_prep_callback_no_cqe(comp_sqe, iim4623x_stream_event_complete, (void *)dev, buf);

	rtio_submit(data->rtio.ctx, 0);
	return;

err_out:
	rtio_iodev_sqe_err(iodev_sqe, ret);
}

static void iim4623x_stream_submit_complete(struct rtio *ctx, const struct rtio_sqe *sqe, void *arg)
{
	rtio_flush_completion_queue(ctx);
}

void iim4623x_stream_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *read_cfg = iodev_sqe->sqe.iodev->data;
	struct iim4623x_data *data = dev->data;
	struct rtio_sqe *comp_sqe;
	int ret;

	if ((read_cfg->count != 1) || (read_cfg->triggers[0].trigger != SENSOR_TRIG_DATA_READY) ||
	    (read_cfg->triggers[0].opt == SENSOR_STREAM_DATA_NOP)) {
		LOG_ERR("Unsupported read config");
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
		return;
	}

	if (!data->stream.iodev_sqe) {
		/* Can't start streaming if device is busy */
		if (!atomic_cas(&data->busy, 0, 1)) {
			rtio_iodev_sqe_err(iodev_sqe, -EBUSY);
			return;
		}

		data->stream.iodev_sqe = iodev_sqe;
		data->stream.drdy_en = true;
		data->stream.data_opt = read_cfg->triggers[0].opt;

		/* Kick off streaming */
		ret = iim4623x_prepare_cmd(dev, IIM4623X_CMD_START_STREAMING, NULL, 0);
		if (ret < 0) {
			LOG_ERR("Failed to start streaming");
			rtio_iodev_sqe_err(iodev_sqe, ret);
			return;
		}

		ret = iim4623x_bus_prep_write(dev, data->trx_buf, ret, &comp_sqe);
		if (ret < 0) {
			LOG_ERR("Failed to prep write");
			rtio_iodev_sqe_err(iodev_sqe, ret);
		}

		/* TODO: consider using SQE flags instead of callback to flush CQEs */
		rtio_sqe_prep_callback_no_cqe(comp_sqe, iim4623x_stream_submit_complete, NULL,
					      NULL);

		rtio_submit(data->rtio.ctx, 0);
	}
}
