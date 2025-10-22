/*
 * Copyright (c) 2023 Google LLC
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/check.h>

#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(invensense_icm45686, i3c)
#include <zephyr/drivers/i3c.h>
#endif

#include "icm45686.h"
#include "icm45686_bus.h"
#include "icm45686_reg.h"
#include "icm45686_stream.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ICM45686_STREAM, CONFIG_SENSOR_LOG_LEVEL);

enum icm45686_stream_state {
	ICM45686_STREAM_OFF = 0,
	ICM45686_STREAM_ON = 1,
	ICM45686_STREAM_BUSY = 2,
};

static struct sensor_stream_trigger *get_read_config_trigger(const struct sensor_read_config *cfg,
							     enum sensor_trigger_type trig)
{
	for (int i = 0; i < cfg->count; ++i) {
		if (cfg->triggers[i].trigger == trig) {
			return &cfg->triggers[i];
		}
	}
	LOG_DBG("Unsupported trigger (%d)", trig);
	return NULL;
}

static inline bool should_flush_fifo(const struct sensor_read_config *read_cfg,
				     uint8_t int_status)
{
	struct sensor_stream_trigger *trig_fifo_ths = get_read_config_trigger(
		read_cfg,
		SENSOR_TRIG_FIFO_WATERMARK);

	bool fifo_ths = int_status & REG_INT1_STATUS0_FIFO_THS(true);
	bool fifo_full = int_status & REG_INT1_STATUS0_FIFO_FULL(true);

	if ((trig_fifo_ths && trig_fifo_ths->opt == SENSOR_STREAM_DATA_DROP && fifo_ths) ||
	    (fifo_full)) {
		return true;
	}

	return false;
}

static inline bool should_read_fifo(const struct sensor_read_config *read_cfg)
{
	struct sensor_stream_trigger *trig_fifo_ths = get_read_config_trigger(
		read_cfg,
		SENSOR_TRIG_FIFO_WATERMARK);
	struct sensor_stream_trigger *trig_fifo_full = get_read_config_trigger(
		read_cfg,
		SENSOR_TRIG_FIFO_FULL);

	if ((trig_fifo_ths && trig_fifo_ths->opt == SENSOR_STREAM_DATA_INCLUDE) ||
	    (trig_fifo_full && trig_fifo_full->opt == SENSOR_STREAM_DATA_INCLUDE)) {
		return true;
	}

	return false;
}

static inline bool should_read_all_fifo(const struct sensor_read_config *read_cfg)
{
	struct sensor_stream_trigger *trig_fifo_full = get_read_config_trigger(
		read_cfg,
		SENSOR_TRIG_FIFO_FULL);

	return (trig_fifo_full && trig_fifo_full->opt == SENSOR_STREAM_DATA_INCLUDE);
}

static inline bool should_read_data(const struct sensor_read_config *read_cfg)
{
	struct sensor_stream_trigger *trig_drdy = get_read_config_trigger(
		read_cfg,
		SENSOR_TRIG_DATA_READY);

	if (trig_drdy && trig_drdy->opt == SENSOR_STREAM_DATA_INCLUDE) {
		return true;
	}

	return false;
}

static inline void icm45686_stream_result(const struct device *dev,
					  int result)
{
	struct icm45686_data *data = dev->data;
	const struct icm45686_config *cfg = dev->config;
	struct rtio_iodev_sqe *iodev_sqe = data->stream.iodev_sqe;

	(void)atomic_set(&data->stream.state, ICM45686_STREAM_OFF);
	memset(&data->stream.data, 0, sizeof(data->stream.data));
	data->stream.iodev_sqe = NULL;

	if (result < 0) {
		/** Clear config-set so next submission re-configures the IMU */
		memset(&data->stream.settings, 0, sizeof(data->stream.settings));
		(void)gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);
		rtio_iodev_sqe_err(iodev_sqe, result);
	} else {
		rtio_iodev_sqe_ok(iodev_sqe, 0);
	}
}

