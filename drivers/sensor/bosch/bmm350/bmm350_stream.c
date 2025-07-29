/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/check.h>
#include "bmm350.h"
#include "bmm350_stream.h"
#include "bmm350_decoder.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(BMM350_STREAM, CONFIG_SENSOR_LOG_LEVEL);

enum bmm350_stream_state {
	BMM350_STREAM_OFF = 0,
	BMM350_STREAM_ON = 1,
	BMM350_STREAM_BUSY = 2,
};

static void bmm350_stream_result(const struct device *dev, int err)
{
	struct bmm350_data *data = dev->data;
	struct rtio_iodev_sqe *iodev_sqe = data->stream.iodev_sqe;

	data->stream.iodev_sqe = NULL;
	if (err < 0) {
		rtio_iodev_sqe_err(iodev_sqe, err);
	} else {
		rtio_iodev_sqe_ok(iodev_sqe, 0);
	}
}

static void bmm350_stream_event_complete(struct rtio *ctx, const struct rtio_sqe *sqe, void *arg0)
{
	struct rtio_iodev_sqe *iodev_sqe = (struct rtio_iodev_sqe *)arg0;
	struct sensor_read_config *cfg = (struct sensor_read_config *)iodev_sqe->sqe.iodev->data;
	const struct device *dev = (const struct device *)sqe->userdata;
	struct bmm350_data *data = dev->data;
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
		goto bmm350_stream_evt_finish;
	}

	/* We've allocated the data already, just grab the pointer to fill comp-data
	 * now that the bus transfer is complete.
	 */
	err = rtio_sqe_rx_buf(iodev_sqe, 0, 0, &buf, &buf_len);

	CHECKIF(err != 0 || !buf || buf_len < sizeof(struct bmm350_encoded_data)) {
		LOG_ERR("Couldn't get encoded buffer on completion");
		err = -EIO;
		goto bmm350_stream_evt_finish;
	}

	err = bmm350_encode(dev, cfg, true, buf);
	if (err != 0) {
		LOG_ERR("Failed to encode frame: %d", err);
		goto bmm350_stream_evt_finish;
	}

bmm350_stream_evt_finish:
	atomic_set(&data->stream.state, BMM350_STREAM_ON);
	bmm350_stream_result(dev, err);
}

static void bmm350_event_handler(const struct device *dev)
{
	struct bmm350_data *data = dev->data;
	const struct bmm350_config *cfg = dev->config;
	struct rtio_iodev_sqe *iodev_sqe = data->stream.iodev_sqe;
	uint8_t *buf = NULL;
	uint32_t buf_len = 0;
	int err;

	CHECKIF(!data->stream.iodev_sqe ||
		FIELD_GET(RTIO_SQE_CANCELED, iodev_sqe->sqe.flags)) {

		LOG_WRN("Callback triggered with no streaming submission - Disabling interrupts");

		(void)gpio_pin_interrupt_configure_dt(&cfg->drdy_int, GPIO_INT_DISABLE);

		err = bmm350_prep_reg_write_async(dev, BMM350_REG_INT_CTRL, 0, NULL);
		if (err >= 0) {
			rtio_submit(cfg->bus.rtio.ctx, 0);
		}

		(void)atomic_set(&data->stream.state, BMM350_STREAM_OFF);

		return;
	}

	CHECKIF(atomic_cas(&data->stream.state, BMM350_STREAM_ON, BMM350_STREAM_BUSY) == false) {
		LOG_WRN("Callback triggered while stream is busy. Ignoring request");
		return;
	}

	err = rtio_sqe_rx_buf(iodev_sqe,
			      sizeof(struct bmm350_encoded_data),
			      sizeof(struct bmm350_encoded_data),
			      &buf, &buf_len);
	CHECKIF(err != 0 || buf_len < sizeof(struct bmm350_encoded_data)) {
		LOG_ERR("Failed to allocate BMM350 encoded buffer: %d", err);
		bmm350_stream_result(dev, -ENOMEM);
		return;
	}

	struct bmm350_encoded_data *edata = (struct bmm350_encoded_data *)buf;
	struct rtio_sqe *read_sqe = NULL;
	struct rtio_sqe *cb_sqe;

	err = bmm350_prep_reg_read_async(dev, BMM350_REG_MAG_X_XLSB,
					 edata->payload.buf, sizeof(edata->payload.buf),
					 &read_sqe);
	CHECKIF(err < 0 || !read_sqe) {
		bmm350_stream_result(dev, err);
		return;
	}
	read_sqe->flags |= RTIO_SQE_CHAINED;

	cb_sqe = rtio_sqe_acquire(cfg->bus.rtio.ctx);

	rtio_sqe_prep_callback_no_cqe(cb_sqe, bmm350_stream_event_complete,
				      iodev_sqe, (void *)dev);

	rtio_submit(cfg->bus.rtio.ctx, 0);
}

static void bmm350_gpio_callback(const struct device *port, struct gpio_callback *cb, uint32_t pin)
{
	struct bmm350_stream *stream = CONTAINER_OF(cb, struct bmm350_stream, cb);
	const struct device *dev = stream->dev;

	bmm350_event_handler(dev);
}

void bmm350_stream_submit(const struct device *dev,
			  struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *read_config = iodev_sqe->sqe.iodev->data;
	struct bmm350_data *data = dev->data;
	const struct bmm350_config *cfg = dev->config;
	int err;

	if ((read_config->count != 1) ||
	    (read_config->triggers[0].trigger != SENSOR_TRIG_DATA_READY)) {
		LOG_ERR("Only SENSOR_TRIG_DATA_READY is supported");
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
		return;
	}

	data->stream.iodev_sqe = iodev_sqe;

	if (atomic_cas(&data->stream.state, BMM350_STREAM_OFF, BMM350_STREAM_ON)) {
		/* Set PMU command configuration */
		err = bmm350_prep_reg_write_async(dev, BMM350_REG_INT_CTRL, cfg->int_flags, NULL);
		if (err < 0) {
			bmm350_stream_result(dev, err);
			return;
		}
		rtio_submit(cfg->bus.rtio.ctx, 0);

		err = gpio_pin_interrupt_configure_dt(&cfg->drdy_int, GPIO_INT_EDGE_TO_ACTIVE);
		if (err < 0) {
			bmm350_stream_result(dev, err);
			return;
		}
	}
}

int bmm350_stream_init(const struct device *dev)
{
	struct bmm350_data *data = dev->data;
	const struct bmm350_config *cfg = dev->config;
	int err;

	/** Needed to get back the device handle from the callback context */
	data->stream.dev = dev;

	(void)atomic_set(&data->stream.state, BMM350_STREAM_OFF);

	if (!device_is_ready(cfg->drdy_int.port)) {
		LOG_ERR("INT device is not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&cfg->drdy_int, GPIO_INPUT);
	if (err < 0) {
		return err;
	}

	gpio_init_callback(&data->stream.cb, bmm350_gpio_callback, BIT(cfg->drdy_int.pin));

	err = gpio_add_callback(cfg->drdy_int.port, &data->stream.cb);
	if (err < 0) {
		return err;
	}

	err = gpio_pin_interrupt_configure_dt(&cfg->drdy_int, GPIO_INT_DISABLE);
	if (err < 0) {
		return err;
	}


	return 0;
}
