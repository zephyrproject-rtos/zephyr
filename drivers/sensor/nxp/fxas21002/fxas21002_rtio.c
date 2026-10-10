/*
 * Copyright (c) 2026 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_fxas21002

#include <stdint.h>
#include <string.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/logging/log.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/mpsc_lockfree.h>
#include <zephyr/sys/util.h>

#include "fxas21002.h"

LOG_MODULE_DECLARE(FXAS21002, CONFIG_SENSOR_LOG_LEVEL);

#define FXAS21002_GYRO_SHIFT 16

static int fxas21002_validate_request(const struct sensor_read_config *cfg)
{
	for (size_t i = 0; i < cfg->count; i++) {
		if (cfg->channels[i].chan_idx != 0) {
			return -ENOTSUP;
		}

		switch (cfg->channels[i].chan_type) {
		case SENSOR_CHAN_GYRO_XYZ:
			break;
		default:
			return -ENOTSUP;
		}
	}

	return 0;
}

/* Forward declarations */

static void fxas21002_start_transfer(struct rtio_iodev_sqe *iodev_sqe,
							const struct device *dev);

/* MPSC dispatcher */

static void fxas21002_rtio_start_next(const struct device *dev)
{
	struct fxas21002_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->mpsc_lock);

	if (data->pending_sqe != NULL) {
		k_spin_unlock(&data->mpsc_lock, key);
		return;
	}

#ifdef CONFIG_FXAS21002_STREAM
	if (data->stream_busy || data->stream_defer) {
		k_spin_unlock(&data->mpsc_lock, key);
		return;
	}
#endif

	struct mpsc_node *node = mpsc_pop(&data->io_q);

	if (node == NULL) {
		k_spin_unlock(&data->mpsc_lock, key);
		return;
	}

	struct rtio_iodev_sqe *next_sqe = CONTAINER_OF(node, struct rtio_iodev_sqe, q);

	data->pending_sqe = next_sqe;
	k_spin_unlock(&data->mpsc_lock, key);

	fxas21002_start_transfer(next_sqe, dev);
}

/* Completion callback runs in RTIO executor */

static void fxas21002_complete_cb(struct rtio *r, const struct rtio_sqe *sqe,
				  int res, void *arg)
{
	const struct device *dev = arg;
	struct fxas21002_data *data = dev->data;
	const struct fxas21002_config *config = dev->config;
	struct rtio_iodev_sqe *iodev_sqe = sqe->userdata;
	struct fxas21002_encoded_data *edata;
	uint8_t *buf;
	uint32_t buf_len;
	uint64_t cycles;
	int rc;

	if (res != 0) {
		rtio_iodev_sqe_err(iodev_sqe, res);
		goto check_next;
	}

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	const int status_off = (config->inst_on_bus == FXAS21002_BUS_SPI) ? 1 : 0;
#else
	const int status_off = 0;
#endif

	rc = rtio_sqe_rx_buf(iodev_sqe, sizeof(*edata), sizeof(*edata), &buf, &buf_len);
	if (rc != 0) {
		rtio_iodev_sqe_err(iodev_sqe, rc);
		goto check_next;
	}

	rc = sensor_clock_get_cycles(&cycles);
	if (rc != 0) {
		rtio_iodev_sqe_err(iodev_sqe, rc);
		goto check_next;
	}

	edata = (struct fxas21002_encoded_data *)buf;
	edata->header.base_timestamp_ns = sensor_clock_cycles_to_ns(cycles);
	edata->header.reading_count = 1U;
	edata->status = data->raw_buffer[status_off];
	edata->range = config->range;
	edata->sample_count = 1;

	for (int i = 0; i < FXAS21002_MAX_NUM_CHANNELS; i++) {
		edata->raw[i] = ((int16_t)data->raw_buffer[status_off + 1 + i * 2] << 8) |
						data->raw_buffer[status_off + 2 + i * 2];
	}

	rtio_iodev_sqe_ok(iodev_sqe, 0);

check_next:
	{
		k_spinlock_key_t key = k_spin_lock(&data->mpsc_lock);

#ifdef CONFIG_FXAS21002_STREAM
		bool defer = data->stream_defer && (data->streaming_sqe != NULL);

		data->pending_sqe = NULL;
		if (defer) {
			data->stream_defer = false;
		}
		k_spin_unlock(&data->mpsc_lock, key);

		if (defer) {
			fxas21002_stream_irq_handler(dev);
			return;
		}
#else
		data->pending_sqe = NULL;
		k_spin_unlock(&data->mpsc_lock, key);
#endif
	}
	fxas21002_rtio_start_next(dev);
}

/* RTIO SQE chain builder (I2C: 3 SQEs; SPI: 2 SQEs) */

