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

#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(invensense_icm45686, i3c)
#include <zephyr/drivers/i3c.h>
#endif

#include "icm45686.h"
#include "icm45686_bus.h"
#include "icm45686_stream.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ICM45686_STREAM, CONFIG_SENSOR_LOG_LEVEL);

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
	struct sensor_stream_trigger *trig_fifo_full = get_read_config_trigger(
		read_cfg,
		SENSOR_TRIG_FIFO_FULL);

	bool fifo_ths = int_status & REG_INT1_STATUS0_FIFO_THS(true);
	bool fifo_full = int_status & REG_INT1_STATUS0_FIFO_FULL(true);

	if ((trig_fifo_ths && trig_fifo_ths->opt == SENSOR_STREAM_DATA_DROP && fifo_ths) ||
	    (trig_fifo_full && trig_fifo_full->opt == SENSOR_STREAM_DATA_DROP && fifo_full)) {
		return true;
	}

	return false;
}

static inline bool should_read_fifo(const struct sensor_read_config *read_cfg,
				    uint8_t int_status)
{
	struct sensor_stream_trigger *trig_fifo_ths = get_read_config_trigger(
		read_cfg,
		SENSOR_TRIG_FIFO_WATERMARK);
	struct sensor_stream_trigger *trig_fifo_full = get_read_config_trigger(
		read_cfg,
		SENSOR_TRIG_FIFO_FULL);

	bool fifo_ths = int_status & REG_INT1_STATUS0_FIFO_THS(true);
	bool fifo_full = int_status & REG_INT1_STATUS0_FIFO_FULL(true);

	if ((trig_fifo_ths && trig_fifo_ths->opt == SENSOR_STREAM_DATA_INCLUDE && fifo_ths) ||
	    (trig_fifo_full && trig_fifo_full->opt == SENSOR_STREAM_DATA_INCLUDE && fifo_full)) {
		return true;
	}

	return false;
}

static inline bool should_read_data(const struct sensor_read_config *read_cfg,
				    uint8_t int_status)
{
	struct sensor_stream_trigger *trig_drdy = get_read_config_trigger(
		read_cfg,
		SENSOR_TRIG_DATA_READY);

	bool drdy = int_status & REG_INT1_STATUS0_DRDY(true);

	if (trig_drdy && trig_drdy->opt == SENSOR_STREAM_DATA_INCLUDE && drdy) {
		return true;
	}

	return false;
}

