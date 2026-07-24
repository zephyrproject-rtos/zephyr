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

static gpio_flags_t bmp581_stream_int_gpio_flags(const struct bmp581_config *cfg)
{
	ARG_UNUSED(cfg);

	/* Latched BMP581 INT stays active until INT_STATUS is read; edge + BUSY is enough. */
	return GPIO_INT_EDGE_TO_ACTIVE;
}

static int bmp581_stream_clear_latched_int(const struct device *dev)
{
	const struct bmp581_config *cfg = dev->config;
	struct bmp581_data *data = dev->data;

	if (!cfg->int_latched) {
		return 0;
	}

	return bmp581_reg_read_rtio(&cfg->bus, BMP5_REG_INT_STATUS,
				    &data->stream.int_status_scratch, 1);
}

static int bmp581_stream_bus_wait(const struct device *dev)
{
	const struct bmp581_config *cfg = dev->config;
	struct rtio *ctx = cfg->bus.rtio.ctx;
	struct rtio_cqe *cqe;
	int ret = 0;
	bool blocked = false;

	for (;;) {
		if (!blocked) {
			cqe = rtio_cqe_consume_block(ctx);
			blocked = true;
		} else {
			cqe = rtio_cqe_consume(ctx);
		}

		if (cqe == NULL) {
			break;
		}

		if (ret == 0) {
			ret = cqe->result;
		}
		rtio_cqe_release(ctx, cqe);
	}

	return ret;
}

static inline void bmp581_stream_result(const struct device *dev, int err)
{
	struct bmp581_data *data = dev->data;
	struct rtio_iodev_sqe *iodev_sqe = data->stream.iodev_sqe;

	if (err < 0) {
		data->stream.iodev_sqe = NULL;
		rtio_iodev_sqe_err(iodev_sqe, err);
	} else {
		rtio_iodev_sqe_ok(iodev_sqe, 0);
	}
}

static void bmp581_stream_latched_int_recover_work(struct k_work *work)
{
	struct bmp581_stream *stream =
		CONTAINER_OF(work, struct bmp581_stream, latched_int_recover_work);
	const struct device *dev = stream->dev;
	const struct bmp581_config *cfg = dev->config;
	struct bmp581_data *data = dev->data;
	int ret;

	if (!cfg->int_latched) {
		return;
	}

	if (atomic_get(&data->stream.state) == BMP581_STREAM_BUSY) {
		(void)k_work_submit(&data->stream.latched_int_recover_work);
		return;
	}

	if (atomic_get(&data->stream.state) != BMP581_STREAM_ON) {
		return;
	}

	ret = bmp581_reg_read_rtio(&cfg->bus, BMP5_REG_INT_STATUS, &stream->int_status_scratch, 1);
	if (ret < 0) {
		LOG_ERR("Latched INT recover: INT_STATUS read failed: %d", ret);
	}
}

static void bmp581_stream_schedule_latched_int_recover(const struct device *dev)
{
	const struct bmp581_config *cfg = dev->config;
	struct bmp581_data *data = dev->data;

	if (!cfg->int_latched) {
		return;
	}

	(void)k_work_submit(&data->stream.latched_int_recover_work);
}

static void bmp581_stream_handler_abort(const struct device *dev, int err)
{
	const struct bmp581_config *cfg = dev->config;
	struct bmp581_data *data = dev->data;

	rtio_sqe_drop_all(cfg->bus.rtio.ctx);
	atomic_set(&data->stream.state, BMP581_STREAM_ON);
	bmp581_stream_schedule_latched_int_recover(dev);
	bmp581_stream_result(dev, err);
}

