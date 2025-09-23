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

static inline void bmp581_stream_result(const struct device *dev, int err)
{
	struct bmp581_data *data = dev->data;
	struct rtio_iodev_sqe *iodev_sqe = data->stream.iodev_sqe;

	data->stream.iodev_sqe = NULL;
	if (err < 0) {
		rtio_iodev_sqe_err(iodev_sqe, err);
	} else {
		rtio_iodev_sqe_ok(iodev_sqe, 0);
	}
}

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

	err = bmp581_encode(dev, (const struct sensor_read_config *)iodev_sqe->sqe.iodev->data,
			    data->stream.enabled_mask, buf);
	if (err != 0) {
		LOG_ERR("Failed to encode frame: %d", err);
		goto bmp581_stream_evt_finish;
	}

bmp581_stream_evt_finish:
	atomic_set(&data->stream.state, BMP581_STREAM_ON);
	bmp581_stream_result(dev, err);
}

static void bmp581_event_handler(const struct device *dev)
{
	struct bmp581_data *data = dev->data;
	const struct bmp581_config *cfg = dev->config;
	struct rtio_iodev_sqe *iodev_sqe = data->stream.iodev_sqe;
	struct rtio_sqe *cb_sqe;
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
			(void)rtio_submit(cfg->bus.rtio.ctx, 0);
		}

		(void)atomic_set(&data->stream.state, BMP581_STREAM_OFF);

		return;
	}

	CHECKIF(atomic_cas(&data->stream.state, BMP581_STREAM_ON, BMP581_STREAM_BUSY) == false) {
		LOG_WRN("Callback triggered while stream is busy. Ignoring request");
		return;
	}

	if ((data->stream.enabled_mask & BMP581_EVENT_DRDY) != 0) {

		err = rtio_sqe_rx_buf(iodev_sqe,
				      sizeof(struct bmp581_encoded_data),
				      sizeof(struct bmp581_encoded_data),
				      &buf, &buf_len);
		CHECKIF(err != 0 || buf_len < sizeof(struct bmp581_encoded_data)) {
			LOG_ERR("Failed to allocate BMP581 encoded buffer: %d", err);
			bmp581_stream_result(dev, -ENOMEM);
			return;
		}

		struct bmp581_encoded_data *edata = (struct bmp581_encoded_data *)buf;
		struct rtio_sqe *read_sqe = NULL;

		err = bmp581_prep_reg_read_rtio_async(&cfg->bus, BMP5_REG_TEMP_DATA_XLSB,
						      edata->payload, sizeof(edata->payload),
						      &read_sqe);
		CHECKIF(err < 0 || !read_sqe) {
			bmp581_stream_result(dev, err);
			return;
		}
		read_sqe->flags |= RTIO_SQE_CHAINED;

	} else if ((data->stream.enabled_mask & BMP581_EVENT_FIFO_WM) != 0) {

		size_t len_data = data->stream.fifo_thres * sizeof(struct bmp581_frame);

		size_t len_required = sizeof(struct bmp581_encoded_data) + len_data;

		err = rtio_sqe_rx_buf(iodev_sqe, len_required, len_required, &buf, &buf_len);

		CHECKIF(err != 0 || (buf_len < len_required)) {
			LOG_ERR("Failed to allocate BMP581 encoded buffer: %d", err);
			bmp581_stream_result(dev, -ENOMEM);
			return;
		}

		struct bmp581_encoded_data *edata = (struct bmp581_encoded_data *)buf;
		struct rtio_sqe *read_sqe = NULL;

		err = bmp581_prep_reg_read_rtio_async(&cfg->bus, BMP5_REG_FIFO_DATA,
						      (uint8_t *)edata->frame, len_data,
						      &read_sqe);
		CHECKIF(err < 0 || !read_sqe) {
			bmp581_stream_result(dev, err);
			return;
		}
		read_sqe->flags |= RTIO_SQE_CHAINED;

	} else {

		uint8_t val = 0;

		LOG_ERR("Callback triggered with invalid streaming-config. Disabling interrupts");

		(void)gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);

		err = bmp581_prep_reg_write_rtio_async(&cfg->bus, BMP5_REG_INT_SOURCE, &val, 1,
						       NULL);
		if (err >= 0) {
			(void)rtio_submit(cfg->bus.rtio.ctx, 0);
		}

		(void)atomic_set(&data->stream.state, BMP581_STREAM_OFF);

		return;
	}

	cb_sqe = rtio_sqe_acquire(cfg->bus.rtio.ctx);
	if (cb_sqe == NULL) {
		LOG_ERR("Failed to acquire callback SQE");
		bmp581_stream_result(dev, -ENOMEM);
		return;
	}

	rtio_sqe_prep_callback_no_cqe(cb_sqe, bmp581_stream_event_complete,
				      iodev_sqe, (void *)dev);

	(void)rtio_submit(cfg->bus.rtio.ctx, 0);
}

static void bmp581_gpio_callback(const struct device *port, struct gpio_callback *cb, uint32_t pin)
{
	struct bmp581_stream *stream = CONTAINER_OF(cb, struct bmp581_stream, cb);
	const struct device *dev = stream->dev;

	bmp581_event_handler(dev);
}