static void icm45686_complete_result(struct rtio *ctx,
				     const struct rtio_sqe *sqe,
				     void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct icm45686_data *data = dev->data;

	struct rtio_iodev_sqe *iodev_sqe = data->stream.iodev_sqe;

	memset(&data->stream.data, 0, sizeof(data->stream.data));

	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

static void icm45686_handle_event_actions(struct rtio *ctx,
					  const struct rtio_sqe *sqe,
					  void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct icm45686_data *data = dev->data;
	const struct sensor_read_config *read_cfg = data->stream.iodev_sqe->sqe.iodev->data;
	uint8_t int_status = data->stream.data.int_status;
	int err;

	data->stream.data.events.drdy = int_status & REG_INT1_STATUS0_DRDY(true);
	data->stream.data.events.fifo_ths = int_status & REG_INT1_STATUS0_FIFO_THS(true);
	data->stream.data.events.fifo_full = int_status & REG_INT1_STATUS0_FIFO_FULL(true);

	__ASSERT(data->stream.data.fifo_count > 0 &&
		 data->stream.data.fifo_count <= FIFO_COUNT_MAX_HIGH_RES,
		 "Invalid fifo count: %d", data->stream.data.fifo_count);

	struct icm45686_encoded_data *buf;
	uint32_t buf_len;
	uint32_t buf_len_required = sizeof(struct icm45686_encoded_header);

	/** We just need the header to communicate the events occurred during
	 * this SQE. Only include more data if the associated trigger needs it.
	 */
	if (should_read_fifo(read_cfg, int_status)) {
		buf_len_required += (data->stream.data.fifo_count *
				     sizeof(struct icm45686_encoded_fifo_payload));
	} else if (should_read_data(read_cfg, int_status)) {
		buf_len_required += sizeof(struct icm45686_encoded_payload);
	}

	err = rtio_sqe_rx_buf(data->stream.iodev_sqe,
			      buf_len_required,
			      buf_len_required,
			      (uint8_t **)&buf,
			      &buf_len);
	__ASSERT(err == 0, "Failed to acquire buffer (len: %d) for encoded data: %d. "
			   "Please revisit RTIO queue sizing and look for "
			   "bottlenecks during sensor data processing",
			   buf_len_required, err);

	/** Still throw an error even if asserts are off */
	if (err) {
		struct rtio_iodev_sqe *iodev_sqe = data->stream.iodev_sqe;

		LOG_ERR("Failed to acquire buffer for encoded data: %d", err);

		data->stream.iodev_sqe = NULL;
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}

	LOG_DBG("Alloc buf - required: %d, alloc: %d", buf_len_required, buf_len);

	buf->header.timestamp = data->stream.data.timestamp;
	buf->header.fifo_count = 0;
	buf->header.channels = 0;
	buf->header.events = REG_INT1_STATUS0_DRDY(data->stream.data.events.drdy) |
			     REG_INT1_STATUS0_FIFO_THS(data->stream.data.events.fifo_ths) |
			     REG_INT1_STATUS0_FIFO_FULL(data->stream.data.events.fifo_full);

	if (should_read_fifo(read_cfg, int_status)) {

		struct rtio_sqe *data_wr_sqe = rtio_sqe_acquire(ctx);
		struct rtio_sqe *data_rd_sqe = rtio_sqe_acquire(ctx);
		uint8_t read_reg;

		if (!data_wr_sqe || !data_rd_sqe) {
			struct rtio_iodev_sqe *iodev_sqe = data->stream.iodev_sqe;

			LOG_ERR("Failed to acquire RTIO SQEs");

			data->stream.iodev_sqe = NULL;
			rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
			return;
		}

		/** In FIFO data, the scale is fixed, irrespective of
		 * the configured settings.
		 */
		buf->header.accel_fs = ICM45686_DT_ACCEL_FS_32;
		buf->header.gyro_fs = ICM45686_DT_GYRO_FS_4000;
		buf->header.channels = 0x7F; /* Signal all channels are available */
		buf->header.fifo_count = data->stream.data.fifo_count;

		read_reg = REG_FIFO_DATA | REG_READ_BIT;
		rtio_sqe_prep_tiny_write(data_wr_sqe,
					 data->rtio.iodev,
					 RTIO_PRIO_HIGH,
					 &read_reg,
					 1,
					 NULL);
		data_wr_sqe->flags |= RTIO_SQE_TRANSACTION;

		rtio_sqe_prep_read(data_rd_sqe,
				   data->rtio.iodev,
				   RTIO_PRIO_HIGH,
				   (uint8_t *)&buf->fifo_payload,
				   (buf->header.fifo_count *
				   sizeof(struct icm45686_encoded_fifo_payload)),
				   NULL);
		if (data->rtio.type == ICM45686_BUS_I2C) {
			data_rd_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
		} else if (data->rtio.type == ICM45686_BUS_I3C) {
			data_rd_sqe->iodev_flags |= RTIO_IODEV_I3C_STOP | RTIO_IODEV_I3C_RESTART;
		}
		data_rd_sqe->flags |= RTIO_SQE_CHAINED;

	} else if (should_flush_fifo(read_cfg, int_status)) {

		struct rtio_sqe *write_sqe = rtio_sqe_acquire(ctx);

		if (!write_sqe) {
			struct rtio_iodev_sqe *iodev_sqe = data->stream.iodev_sqe;

			LOG_ERR("Failed to acquire RTIO SQE");

			data->stream.iodev_sqe = NULL;
			rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
			return;
		}

		uint8_t write_reg[] = {
			REG_FIFO_CONFIG2,
			REG_FIFO_CONFIG2_FIFO_FLUSH(true) |
			REG_FIFO_CONFIG2_FIFO_WM_GT_THS(true)
		};

		rtio_sqe_prep_tiny_write(write_sqe,
					 data->rtio.iodev,
					 RTIO_PRIO_HIGH,
					 write_reg,
					 sizeof(write_reg),
					 NULL);
		if (data->rtio.type == ICM45686_BUS_I2C) {
			write_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP;
		} else if (data->rtio.type == ICM45686_BUS_I3C) {
			write_sqe->iodev_flags |= RTIO_IODEV_I3C_STOP;
		}
		write_sqe->flags |= RTIO_SQE_CHAINED;

	} else if (should_read_data(read_cfg, int_status)) {

		buf->header.accel_fs = data->edata.header.accel_fs;
		buf->header.gyro_fs = data->edata.header.gyro_fs;
		buf->header.channels = 0x7F; /* Signal all channels are available */

		struct rtio_sqe *write_sqe = rtio_sqe_acquire(ctx);
		struct rtio_sqe *read_sqe = rtio_sqe_acquire(ctx);

		if (!write_sqe || !read_sqe) {
			struct rtio_iodev_sqe *iodev_sqe = data->stream.iodev_sqe;

			LOG_ERR("Failed to acquire RTIO SQEs");

			data->stream.iodev_sqe = NULL;
			rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
			return;
		}

		uint8_t read_reg = REG_ACCEL_DATA_X1_UI | REG_READ_BIT;

		rtio_sqe_prep_tiny_write(write_sqe,
					 data->rtio.iodev,
					 RTIO_PRIO_HIGH,
					 &read_reg,
					 1,
					 NULL);
		write_sqe->flags |= RTIO_SQE_TRANSACTION;

		rtio_sqe_prep_read(read_sqe,
				   data->rtio.iodev,
				   RTIO_PRIO_HIGH,
				   buf->payload.buf,
				   sizeof(buf->payload.buf),
				   NULL);
		if (data->rtio.type == ICM45686_BUS_I2C) {
			read_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
		} else if (data->rtio.type == ICM45686_BUS_I3C) {
			read_sqe->iodev_flags |= RTIO_IODEV_I3C_STOP | RTIO_IODEV_I3C_RESTART;
		}
		read_sqe->flags |= RTIO_SQE_CHAINED;
	}

	struct rtio_cqe *cqe;

	do {
		cqe = rtio_cqe_consume(ctx);
		if (cqe != NULL) {
			err = cqe->result;
			rtio_cqe_release(ctx, cqe);
		}
	} while (cqe != NULL);

	struct rtio_sqe *cb_sqe = rtio_sqe_acquire(ctx);

	if (!cb_sqe) {
		LOG_ERR("Failed to acquire RTIO SQE for completion callback");
		rtio_iodev_sqe_err(data->stream.iodev_sqe, -ENOMEM);
		return;
	}

	rtio_sqe_prep_callback_no_cqe(cb_sqe,
				      icm45686_complete_result,
				      (void *)dev,
				      data->stream.iodev_sqe);

	rtio_submit(ctx, 0);
}

static void icm45686_event_handler(const struct device *dev)
{
	struct icm45686_data *data = dev->data;
	uint8_t val = 0;
	uint64_t cycles;
	int err;

	if (!data->stream.iodev_sqe ||
	    FIELD_GET(RTIO_SQE_CANCELED, data->stream.iodev_sqe->sqe.flags)) {
		LOG_WRN("Callback triggered with no streaming submission - Disabling interrupts");

		struct rtio_sqe *write_sqe = rtio_sqe_acquire(data->rtio.ctx);
		uint8_t wr_data[] = {REG_INT1_CONFIG0, 0x00};

		rtio_sqe_prep_tiny_write(write_sqe,
					 data->rtio.iodev,
					 RTIO_PRIO_HIGH,
					 wr_data,
					 sizeof(wr_data),
					 NULL);
		if (data->rtio.type == ICM45686_BUS_I2C) {
			write_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP;
		} else if (data->rtio.type == ICM45686_BUS_I3C) {
			write_sqe->iodev_flags |= RTIO_IODEV_I3C_STOP;
		}
		rtio_submit(data->rtio.ctx, 0);

		data->stream.settings.enabled.drdy = false;
		data->stream.settings.enabled.fifo_ths = false;
		data->stream.settings.enabled.fifo_full = false;
		return;
	} else if (!atomic_cas(&data->stream.in_progress, 0, 1)) {
		/** There's an on-going */
		return;
	}

	err = sensor_clock_get_cycles(&cycles);
	if (err) {
		struct rtio_iodev_sqe *iodev_sqe = data->stream.iodev_sqe;

		LOG_ERR("Failed to get timestamp: %d", err);

		data->stream.iodev_sqe = NULL;
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}

	data->stream.data.timestamp = sensor_clock_cycles_to_ns(cycles);

	/** Prepare an asynchronous read of the INT status register */
	struct rtio_sqe *write_sqe = rtio_sqe_acquire(data->rtio.ctx);
	struct rtio_sqe *read_sqe = rtio_sqe_acquire(data->rtio.ctx);
	struct rtio_sqe *write_fifo_ct_sqe = rtio_sqe_acquire(data->rtio.ctx);
	struct rtio_sqe *read_fifo_ct_sqe = rtio_sqe_acquire(data->rtio.ctx);
	struct rtio_sqe *complete_sqe = rtio_sqe_acquire(data->rtio.ctx);

	if (!write_sqe || !read_sqe || !complete_sqe) {
		struct rtio_iodev_sqe *iodev_sqe = data->stream.iodev_sqe;

		LOG_ERR("Failed to acquire RTIO SQEs");

		data->stream.iodev_sqe = NULL;
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	/** Directly read Status Register to determine what triggered the event */
	val = REG_INT1_STATUS0 | REG_READ_BIT;
	rtio_sqe_prep_tiny_write(write_sqe,
				 data->rtio.iodev,
				 RTIO_PRIO_HIGH,
				 &val,
				 1,
				 NULL);
	write_sqe->flags |= RTIO_SQE_TRANSACTION;

	rtio_sqe_prep_read(read_sqe,
			   data->rtio.iodev,
			   RTIO_PRIO_HIGH,
			   &data->stream.data.int_status,
			   1,
			   NULL);
	if (data->rtio.type == ICM45686_BUS_I2C) {
		read_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
	} else if (data->rtio.type == ICM45686_BUS_I3C) {
		read_sqe->iodev_flags |= RTIO_IODEV_I3C_STOP | RTIO_IODEV_I3C_RESTART;
	}
	read_sqe->flags |= RTIO_SQE_CHAINED;


	/** Preemptively read FIFO count so we can decide on the next callback
	 * how much FIFO data we'd read (if needed).
	 */
	val = REG_FIFO_COUNT_0 | REG_READ_BIT;
	rtio_sqe_prep_tiny_write(write_fifo_ct_sqe,
				 data->rtio.iodev,
				 RTIO_PRIO_HIGH,
				 &val,
				 1,
				 NULL);
	write_fifo_ct_sqe->flags |= RTIO_SQE_TRANSACTION;

	rtio_sqe_prep_read(read_fifo_ct_sqe,
			   data->rtio.iodev,
			   RTIO_PRIO_HIGH,
			   (uint8_t *)&data->stream.data.fifo_count,
			   2,
			   NULL);
	if (data->rtio.type == ICM45686_BUS_I2C) {
		read_fifo_ct_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
	} else if (data->rtio.type == ICM45686_BUS_I3C) {
		read_fifo_ct_sqe->iodev_flags |= RTIO_IODEV_I3C_STOP | RTIO_IODEV_I3C_RESTART;
	}
	read_fifo_ct_sqe->flags |= RTIO_SQE_CHAINED;

	rtio_sqe_prep_callback_no_cqe(complete_sqe,
				      icm45686_handle_event_actions,
				      (void *)dev,
				      data->stream.iodev_sqe);

	rtio_submit(data->rtio.ctx, 0);
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

	(void)atomic_clear(&data->stream.in_progress);

	if (settings_changed(&data->stream, &stream)) {

		data->stream.settings = stream.settings;

		/* Disable all interrupts before re-configuring */
		err = icm45686_bus_write(dev, REG_INT1_CONFIG0, &val, 1);
		if (err) {
			LOG_ERR("Failed to disable interrupts on INT1_CONFIG0: %d", err);
			data->stream.iodev_sqe = NULL;
			rtio_iodev_sqe_err(iodev_sqe, err);
			return;
		}

		/* Read flags to clear them */
		err = icm45686_bus_read(dev, REG_INT1_STATUS0, &val, 1);
		if (err) {
			LOG_ERR("Failed to read INT1_STATUS0: %d", err);
			data->stream.iodev_sqe = NULL;
			rtio_iodev_sqe_err(iodev_sqe, err);
			return;
		}

		val = REG_FIFO_CONFIG3_FIFO_EN(false) |
		      REG_FIFO_CONFIG3_FIFO_ACCEL_EN(false) |
		      REG_FIFO_CONFIG3_FIFO_GYRO_EN(false) |
		      REG_FIFO_CONFIG3_FIFO_HIRES_EN(false);
		err = icm45686_bus_write(dev, REG_FIFO_CONFIG3, &val, 1);
		if (err) {
			LOG_ERR("Failed to disable all FIFO settings: %d", err);
			data->stream.iodev_sqe = NULL;
			rtio_iodev_sqe_err(iodev_sqe, err);
			return;
		}

		val = REG_INT1_CONFIG0_STATUS_EN_DRDY(data->stream.settings.enabled.drdy) |
		      REG_INT1_CONFIG0_STATUS_EN_FIFO_THS(data->stream.settings.enabled.fifo_ths) |
		      REG_INT1_CONFIG0_STATUS_EN_FIFO_FULL(data->stream.settings.enabled.fifo_full);
		err = icm45686_bus_write(dev, REG_INT1_CONFIG0, &val, 1);
		if (err) {
			LOG_ERR("Failed to configure INT1_CONFIG0: %d", err);
			data->stream.iodev_sqe = NULL;
			rtio_iodev_sqe_err(iodev_sqe, err);
			return;
		}

		val = REG_FIFO_CONFIG0_FIFO_MODE(REG_FIFO_CONFIG0_FIFO_MODE_BYPASS) |
		      REG_FIFO_CONFIG0_FIFO_DEPTH(REG_FIFO_CONFIG0_FIFO_DEPTH_2K);
		err = icm45686_bus_write(dev, REG_FIFO_CONFIG0, &val, 1);
		if (err) {
			LOG_ERR("Failed to disable FIFO: %d", err);
			data->stream.iodev_sqe = NULL;
			rtio_iodev_sqe_err(iodev_sqe, err);
			return;
		}

		if (data->stream.settings.enabled.fifo_ths ||
		    data->stream.settings.enabled.fifo_full) {
			uint16_t fifo_ths = data->stream.settings.enabled.fifo_ths ?
					    cfg->settings.fifo_watermark : 0;

			val = REG_FIFO_CONFIG2_FIFO_WM_GT_THS(true) |
			      REG_FIFO_CONFIG2_FIFO_FLUSH(true);
			err = icm45686_bus_write(dev, REG_FIFO_CONFIG2, &val, 1);
			if (err) {
				LOG_ERR("Failed to configure greater-than FIFO threshold: %d", err);
				data->stream.iodev_sqe = NULL;
				rtio_iodev_sqe_err(iodev_sqe, err);
				return;
			}

			err = icm45686_bus_write(dev, REG_FIFO_CONFIG1_0, (uint8_t *)&fifo_ths, 2);
			if (err) {
				LOG_ERR("Failed to configure FIFO watermark: %d", err);
				data->stream.iodev_sqe = NULL;
				rtio_iodev_sqe_err(iodev_sqe, err);
				return;
			}

			val = REG_FIFO_CONFIG0_FIFO_MODE(REG_FIFO_CONFIG0_FIFO_MODE_STREAM) |
			      REG_FIFO_CONFIG0_FIFO_DEPTH(REG_FIFO_CONFIG0_FIFO_DEPTH_2K);
			err = icm45686_bus_write(dev, REG_FIFO_CONFIG0, &val, 1);
			if (err) {
				LOG_ERR("Failed to disable FIFO: %d", err);
				data->stream.iodev_sqe = NULL;
				rtio_iodev_sqe_err(iodev_sqe, err);
				return;
			}

			val = REG_FIFO_CONFIG3_FIFO_EN(true) |
			      REG_FIFO_CONFIG3_FIFO_ACCEL_EN(true) |
			      REG_FIFO_CONFIG3_FIFO_GYRO_EN(true) |
			      REG_FIFO_CONFIG3_FIFO_HIRES_EN(true);
			err = icm45686_bus_write(dev, REG_FIFO_CONFIG3, &val, 1);
			if (err) {
				LOG_ERR("Failed to enable FIFO: %d", err);
				data->stream.iodev_sqe = NULL;
				rtio_iodev_sqe_err(iodev_sqe, err);
				return;
			}
		}
	}
}

int icm45686_stream_init(const struct device *dev)
{
	const struct icm45686_config *cfg = dev->config;
	struct icm45686_data *data = dev->data;
	uint8_t val = 0;
	int err;

	/** Needed to get back the device handle from the callback context */
	data->stream.dev = dev;

	(void)atomic_clear(&data->stream.in_progress);

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

		err = gpio_pin_interrupt_configure_dt(&cfg->int_gpio,
						      GPIO_INT_EDGE_TO_ACTIVE);
		if (err) {
			LOG_ERR("Failed to configure interrupt");
		}

		err = icm45686_bus_write(dev, REG_INT1_CONFIG0, &val, 1);
		if (err) {
			LOG_ERR("Failed to disable all INTs");
		}

		val = REG_INT1_CONFIG2_EN_OPEN_DRAIN(false) |
		      REG_INT1_CONFIG2_EN_ACTIVE_HIGH(true);

		err = icm45686_bus_write(dev, REG_INT1_CONFIG2, &val, 1);
		if (err) {
			LOG_ERR("Failed to configure INT as push-pull: %d", err);
		}
#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(invensense_icm45686, i3c)
	/** I3C devices use IBI only if no GPIO INT pin is defined. */
	} else if (data->rtio.type == ICM45686_BUS_I3C) {
		const struct i3c_iodev_data *iodev_data = data->rtio.iodev->data;

		data->rtio.i3c.desc = i3c_device_find(iodev_data->bus,
						      &data->rtio.i3c.id);
		if (data->rtio.i3c.desc == NULL) {
			LOG_ERR("Failed to find I3C device");
			return -ENODEV;
		}
		data->rtio.i3c.desc->ibi_cb = icm45686_ibi_cb;

		err = i3c_ibi_enable(data->rtio.i3c.desc);
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