static void bmp581_stream_event_complete(struct rtio *ctx, const struct rtio_sqe *sqe, int result,
					 void *arg0)
{
	ARG_UNUSED(result);

	struct rtio_iodev_sqe *iodev_sqe = (struct rtio_iodev_sqe *)arg0;
	const struct device *dev = (const struct device *)sqe->userdata;
	const struct bmp581_config *cfg = dev->config;
	struct bmp581_data *data = dev->data;
	uint8_t *buf;
	uint32_t buf_len;
	struct rtio_cqe *cqe;
	int rtio_err = 0;
	int err = 0;

	do {
		cqe = rtio_cqe_consume(ctx);
		if (cqe == NULL) {
			continue;
		}

		if (rtio_err == 0) {
			rtio_err = cqe->result;
		}
		rtio_cqe_release(ctx, cqe);
	} while (cqe != NULL);

	if (rtio_err != 0) {
		err = rtio_err;
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
	if (cfg->int_latched && rtio_err != 0) {
		bmp581_stream_schedule_latched_int_recover(dev);
	}
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

	if (!data->stream.iodev_sqe) {
		/* Multishot resubmit restores iodev_sqe; ignore a stale DRDY edge. */
		LOG_DBG("GPIO callback with no active streaming submission; ignoring");
		return;
	}

	if (FIELD_GET(RTIO_SQE_CANCELED, iodev_sqe->sqe.flags)) {
		LOG_DBG("DRDY after stream cancel; ignoring");
		return;
	}

	CHECKIF(atomic_cas(&data->stream.state, BMP581_STREAM_ON, BMP581_STREAM_BUSY) == false) {
		LOG_WRN("Callback triggered while stream is busy. Ignoring request");
		return;
	}

	if ((data->stream.enabled_mask & BMP581_EVENT_DRDY) != 0) {

		err = rtio_sqe_rx_buf(iodev_sqe, sizeof(struct bmp581_encoded_data),
				      sizeof(struct bmp581_encoded_data), &buf, &buf_len);
		CHECKIF(err != 0 || buf_len < sizeof(struct bmp581_encoded_data)) {
			LOG_ERR("Failed to allocate BMP581 encoded buffer: %d", err);
			bmp581_stream_handler_abort(dev, err != 0 ? err : -ENOMEM);
			return;
		}

		struct bmp581_encoded_data *edata = (struct bmp581_encoded_data *)buf;
		struct rtio_sqe *read_sqe = NULL;

		data->stream.i2c_reg_temp = BMP5_REG_TEMP_DATA_XLSB;
		err = bmp581_prep_reg_read_rtio_async(&cfg->bus, &data->stream.i2c_reg_temp,
						      edata->payload, sizeof(edata->payload),
						      &read_sqe);
		CHECKIF(err < 0 || !read_sqe) {
			bmp581_stream_handler_abort(dev, err);
			return;
		}
		read_sqe->flags |= RTIO_SQE_CHAINED;

		if (cfg->int_latched) {
			struct rtio_sqe *status_read_sqe = NULL;

			data->stream.i2c_reg_int_status = BMP5_REG_INT_STATUS;
			err = bmp581_prep_reg_read_rtio_async(
				&cfg->bus, &data->stream.i2c_reg_int_status,
				&data->stream.int_status_scratch, 1, &status_read_sqe);
			CHECKIF(err < 0 || !status_read_sqe) {
				LOG_ERR("Failed to chain INT_STATUS read: %d", err);
				bmp581_stream_handler_abort(dev, err < 0 ? err : -ENOMEM);
				return;
			}
			status_read_sqe->flags |= RTIO_SQE_CHAINED;
		}

	} else if ((data->stream.enabled_mask & BMP581_EVENT_FIFO_WM) != 0) {

		size_t len_data = data->stream.fifo_thres * sizeof(struct bmp581_frame);

		size_t len_required = sizeof(struct bmp581_encoded_data) + len_data;

		err = rtio_sqe_rx_buf(iodev_sqe, len_required, len_required, &buf, &buf_len);

		CHECKIF(err != 0 || (buf_len < len_required)) {
			LOG_ERR("Failed to allocate BMP581 encoded buffer: %d", err);
			bmp581_stream_handler_abort(dev, err != 0 ? err : -ENOMEM);
			return;
		}

		struct bmp581_encoded_data *edata = (struct bmp581_encoded_data *)buf;
		struct rtio_sqe *read_sqe = NULL;

		data->stream.i2c_reg_fifo = BMP5_REG_FIFO_DATA;
		err = bmp581_prep_reg_read_rtio_async(&cfg->bus, &data->stream.i2c_reg_fifo,
						      (uint8_t *)edata->frame, len_data, &read_sqe);
		CHECKIF(err < 0 || !read_sqe) {
			bmp581_stream_handler_abort(dev, err);
			return;
		}
		read_sqe->flags |= RTIO_SQE_CHAINED;

		if (cfg->int_latched) {
			struct rtio_sqe *status_read_sqe = NULL;

			data->stream.i2c_reg_int_status = BMP5_REG_INT_STATUS;
			err = bmp581_prep_reg_read_rtio_async(
				&cfg->bus, &data->stream.i2c_reg_int_status,
				&data->stream.int_status_scratch, 1, &status_read_sqe);
			CHECKIF(err < 0 || !status_read_sqe) {
				LOG_ERR("Failed to chain INT_STATUS read: %d", err);
				bmp581_stream_handler_abort(dev, err < 0 ? err : -ENOMEM);
				return;
			}
			status_read_sqe->flags |= RTIO_SQE_CHAINED;
		}

	} else {
		LOG_ERR("Callback triggered with invalid streaming-config. Disabling interrupts");
		goto stream_stop_int;
	}

	cb_sqe = rtio_sqe_acquire(cfg->bus.rtio.ctx);
	if (cb_sqe == NULL) {
		LOG_ERR("Failed to acquire callback SQE");
		bmp581_stream_handler_abort(dev, -ENOMEM);
		return;
	}

	rtio_sqe_prep_callback_no_cqe(cb_sqe, bmp581_stream_event_complete, iodev_sqe, (void *)dev);

	(void)rtio_submit(cfg->bus.rtio.ctx, 0);
	return;

stream_stop_int:
	(void)gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);

	data->stream.wr_int_source_reg = BMP5_REG_INT_SOURCE;
	data->stream.wr_int_source_data = 0;
	err = bmp581_prep_reg_write_rtio_async(&cfg->bus, &data->stream.wr_int_source_reg,
					       &data->stream.wr_int_source_data, 1, NULL);
	if (err >= 0) {
		(void)rtio_submit(cfg->bus.rtio.ctx, 0);
	}

	(void)atomic_set(&data->stream.state, BMP581_STREAM_OFF);
}