static void icm45686_complete_handler(struct rtio *ctx,
				      const struct rtio_sqe *sqe, int result,
				      void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct icm45686_data *data = dev->data;
	const struct icm45686_config *cfg = dev->config;
	const struct sensor_read_config *read_cfg = data->stream.iodev_sqe->sqe.iodev->data;
	const bool wm_gt_ths = !cfg->settings.fifo_watermark_equals;
	uint8_t int_status = data->stream.data.int_status;
	int err;

	if (result < 0) {
		LOG_ERR("Data readout failed: %d", result);
		icm45686_stream_result(dev, result);
		return;
	}

	data->stream.data.events.drdy = int_status & REG_INT1_STATUS0_DRDY(true);
	data->stream.data.events.fifo_ths = int_status & REG_INT1_STATUS0_FIFO_THS(true);
	data->stream.data.events.fifo_full = int_status & REG_INT1_STATUS0_FIFO_FULL(true);

	struct icm45686_encoded_data *buf;
	uint32_t buf_len;

	err = rtio_sqe_rx_buf(data->stream.iodev_sqe, 0, 0, (uint8_t **)&buf, &buf_len);
	CHECKIF(err != 0 || buf_len == 0) {
		LOG_ERR("Failed to acquire buffer for encoded data: %d, len: %d", err, buf_len);
		icm45686_stream_result(dev, -ENOMEM);
		return;
	}

	buf->header.timestamp = data->stream.data.timestamp;
	buf->header.events = REG_INT1_STATUS0_DRDY(data->stream.data.events.drdy) |
			     REG_INT1_STATUS0_FIFO_THS(data->stream.data.events.fifo_ths) |
			     REG_INT1_STATUS0_FIFO_FULL(data->stream.data.events.fifo_full);

	if (should_flush_fifo(read_cfg, int_status)) {
		uint8_t write_reg = REG_FIFO_CONFIG2_FIFO_FLUSH(true) |
				    REG_FIFO_CONFIG2_FIFO_WM_GT_THS(wm_gt_ths);
		LOG_WRN("Flushing FIFO: %d", int_status);

		err = icm45686_prep_reg_write_rtio_async(&data->bus, REG_FIFO_CONFIG2, &write_reg,
							 1, NULL);
		if (err < 0) {
			LOG_ERR("Failed to acquire RTIO SQE");
			icm45686_stream_result(dev, -ENOMEM);
			return;
		}
		icm45686_stream_result(dev, 0);
		return;
	}

	struct rtio_cqe *cqe;

	err = 0;
	do {
		cqe = rtio_cqe_consume(ctx);
		if (cqe != NULL) {
			if (err == 0) {
				err = cqe->result;
			}
			rtio_cqe_release(ctx, cqe);
		}
	} while (cqe != NULL);

	icm45686_stream_result(dev, err);
}

