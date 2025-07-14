/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/check.h>
#include <zephyr/drivers/gpio.h>

#include "bmp581.h"
#include "bmp581_stream.h"
#include "bmp581_decoder.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(BMP581_STREAM, CONFIG_SENSOR_LOG_LEVEL);

enum bmp581_stream_state {
	BMP581_STREAM_OFF = 0,
	BMP581_STREAM_ON = 1,
	BMP581_STREAM_BUSY = 2,
};

static void bmp581_stream_event_complete(struct rtio *ctx, const struct rtio_sqe *sqe, void *arg0)
{
	struct rtio_iodev_sqe *iodev_sqe = (struct rtio_iodev_sqe *)arg0;
	const struct device *dev = (const struct device *)sqe->userdata;
	struct bmp581_data *data = dev->data;
	uint8_t *buf;
	uint32_t buf_len;
	struct rtio_cqe *cqe;
	int err = 0;

	do {
		cqe = rtio_cqe_consume(ctx);
		if (cqe == NULL) {
			continue;
		}

		if (err == 0) {
			err = cqe->result;
		}
		rtio_cqe_release(ctx, cqe);
	} while (cqe != NULL);

	if (err != 0) {
		goto bmp581_stream_evt_finish;
	}

	err = rtio_sqe_rx_buf(iodev_sqe, 0, 0, &buf, &buf_len);

	CHECKIF(err != 0 || !buf || buf_len < sizeof(struct bmp581_encoded_data)) {
		LOG_ERR("Couldn't get encoded buffer on completion");
		err = -EIO;
		goto bmp581_stream_evt_finish;
	}

	err = bmp581_encode(dev, (struct sensor_read_config *)iodev_sqe->sqe.iodev->data,
			    0x01, buf);
	if (err != 0) {
		LOG_ERR("Failed to encode frame: %d", err);
		goto bmp581_stream_evt_finish;
	}

bmp581_stream_evt_finish:
	atomic_set(&data->stream.state, BMP581_STREAM_ON);

	if (err < 0) {
		rtio_iodev_sqe_err(iodev_sqe, err);
	} else {
		rtio_iodev_sqe_ok(iodev_sqe, 0);
	}
}