static void bmp581_gpio_callback(const struct device *port, struct gpio_callback *cb, uint32_t pin)
{
	struct bmp581_stream *stream = CONTAINER_OF(cb, struct bmp581_stream, cb);
	const struct device *dev = stream->dev;

	bmp581_event_handler(dev);
}

static inline int bmp581_stream_prep_fifo_wm_async(const struct device *dev)
{
	struct rtio_sqe *out_sqe;
	const struct bmp581_config *cfg = dev->config;
	struct bmp581_data *data = dev->data;
	int err;

	data->stream.wr_wm_odr_reg = BMP5_REG_ODR_CONFIG;
	data->stream.wr_wm_odr_data_a =
		BMP5_SET_BITSLICE(0, BMP5_ODR, data->osr_odr_press_config.odr);
	data->stream.wr_wm_odr_data_a =
		BMP5_SET_BITSLICE(data->stream.wr_wm_odr_data_a, BMP5_POWERMODE, 0);

	err = bmp581_prep_reg_write_rtio_async(&cfg->bus, &data->stream.wr_wm_odr_reg,
					       &data->stream.wr_wm_odr_data_a, 1, &out_sqe);
	if (err < 0) {
		return err;
	}
	out_sqe->flags |= RTIO_SQE_CHAINED;

	out_sqe = rtio_sqe_acquire(cfg->bus.rtio.ctx);
	if (!out_sqe) {
		rtio_sqe_drop_all(cfg->bus.rtio.ctx);
		return err;
	}
	/* STANDBY effective before FIFO setup (t_standby typ. 2.5 ms; use 5 ms). */
	rtio_sqe_prep_delay(out_sqe, K_MSEC(5), NULL);
	out_sqe->flags |= RTIO_SQE_CHAINED;

	data->stream.wr_wm_fifo_cfg_reg = BMP5_REG_FIFO_CONFIG;
	data->stream.wr_wm_fifo_cfg_data =
		BMP5_SET_BITSLICE(0, BMP5_FIFO_COUNT, data->stream.fifo_thres);

	err = bmp581_prep_reg_write_rtio_async(&cfg->bus, &data->stream.wr_wm_fifo_cfg_reg,
					       &data->stream.wr_wm_fifo_cfg_data, 1, &out_sqe);
	if (err < 0) {
		return err;
	}
	out_sqe->flags |= RTIO_SQE_CHAINED;

	data->stream.wr_wm_fifo_sel_reg = BMP5_REG_FIFO_SEL;
	data->stream.wr_wm_fifo_sel_data =
		BMP5_SET_BITSLICE(0, BMP5_FIFO_FRAME_SEL, BMP5_FIFO_FRAME_SEL_ALL);

	err = bmp581_prep_reg_write_rtio_async(&cfg->bus, &data->stream.wr_wm_fifo_sel_reg,
					       &data->stream.wr_wm_fifo_sel_data, 1, &out_sqe);
	if (err < 0) {
		return err;
	}
	out_sqe->flags |= RTIO_SQE_CHAINED;

	data->stream.wr_wm_odr_reg = BMP5_REG_ODR_CONFIG;
	data->stream.wr_wm_odr_data_b =
		BMP5_SET_BITSLICE(0, BMP5_ODR, data->osr_odr_press_config.odr);
	data->stream.wr_wm_odr_data_b =
		BMP5_SET_BITSLICE(data->stream.wr_wm_odr_data_b, BMP5_POWERMODE,
				  data->osr_odr_press_config.power_mode);

	err = bmp581_prep_reg_write_rtio_async(&cfg->bus, &data->stream.wr_wm_odr_reg,
					       &data->stream.wr_wm_odr_data_b, 1, &out_sqe);
	if (err < 0) {
		return err;
	}
	out_sqe->flags |= RTIO_SQE_CHAINED;

	return 0;
}