static void icm45686_event_handler(const struct device *dev)
{
	struct icm45686_data *data = dev->data;
	const struct icm45686_config *cfg = dev->config;
	const struct sensor_read_config *read_cfg = data->stream.iodev_sqe->sqe.iodev->data;
	uint8_t val = 0;
	uint64_t cycles;
	int err;

	if (!data->stream.iodev_sqe ||
	    FIELD_GET(RTIO_SQE_CANCELED, data->stream.iodev_sqe->sqe.flags)) {
		LOG_WRN("Callback triggered with no streaming submission - Disabling interrupts");
		(void)atomic_set(&data->stream.state, ICM45686_STREAM_OFF);
		(void)gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);
		err = icm45686_prep_reg_write_rtio_async(&data->bus, REG_INT1_CONFIG0, &val, 1,
							 NULL);
		if (err < 0) {
			LOG_ERR("Failed to prepare write to disable interrupts: %d", err);
			rtio_sqe_drop_all(data->bus.rtio.ctx);
			data->stream.iodev_sqe = NULL;
			return;
		}
		rtio_submit(data->bus.rtio.ctx, 0);

		data->stream.settings.enabled.drdy = false;
		data->stream.settings.enabled.fifo_ths = false;
		data->stream.settings.enabled.fifo_full = false;
		return;
	}

	if (atomic_cas(&data->stream.state, ICM45686_STREAM_ON, ICM45686_STREAM_BUSY) == false) {
		LOG_WRN("Event handler triggered while a stream is in progress! Ignoring");
		return;
	}

	err = sensor_clock_get_cycles(&cycles);
	if (err) {
		LOG_ERR("Failed to get timestamp: %d", err);
		icm45686_stream_result(dev, err);
		return;
	}

	data->stream.data.timestamp = sensor_clock_cycles_to_ns(cycles);

	/** Prepare an asynchronous read of the INT status register */
	struct rtio_sqe *read_sqe;
	struct rtio_sqe *data_rd_sqe;

	/** Directly read Status Register to determine what triggered the event */
	err = icm45686_prep_reg_read_rtio_async(&data->bus, REG_INT1_STATUS0 | REG_READ_BIT,
						&data->stream.data.int_status, 1, &read_sqe);
	if (err < 0) {
		LOG_ERR("Failed to prepare Status-reg read: %d", err);
		icm45686_stream_result(dev, -ENOMEM);
		return;
	}
	read_sqe->flags |= RTIO_SQE_CHAINED;

	struct icm45686_encoded_data *buf;
	uint32_t buf_len;
	uint32_t buf_len_required = sizeof(struct icm45686_encoded_header);

	/** We just need the header to communicate the events occurred during
	 * this SQE. Only include more data if the associated trigger needs it.
	 */
	if (should_read_fifo(read_cfg)) {
		size_t num_frames_to_read = should_read_all_fifo(read_cfg) ?
					    FIFO_COUNT_MAX_HIGH_RES : cfg->settings.fifo_watermark;

		buf_len_required += (num_frames_to_read *
				     sizeof(struct icm45686_encoded_fifo_payload));
	} else if (should_read_data(read_cfg)) {
		buf_len_required += sizeof(struct icm45686_encoded_payload);
	} else {
		/** No need additional space as probably we'll want to
		 * flush the data or just report the event.
		 */
	}
	err = rtio_sqe_rx_buf(data->stream.iodev_sqe, buf_len_required, buf_len_required,
			      (uint8_t **)&buf, &buf_len);
	CHECKIF(err != 0) {
		LOG_ERR("Failed to acquire buffer (len: %d) for encoded data: %d. Please revisit"
			" RTIO queue sizing and look for bottlenecks during sensor data processing",
			buf_len_required, err);
		rtio_sqe_drop_all(data->bus.rtio.ctx);
		icm45686_stream_result(dev, -ENOMEM);
		return;
	}

	if (should_read_fifo(read_cfg)) {
		/** In FIFO data, the scale is fixed, irrespective of
		 * the configured settings.
		 */
		buf->header.accel_fs = ICM45686_DT_ACCEL_FS_32;
		buf->header.gyro_fs = ICM45686_DT_GYRO_FS_4000;
		buf->header.channels = 0x7F; /* Signal all channels are available */
		buf->header.fifo_count = should_read_all_fifo(read_cfg) ? FIFO_COUNT_MAX_HIGH_RES :
					 cfg->settings.fifo_watermark;

		err = icm45686_prep_reg_read_rtio_async(
			&data->bus, REG_FIFO_DATA | REG_READ_BIT, (uint8_t *)&buf->fifo_payload,
			buf->header.fifo_count * sizeof(struct icm45686_encoded_fifo_payload),
			&data_rd_sqe);
		if (err < 0) {
			LOG_ERR("Failed to acquire RTIO SQEs");
			icm45686_stream_result(dev, -ENOMEM);
			return;
		}
		data_rd_sqe->flags |= RTIO_SQE_CHAINED;
	} else if (should_read_data(read_cfg)) {
		buf->header.accel_fs = data->edata.header.accel_fs;
		buf->header.gyro_fs = data->edata.header.gyro_fs;
		buf->header.channels = 0x7F; /* Signal all channels are available */

		err = icm45686_prep_reg_read_rtio_async(&data->bus,
							REG_ACCEL_DATA_X1_UI | REG_READ_BIT,
							buf->payload.buf, sizeof(buf->payload.buf),
							&data_rd_sqe);
		if (err < 0) {
			LOG_ERR("Failed to acquire RTIO SQEs");
			icm45686_stream_result(dev, -ENOMEM);
			return;
		}
		data_rd_sqe->flags |= RTIO_SQE_CHAINED;
	} else {
		/** No need additional actions as probably we'll want to
		 * flush the data or just report the event.
		 */
	}

	struct rtio_sqe *complete_sqe = rtio_sqe_acquire(data->bus.rtio.ctx);

	if (!complete_sqe) {
		LOG_ERR("Failed to acquire complete_sqe");
		rtio_sqe_drop_all(data->bus.rtio.ctx);
		icm45686_stream_result(dev, -ENOMEM);
		return;
	}
	rtio_sqe_prep_callback_no_cqe(complete_sqe,
				      icm45686_complete_handler,
				      (void *)dev,
				      data->stream.iodev_sqe);

	rtio_submit(data->bus.rtio.ctx, 0);
}