static void bmp581_event_handler(const struct device *dev)
{
	struct bmp581_data *data = dev->data;
	const struct bmp581_config *cfg = dev->config;
	struct rtio_iodev_sqe *iodev_sqe = data->stream.iodev_sqe;
	uint8_t *buf = NULL;
	uint32_t buf_len = 0;
	int err;

	CHECKIF(!data->stream.iodev_sqe || FIELD_GET(RTIO_SQE_CANCELED, iodev_sqe->sqe.flags)) {

		uint8_t val = 0;

		LOG_WRN("Callback triggered with no streaming submission - Disabling interrupts");

		(void)gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);

		err = bmp581_prep_reg_write_rtio_async(&cfg->bus, BMP5_REG_INT_SOURCE, &val, 1,
						       NULL);
		if (err >= 0) {
			rtio_submit(cfg->bus.rtio.ctx, 0);
		}

		(void)atomic_set(&data->stream.state, BMP581_STREAM_OFF);

		return;
	}

	CHECKIF(atomic_cas(&data->stream.state, BMP581_STREAM_ON, BMP581_STREAM_BUSY) == false) {
		LOG_WRN("Callback triggered while stream is busy. Ignoring request");
		return;
	}

	err = rtio_sqe_rx_buf(iodev_sqe,
			      sizeof(struct bmp581_encoded_data),
			      sizeof(struct bmp581_encoded_data),
			      &buf, &buf_len);
	CHECKIF(err != 0 || buf_len < sizeof(struct bmp581_encoded_data)) {
		LOG_ERR("Failed to allocate BMP581 encoded buffer: %d", err);
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	struct bmp581_encoded_data *edata = (struct bmp581_encoded_data *)buf;
	struct rtio_sqe *read_sqe = NULL;
	struct rtio_sqe *cb_sqe;

	err = bmp581_prep_reg_read_rtio_async(&cfg->bus, BMP5_REG_TEMP_DATA_XLSB,
					      edata->payload, sizeof(edata->payload),
					      &read_sqe);
	CHECKIF(err < 0 || !read_sqe) {
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}
	read_sqe->flags |= RTIO_SQE_CHAINED;

	cb_sqe = rtio_sqe_acquire(cfg->bus.rtio.ctx);
	if (cb_sqe == NULL) {
		LOG_ERR("Failed to acquire callback SQE");
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	rtio_sqe_prep_callback_no_cqe(cb_sqe, bmp581_stream_event_complete,
					iodev_sqe, (void *)dev);

	rtio_submit(cfg->bus.rtio.ctx, 0);
}

static void bmp581_gpio_callback(const struct device *port, struct gpio_callback *cb, uint32_t pin)
{
	struct bmp581_stream *stream = CONTAINER_OF(cb, struct bmp581_stream, cb);
	const struct device *dev = stream->dev;

	bmp581_event_handler(dev);
}

void bmp581_stream_submit(const struct device *dev,
			  struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *read_config = iodev_sqe->sqe.iodev->data;
	struct bmp581_data *data = dev->data;
	const struct bmp581_config *cfg = dev->config;
	int err;

	if ((read_config->count != 1) ||
		(read_config->triggers[0].trigger != SENSOR_TRIG_DATA_READY)) {
		LOG_ERR("Only SENSOR_TRIG_DATA_READY is supported");
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
		return;
	}

	data->stream.iodev_sqe = iodev_sqe;

	if (atomic_cas(&data->stream.state, BMP581_STREAM_OFF, BMP581_STREAM_ON)) {
		/* Enable DRDY interrupt */
		struct rtio_sqe *int_src_sqe;
		uint8_t val = 0;

		val = BMP5_SET_BITSLICE(val, BMP5_INT_DRDY_EN, 1);

		err = bmp581_prep_reg_write_rtio_async(&cfg->bus, BMP5_REG_INT_SOURCE, &val, 1,
						       &int_src_sqe);
		if (err < 0) {
			rtio_iodev_sqe_err(iodev_sqe, err);
			return;
		}
		int_src_sqe->flags |= RTIO_SQE_CHAINED;

		val = BMP5_SET_BITSLICE(val, BMP5_INT_MODE, BMP5_INT_MODE_PULSED);
		val = BMP5_SET_BITSLICE(val, BMP5_INT_POL, BMP5_INT_POL_ACTIVE_HIGH);
		val = BMP5_SET_BITSLICE(val, BMP5_INT_OD, BMP5_INT_OD_PUSHPULL);
		val = BMP5_SET_BITSLICE(val, BMP5_INT_EN, 1);

		err = bmp581_prep_reg_write_rtio_async(&cfg->bus, BMP5_REG_INT_CONFIG, &val, 1,
						       NULL);
		if (err < 0) {
			rtio_iodev_sqe_err(iodev_sqe, err);
			return;
		}

		(void)rtio_submit(cfg->bus.rtio.ctx, 0);

		err = gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		if (err < 0) {
			rtio_iodev_sqe_err(iodev_sqe, err);
		}
	}
}

int bmp581_stream_init(const struct device *dev)
{
	struct bmp581_data *data = dev->data;
	const struct bmp581_config *cfg = dev->config;
	int err;

	data->stream.dev = dev;
	atomic_set(&data->stream.state, BMP581_STREAM_OFF);

	if (!device_is_ready(cfg->int_gpio.port)) {
		LOG_ERR("DRDY GPIO device is not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT);
	if (err < 0) {
		return err;
	}

	gpio_init_callback(&data->stream.cb, bmp581_gpio_callback, BIT(cfg->int_gpio.pin));

	err = gpio_add_callback(cfg->int_gpio.port, &data->stream.cb);
	if (err < 0) {
		return err;
	}

	err = gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);
	if (err < 0) {
		return err;
	}

	return 0;
}