void bmp581_stream_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *read_config = iodev_sqe->sqe.iodev->data;
	struct bmp581_data *data = dev->data;
	const struct bmp581_config *cfg = dev->config;
	int err;

	enum bmp581_event enabled_mask =
		bmp581_encode_events_bitmask(read_config->triggers, read_config->count);

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
		(void)atomic_set(&data->stream.state, BMP581_STREAM_ON);
		data->stream.enabled_mask = enabled_mask;

		data->stream.wr_int_source_reg = BMP5_REG_INT_SOURCE;
		data->stream.wr_int_source_data = BMP5_SET_BITSLICE(
			0, BMP5_INT_DRDY_EN, (enabled_mask & BMP581_EVENT_DRDY) ? 1 : 0);
		data->stream.wr_int_source_data =
			BMP5_SET_BITSLICE(data->stream.wr_int_source_data, BMP5_INT_FIFO_THRES_EN,
					  (enabled_mask & BMP581_EVENT_FIFO_WM) ? 1 : 0);

		data->stream.wr_int_config_reg = BMP5_REG_INT_CONFIG;
		data->stream.wr_int_config_data = BMP5_SET_BITSLICE(
			0, BMP5_INT_MODE,
			cfg->int_latched ? BMP5_INT_MODE_LATCHED : BMP5_INT_MODE_PULSED);
		data->stream.wr_int_config_data = BMP5_SET_BITSLICE(
			data->stream.wr_int_config_data, BMP5_INT_POL, cfg->int_polarity);
		data->stream.wr_int_config_data = BMP5_SET_BITSLICE(
			data->stream.wr_int_config_data, BMP5_INT_OD, cfg->int_open_drain);
		data->stream.wr_int_config_data =
			BMP5_SET_BITSLICE(data->stream.wr_int_config_data, BMP5_INT_EN, 1);

		if ((enabled_mask & BMP581_EVENT_FIFO_WM) != 0) {
			struct rtio_sqe *int_src_sqe;

			err = bmp581_stream_prep_fifo_wm_async(dev);
			if (err < 0) {
				bmp581_stream_result(dev, err);
				return;
			}

			err = bmp581_prep_reg_write_rtio_async(
				&cfg->bus, &data->stream.wr_int_source_reg,
				&data->stream.wr_int_source_data, 1, &int_src_sqe);
			if (err < 0) {
				bmp581_stream_result(dev, err);
				return;
			}
			int_src_sqe->flags |= RTIO_SQE_CHAINED;

			err = bmp581_prep_reg_write_rtio_async(
				&cfg->bus, &data->stream.wr_int_config_reg,
				&data->stream.wr_int_config_data, 1, NULL);
			if (err < 0) {
				bmp581_stream_result(dev, err);
				return;
			}

			(void)rtio_submit(cfg->bus.rtio.ctx, 0);
			err = bmp581_stream_bus_wait(dev);
		} else {
			err = bmp581_reg_write_rtio(&cfg->bus, data->stream.wr_int_source_reg,
						    &data->stream.wr_int_source_data, 1);
			if (err == 0) {
				err = bmp581_reg_write_rtio(&cfg->bus,
							    data->stream.wr_int_config_reg,
							    &data->stream.wr_int_config_data, 1);
			}
		}

		if (err < 0) {
			bmp581_stream_result(dev, err);
			return;
		}

		err = bmp581_stream_clear_latched_int(dev);
		if (err < 0) {
			bmp581_stream_result(dev, err);
			return;
		}

		err = gpio_pin_interrupt_configure_dt(&cfg->int_gpio,
						      bmp581_stream_int_gpio_flags(cfg));
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
	k_work_init(&data->stream.latched_int_recover_work, bmp581_stream_latched_int_recover_work);

	if (!device_is_ready(cfg->int_gpio.port)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->int_gpio.port);
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