#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(invensense_icm45686, i3c)
static int icm45686_ibi_cb(struct i3c_device_desc *target,
			   struct i3c_ibi_payload *payload)
{
	icm45686_event_handler(target->dev);

	return 0;
}
#endif

static void icm45686_gpio_callback(const struct device *gpio_dev,
				   struct gpio_callback *cb,
				   uint32_t pins)
{
	struct icm45686_stream *stream = CONTAINER_OF(cb,
						      struct icm45686_stream,
						      cb);
	const struct device *dev = stream->dev;

	icm45686_event_handler(dev);
}

static inline bool settings_changed(const struct icm45686_stream *a,
				    const struct icm45686_stream *b)
{
	return (a->settings.enabled.drdy != b->settings.enabled.drdy) ||
	       (a->settings.opt.drdy != b->settings.opt.drdy) ||
	       (a->settings.enabled.fifo_ths != b->settings.enabled.fifo_ths) ||
	       (a->settings.opt.fifo_ths != b->settings.opt.fifo_ths) ||
	       (a->settings.enabled.fifo_full != b->settings.enabled.fifo_full) ||
	       (a->settings.opt.fifo_full != b->settings.opt.fifo_full);
}

void icm45686_stream_submit(const struct device *dev,
			    struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *read_cfg = iodev_sqe->sqe.iodev->data;
	struct icm45686_data *data = dev->data;
	const struct icm45686_config *cfg = dev->config;
	const bool wm_gt_ths = !cfg->settings.fifo_watermark_equals;
	uint8_t val = 0;
	int err;

	/** This separate struct is required due to the streaming API using a
	 * multi-shot RTIO submission: meaning, re-submitting itself after
	 * completion; hence, we don't have context to determine if this was
	 * the first submission that kicked things off. We're then, inferring
	 * this by comparing if the read-config has changed, and only restart
	 * in such case.
	 */
	struct icm45686_stream stream = {0};

	for (size_t i = 0 ; i  < read_cfg->count ; i++) {
		switch (read_cfg->triggers[i].trigger) {
		case SENSOR_TRIG_DATA_READY:
			stream.settings.enabled.drdy = true;
			stream.settings.opt.drdy = read_cfg->triggers[i].opt;
			break;
		case SENSOR_TRIG_FIFO_WATERMARK:
			stream.settings.enabled.fifo_ths = true;
			stream.settings.opt.fifo_ths = read_cfg->triggers[i].opt;
			break;
		case SENSOR_TRIG_FIFO_FULL:
			stream.settings.enabled.fifo_full = true;
			stream.settings.opt.fifo_full = read_cfg->triggers[i].opt;
			break;
		default:
			LOG_ERR("Unsupported trigger (%d)", read_cfg->triggers[i].trigger);
			rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
			return;
		}
	}

	__ASSERT(stream.settings.enabled.drdy ^
		 ((stream.settings.enabled.fifo_ths || stream.settings.enabled.fifo_full)),
		 "DRDY should not be enabled alongside FIFO triggers");

	__ASSERT(!stream.settings.enabled.fifo_ths ||
		 (stream.settings.enabled.fifo_ths && cfg->settings.fifo_watermark),
		 "FIFO watermark trigger requires a watermark level. Please "
		 "configure it on the device-tree");

	/* Store context for next submission (handled within callbacks) */
	data->stream.iodev_sqe = iodev_sqe;
	(void)atomic_set(&data->stream.state, ICM45686_STREAM_ON);

	if (settings_changed(&data->stream, &stream)) {

		data->stream.settings = stream.settings;

		/* Disable all interrupts before re-configuring */
		err = icm45686_reg_write_rtio(&data->bus, REG_INT1_CONFIG0, &val, 1);
		if (err) {
			LOG_ERR("Failed to disable interrupts on INT1_CONFIG0: %d", err);
			icm45686_stream_result(dev, err);
			return;
		}

		/* Read flags to clear them */
		err = icm45686_reg_read_rtio(&data->bus, REG_INT1_STATUS0 | REG_READ_BIT, &val, 1);
		if (err) {
			LOG_ERR("Failed to read INT1_STATUS0: %d", err);
			icm45686_stream_result(dev, err);
			return;
		}

		val = REG_FIFO_CONFIG3_FIFO_EN(false) |
		      REG_FIFO_CONFIG3_FIFO_ACCEL_EN(false) |
		      REG_FIFO_CONFIG3_FIFO_GYRO_EN(false) |
		      REG_FIFO_CONFIG3_FIFO_HIRES_EN(false);
		err = icm45686_reg_write_rtio(&data->bus, REG_FIFO_CONFIG3, &val, 1);
		if (err) {
			LOG_ERR("Failed to disable all FIFO settings: %d", err);
			icm45686_stream_result(dev, err);
			return;
		}

		val = REG_INT1_CONFIG0_STATUS_EN_DRDY(data->stream.settings.enabled.drdy) |
		      REG_INT1_CONFIG0_STATUS_EN_FIFO_THS(data->stream.settings.enabled.fifo_ths) |
		      REG_INT1_CONFIG0_STATUS_EN_FIFO_FULL(data->stream.settings.enabled.fifo_full);
		err = icm45686_reg_write_rtio(&data->bus, REG_INT1_CONFIG0, &val, 1);
		if (err) {
			LOG_ERR("Failed to configure INT1_CONFIG0: %d", err);
			icm45686_stream_result(dev, err);
			return;
		}

		val = REG_FIFO_CONFIG0_FIFO_MODE(REG_FIFO_CONFIG0_FIFO_MODE_BYPASS) |
		      REG_FIFO_CONFIG0_FIFO_DEPTH(REG_FIFO_CONFIG0_FIFO_DEPTH_2K);
		err = icm45686_reg_write_rtio(&data->bus, REG_FIFO_CONFIG0, &val, 1);
		if (err) {
			LOG_ERR("Failed to disable FIFO: %d", err);
			icm45686_stream_result(dev, err);
			return;
		}

		if (data->stream.settings.enabled.fifo_ths ||
		    data->stream.settings.enabled.fifo_full) {
			/** AN-000364: When operating in FIFO streaming mode, if FIFO threshold
			 * interrupt is triggered with M number of FIFO frames accumulated in the
			 * FIFO buffer, the host should only read the first M-1 number of FIFO
			 * frames.
			 *
			 * To avoid the case where M == 1 and M-- would be 0,
			 * M + 1 threshold is used so M count is read.
			 */
			uint16_t fifo_ths = data->stream.settings.enabled.fifo_ths ?
					    cfg->settings.fifo_watermark + 1 : 0;

			val = REG_FIFO_CONFIG2_FIFO_WM_GT_THS(wm_gt_ths) |
			      REG_FIFO_CONFIG2_FIFO_FLUSH(true);
			err = icm45686_reg_write_rtio(&data->bus, REG_FIFO_CONFIG2, &val, 1);
			if (err) {
				LOG_ERR("Failed to configure greater-than FIFO threshold: %d", err);
				icm45686_stream_result(dev, err);
				return;
			}

			err = icm45686_reg_write_rtio(&data->bus, REG_FIFO_CONFIG1_0,
						      (uint8_t *)&fifo_ths, 2);
			if (err) {
				LOG_ERR("Failed to configure FIFO watermark: %d", err);
				icm45686_stream_result(dev, err);
				return;
			}

			val = REG_FIFO_CONFIG0_FIFO_MODE(REG_FIFO_CONFIG0_FIFO_MODE_STREAM) |
			      REG_FIFO_CONFIG0_FIFO_DEPTH(REG_FIFO_CONFIG0_FIFO_DEPTH_2K);
			err = icm45686_reg_write_rtio(&data->bus, REG_FIFO_CONFIG0, &val, 1);
			if (err) {
				LOG_ERR("Failed to disable FIFO: %d", err);
				icm45686_stream_result(dev, err);
				return;
			}

			val = REG_FIFO_CONFIG3_FIFO_EN(true) |
			      REG_FIFO_CONFIG3_FIFO_ACCEL_EN(true) |
			      REG_FIFO_CONFIG3_FIFO_GYRO_EN(true) |
			      REG_FIFO_CONFIG3_FIFO_HIRES_EN(true);
			err = icm45686_reg_write_rtio(&data->bus, REG_FIFO_CONFIG3, &val, 1);
			if (err) {
				LOG_ERR("Failed to enable FIFO: %d", err);
				icm45686_stream_result(dev, err);
				return;
			}
		}
	}
	(void)gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}