static inline int bmp581_stream_prep_fifo_wm_async(const struct device *dev)
{
	uint8_t val;
	struct rtio_sqe *out_sqe;
	const struct bmp581_config *cfg = dev->config;
	struct bmp581_data *data = dev->data;
	int err;

	val = BMP5_SET_BITSLICE(0, BMP5_ODR, data->osr_odr_press_config.odr);
	val = BMP5_SET_BITSLICE(val, BMP5_POWERMODE, 0);

	err = bmp581_prep_reg_write_rtio_async(&cfg->bus, BMP5_REG_ODR_CONFIG,
					       &val, 1, &out_sqe);
	if (err < 0) {
		return err;
	}
	out_sqe->flags |= RTIO_SQE_CHAINED;

	out_sqe = rtio_sqe_acquire(cfg->bus.rtio.ctx);
	if (!out_sqe) {
		rtio_sqe_drop_all(cfg->bus.rtio.ctx);
		return err;
	}
	/* Wait until standby mode is effective before proceeding writes */
	rtio_sqe_prep_delay(out_sqe, K_MSEC(5), NULL);
	out_sqe->flags |= RTIO_SQE_CHAINED;

	val = BMP5_SET_BITSLICE(0, BMP5_FIFO_COUNT, data->stream.fifo_thres);

	err = bmp581_prep_reg_write_rtio_async(&cfg->bus, BMP5_REG_FIFO_CONFIG,
					       &val, 1,
					       &out_sqe);
	if (err < 0) {
		return err;
	}
	out_sqe->flags |= RTIO_SQE_CHAINED;

	val = BMP5_SET_BITSLICE(0, BMP5_FIFO_FRAME_SEL, BMP5_FIFO_FRAME_SEL_ALL);

	err = bmp581_prep_reg_write_rtio_async(&cfg->bus, BMP5_REG_FIFO_SEL,
					       &val, 1,
					       &out_sqe);
	if (err < 0) {
		return err;
	}
	out_sqe->flags |= RTIO_SQE_CHAINED;

	val = BMP5_SET_BITSLICE(
		0, BMP5_ODR, data->osr_odr_press_config.odr);
	val = BMP5_SET_BITSLICE(
		val, BMP5_POWERMODE, data->osr_odr_press_config.power_mode);

	err = bmp581_prep_reg_write_rtio_async(&cfg->bus, BMP5_REG_ODR_CONFIG,
					       &val, 1, &out_sqe);
	if (err < 0) {
		return err;
	}
	out_sqe->flags |= RTIO_SQE_CHAINED;

	return 0;
}

void bmp581_stream_submit(const struct device *dev,
			  struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *read_config = iodev_sqe->sqe.iodev->data;
	struct bmp581_data *data = dev->data;
	const struct bmp581_config *cfg = dev->config;
	int err;

	enum bmp581_event enabled_mask = bmp581_encode_events_bitmask(read_config->triggers,
								      read_config->count);

	if (enabled_mask == 0) {
		LOG_ERR("Invalid triggers configured!");
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
		return;
	}
	if ((enabled_mask & BMP581_EVENT_DRDY) != 0 && (enabled_mask & BMP581_EVENT_FIFO_WM) != 0) {
		LOG_ERR("Invalid triggers: DRDY and FIFO shouldn't be enabled at the same time");
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
		return;
	}
	if ((enabled_mask & BMP581_EVENT_FIFO_WM) != 0 && data->stream.fifo_thres == 0) {
		LOG_ERR("Can't enable FIFO_WM because FIFO watermark is not configured");
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
		return;
	}

	data->stream.iodev_sqe = iodev_sqe;

	if (atomic_cas(&data->stream.state, BMP581_STREAM_OFF, BMP581_STREAM_ON) ||
	    data->stream.enabled_mask != enabled_mask) {
		struct rtio_sqe *int_src_sqe;
		uint8_t val = 0;

		(void)atomic_set(&data->stream.state, BMP581_STREAM_ON);
		data->stream.enabled_mask = enabled_mask;

		if ((enabled_mask & BMP581_EVENT_FIFO_WM) != 0) {
			err = bmp581_stream_prep_fifo_wm_async(dev);
			if (err < 0) {
				bmp581_stream_result(dev, err);
				return;
			}
		}

		val = BMP5_SET_BITSLICE(
			0, BMP5_INT_DRDY_EN, (enabled_mask & BMP581_EVENT_DRDY) ? 1 : 0);
		val = BMP5_SET_BITSLICE(
			val, BMP5_INT_FIFO_THRES_EN, (enabled_mask & BMP581_EVENT_FIFO_WM) ? 1 : 0);

		err = bmp581_prep_reg_write_rtio_async(&cfg->bus, BMP5_REG_INT_SOURCE, &val, 1,
						       &int_src_sqe);
		if (err < 0) {
			bmp581_stream_result(dev, err);
			return;
		}
		int_src_sqe->flags |= RTIO_SQE_CHAINED;

		val = BMP5_SET_BITSLICE(0, BMP5_INT_MODE, BMP5_INT_MODE_PULSED);
		val = BMP5_SET_BITSLICE(val, BMP5_INT_POL, BMP5_INT_POL_ACTIVE_HIGH);
		val = BMP5_SET_BITSLICE(val, BMP5_INT_OD, BMP5_INT_OD_PUSHPULL);
		val = BMP5_SET_BITSLICE(val, BMP5_INT_EN, 1);

		err = bmp581_prep_reg_write_rtio_async(&cfg->bus, BMP5_REG_INT_CONFIG, &val, 1,
						       NULL);
		if (err < 0) {
			bmp581_stream_result(dev, err);
			return;
		}

		(void)rtio_submit(cfg->bus.rtio.ctx, 0);

		err = gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		if (err < 0) {
			bmp581_stream_result(dev, err);
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