static void fxas21002_start_transfer(struct rtio_iodev_sqe *iodev_sqe,
									 const struct device *dev)
{
	struct fxas21002_data *data = dev->data;
	const struct fxas21002_config *config = dev->config;
	struct rtio_sqe *cb;
	int rc;

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	if (config->inst_on_bus == FXAS21002_BUS_I2C) {
		static const uint8_t reg_addr = FXAS21002_REG_STATUS;
		struct rtio_sqe *wr_sqe, *rd_sqe;

		wr_sqe = rtio_sqe_acquire(config->r);
		rd_sqe = rtio_sqe_acquire(config->r);
		cb     = rtio_sqe_acquire(config->r);

		if (!wr_sqe || !rd_sqe || !cb) {
			rtio_sqe_drop_all(config->r);
			goto enomem;
		}

		/* START, ADDR+W, REG_STATUS */
		rtio_sqe_prep_tiny_write(wr_sqe, config->bus_iodev, RTIO_PRIO_NORM,
								 &reg_addr, 1, NULL);
		wr_sqe->flags = RTIO_SQE_TRANSACTION;

		/* RESTART, ADDR+R, [status, xH, xL, yH, yL, zH, zL], STOP */
		rtio_sqe_prep_read(rd_sqe, config->bus_iodev, RTIO_PRIO_NORM,
						   data->raw_buffer, 7, NULL);
		rd_sqe->iodev_flags = RTIO_IODEV_I2C_RESTART | RTIO_IODEV_I2C_STOP;
		rd_sqe->flags = RTIO_SQE_CHAINED;

		rtio_sqe_prep_callback_no_cqe(cb, fxas21002_complete_cb, (void *)dev, iodev_sqe);
		goto submit;
	}
#endif

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	if (config->inst_on_bus == FXAS21002_BUS_SPI) {
		struct rtio_sqe *io_sqe;

		data->spi_tx_buf[0] = FXAS21002_REG_STATUS | BIT(7); /* DIR_READ */
		memset(&data->spi_tx_buf[1], 0, 7);

		io_sqe = rtio_sqe_acquire(config->r);
		cb     = rtio_sqe_acquire(config->r);

		if (!io_sqe || !cb) {
			rtio_sqe_drop_all(config->r);
			goto enomem;
		}

		rtio_sqe_prep_transceive(io_sqe, config->bus_iodev, RTIO_PRIO_NORM,
					data->spi_tx_buf, data->raw_buffer, 8, NULL);
		io_sqe->flags = RTIO_SQE_CHAINED;

		rtio_sqe_prep_callback_no_cqe(cb, fxas21002_complete_cb, (void *)dev, iodev_sqe);
		goto submit;
	}
#endif

enomem:
	{
		k_spinlock_key_t key = k_spin_lock(&data->mpsc_lock);

		data->pending_sqe = NULL;
		k_spin_unlock(&data->mpsc_lock, key);
	}
	rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
	fxas21002_rtio_start_next(dev);
	return;

submit:
	rc = rtio_submit(config->r, 0);
	if (rc < 0) {

		k_spinlock_key_t key = k_spin_lock(&data->mpsc_lock);

		data->pending_sqe = NULL;
		k_spin_unlock(&data->mpsc_lock, key);
		rtio_iodev_sqe_err(iodev_sqe, rc);
		fxas21002_rtio_start_next(dev);
	}
}

void fxas21002_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	struct fxas21002_data *data = dev->data;
	int ret;

	if (cfg->is_streaming) {
		IF_ENABLED(CONFIG_FXAS21002_STREAM,
			(fxas21002_submit_stream(dev, iodev_sqe);))
		IF_DISABLED(CONFIG_FXAS21002_STREAM,
			(rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);))
		return;
	}

	ret = fxas21002_validate_request(cfg);
	if (ret < 0) {
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}

	mpsc_push(&data->io_q, &iodev_sqe->q);
	fxas21002_rtio_start_next(dev);
}

static q31_t fxas21002_gyro_to_q31(int16_t raw, uint8_t range)
{
	return (q31_t)(((int64_t)raw * 392699LL *
			(1LL << (31 - FXAS21002_GYRO_SHIFT))) /
		       (360000000LL << range));
}

static int fxas21002_decoder_get_frame_count(const uint8_t *buffer,
						 struct sensor_chan_spec chan_spec,
						 uint16_t *frame_count)
{
	const struct fxas21002_encoded_data *edata = (const struct fxas21002_encoded_data *)buffer;

	if (chan_spec.chan_idx != 0) {
		return -ENOTSUP;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_GYRO_XYZ:
		*frame_count = edata->sample_count;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int fxas21002_decoder_get_size_info(struct sensor_chan_spec chan_spec,
					   size_t *base_size,
					   size_t *frame_size)
{
	if (chan_spec.chan_idx != 0) {
		return -ENOTSUP;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_GYRO_XYZ:
		*base_size = sizeof(struct sensor_three_axis_data);
		*frame_size = sizeof(struct sensor_three_axis_sample_data);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int fxas21002_decoder_decode(const uint8_t *buffer,
					struct sensor_chan_spec chan_spec,
					uint32_t *fit,
					uint16_t max_count,
					void *data_out)
{
	const struct fxas21002_encoded_data *edata = (const struct fxas21002_encoded_data *)buffer;
	struct sensor_three_axis_data *out = data_out;
	uint16_t frame_count;

	if (*fit != 0U) {
		return 0;
	}

	if (max_count == 0U) {
		return 0;
	}

	if (chan_spec.chan_idx != 0) {
		return -ENOTSUP;
	}

	frame_count = MIN(edata->sample_count, max_count);

	out->header.base_timestamp_ns = edata->header.base_timestamp_ns;
	out->header.reading_count = frame_count;
	out->shift = FXAS21002_GYRO_SHIFT;

	for (uint16_t i = 0; i < frame_count; i++) {
		out->readings[i].timestamp_delta = 0;
		out->readings[i].x = fxas21002_gyro_to_q31(edata->raw[0], edata->range);
		out->readings[i].y = fxas21002_gyro_to_q31(edata->raw[1], edata->range);
		out->readings[i].z = fxas21002_gyro_to_q31(edata->raw[2], edata->range);
	}

	*fit = frame_count;

	return frame_count;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = fxas21002_decoder_get_frame_count,
	.get_size_info = fxas21002_decoder_get_size_info,
	.decode = fxas21002_decoder_decode,
};

int fxas21002_get_decoder(const struct device *dev,
			  const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);

	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