int icm45686_stream_init(const struct device *dev)
{
	const struct icm45686_config *cfg = dev->config;
	struct icm45686_data *data = dev->data;
	uint8_t val = 0;
	int err;

	/** Needed to get back the device handle from the callback context */
	data->stream.dev = dev;

	(void)atomic_set(&data->stream.state, ICM45686_STREAM_OFF);

	if (cfg->int_gpio.port) {
		if (!gpio_is_ready_dt(&cfg->int_gpio)) {
			LOG_ERR("Interrupt GPIO not ready");
			return -ENODEV;
		}

		err = gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT);
		if (err) {
			LOG_ERR("Failed to configure interrupt GPIO");
			return -EIO;
		}

		gpio_init_callback(&data->stream.cb,
				   icm45686_gpio_callback,
				   BIT(cfg->int_gpio.pin));

		err = gpio_add_callback(cfg->int_gpio.port, &data->stream.cb);
		if (err) {
			LOG_ERR("Failed to add interrupt callback");
			return -EIO;
		}

		err = icm45686_reg_write_rtio(&data->bus, REG_INT1_CONFIG0, &val, 1);
		if (err) {
			LOG_ERR("Failed to disable all INTs");
		}

		val = REG_INT1_CONFIG2_EN_OPEN_DRAIN(false) |
		      REG_INT1_CONFIG2_EN_ACTIVE_HIGH(true);

		err = icm45686_reg_write_rtio(&data->bus, REG_INT1_CONFIG2, &val, 1);
		if (err) {
			LOG_ERR("Failed to configure INT as push-pull: %d", err);
		}
#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(invensense_icm45686, i3c)
	/** I3C devices use IBI only if no GPIO INT pin is defined. */
	} else if (data->bus.rtio.type == ICM45686_BUS_I3C) {
		const struct i3c_iodev_data *iodev_data = data->bus.rtio.iodev->data;

		data->bus.rtio.i3c.desc = i3c_device_find(iodev_data->bus, &data->bus.rtio.i3c.id);
		if (data->bus.rtio.i3c.desc == NULL) {
			LOG_ERR("Failed to find I3C device");
			return -ENODEV;
		}
		data->bus.rtio.i3c.desc->ibi_cb = icm45686_ibi_cb;

		err = i3c_ibi_enable(data->bus.rtio.i3c.desc);
		if (err) {
			LOG_ERR("Failed to enable IBI: %d", err);
			return err;
		}
#endif
	} else {
		LOG_ERR("Interrupt GPIO not supplied");
		return -ENODEV;
	}

	return 0;
}
