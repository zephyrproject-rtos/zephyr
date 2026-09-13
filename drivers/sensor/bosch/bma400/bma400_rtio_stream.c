/*
 * Bosch BMA400 3-axis accelerometer driver
 * SPDX-FileCopyrightText: Copyright 2026 Luca Gessi lucagessi90@gmail.com
 * SPDX-FileCopyrightText: Copyright 2026 The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bma400

#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor_clock.h>

#include "bma400.h"
#include "bma400_interrupt.h"
#include "bma400_defs.h"
#include "bma400_decoder.h"
#include "bma400_rtio.h"

#ifdef CONFIG_BMA400_STREAM

LOG_MODULE_DECLARE(bma400, CONFIG_SENSOR_LOG_LEVEL);

static void bma400_rearm_gpio(struct bma400_data *drv_data)
{
	const struct bma400_config *cfg = drv_data->dev->config;

	gpio_pin_interrupt_configure_dt(&cfg->gpio_interrupt, BMA400_INT_MODE);
}

static inline bool bma400_stream_canceled(const struct rtio_iodev_sqe *iodev_sqe)
{
	return FIELD_GET(RTIO_SQE_CANCELED, iodev_sqe->sqe.flags) != 0;
}

static void bma400_stream_service_done(struct bma400_data *drv_data)
{
	bma400_rearm_gpio(drv_data);
	atomic_clear(&drv_data->stream_busy);
}

static void bma400_stream_cancel_done(struct bma400_data *drv_data)
{
	struct rtio_iodev_sqe *streaming_sqe = drv_data->streaming_sqe;

	drv_data->streaming_sqe = NULL;
	rtio_iodev_sqe_err(streaming_sqe, -ECANCELED);
	atomic_clear(&drv_data->stream_busy);
}

static void bma400_submit_stream_err(struct bma400_data *data, struct rtio_iodev_sqe *iodev_sqe,
				     bool owns_service, int err)
{
	data->streaming_sqe = NULL;
	rtio_iodev_sqe_err(iodev_sqe, err);
	if (owns_service) {
		atomic_clear(&data->stream_busy);
	}
}

void bma400_submit_stream(const struct device *sensor, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	struct bma400_data *data = sensor->data;
	struct bma400_runtime_config new_config = data->cfg;
	const struct bma400_config *cfg_bma400 = sensor->config;
	bool owns_service = false;
	bool reconfigure;
	int ret;

	new_config.fifo_wm_en = false;
	new_config.fifo_full_en = false;
	new_config.motion_detection_en = false;
	new_config.fifo_wm_data_opt = SENSOR_STREAM_DATA_NOP;
	new_config.fifo_full_data_opt = SENSOR_STREAM_DATA_NOP;

	for (size_t i = 0; i < cfg->count; i++) {
		switch (cfg->triggers[i].trigger) {
		case SENSOR_TRIG_FIFO_WATERMARK:
			new_config.fifo_wm_en = true;
			new_config.fifo_wm_data_opt = cfg->triggers[i].opt;
			break;
		case SENSOR_TRIG_FIFO_FULL:
			new_config.fifo_full_en = true;
			new_config.fifo_full_data_opt = cfg->triggers[i].opt;
			break;
		case SENSOR_TRIG_MOTION:
			new_config.motion_detection_en = true;
			break;
		default:
			LOG_ERR("Trigger (%d) not supported", cfg->triggers[i].trigger);
			rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
			return;
		}
	}

	if (new_config.motion_detection_en && (new_config.fifo_wm_en || new_config.fifo_full_en)) {
		LOG_ERR("Motion and FIFO triggers cannot be streamed together");
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
		return;
	}

	if (data->streaming_sqe != NULL) {
		if (!bma400_stream_canceled(data->streaming_sqe)) {
			LOG_ERR("Stream already ongoing");
			rtio_iodev_sqe_err(iodev_sqe, -EBUSY);
			return;
		}

		if (!atomic_cas(&data->stream_busy, 0, 1)) {
			LOG_DBG("Cancelled stream still being serviced");
			rtio_iodev_sqe_err(iodev_sqe, -EBUSY);
			return;
		}
		owns_service = true;

		struct rtio_iodev_sqe *canceled_sqe = data->streaming_sqe;

		data->streaming_sqe = NULL;
		rtio_iodev_sqe_err(canceled_sqe, -ECANCELED);
	}

	reconfigure = owns_service || new_config.fifo_wm_en != data->cfg.fifo_wm_en ||
		      new_config.fifo_full_en != data->cfg.fifo_full_en ||
		      new_config.motion_detection_en != data->cfg.motion_detection_en;

	data->streaming_sqe = iodev_sqe;

	if (reconfigure) {
		ret = gpio_pin_interrupt_configure_dt(&cfg_bma400->gpio_interrupt,
						      GPIO_INT_DISABLE);
		if (ret) {
			LOG_ERR("Failed to disable interrupt 1");
			bma400_submit_stream_err(data, iodev_sqe, owns_service, ret);
			return;
		}
		ret = bma400_stream_configure(sensor, &new_config);

		if (ret) {
			LOG_ERR("Failed to configure sensor");
			bma400_submit_stream_err(data, iodev_sqe, owns_service, ret);
			return;
		}

		data->cfg.fifo_wm_en = new_config.fifo_wm_en;
		data->cfg.fifo_full_en = new_config.fifo_full_en;
		data->cfg.motion_detection_en = new_config.motion_detection_en;
		data->cfg.fifo_wm_data_opt = new_config.fifo_wm_data_opt;
		data->cfg.fifo_full_data_opt = new_config.fifo_full_data_opt;

		ret = gpio_pin_interrupt_configure_dt(&cfg_bma400->gpio_interrupt, BMA400_INT_MODE);

		if (ret) {
			LOG_ERR("Failed to set interrupt 1");
			bma400_submit_stream_err(data, iodev_sqe, owns_service, ret);
			return;
		}
	} else if (atomic_get(&data->stream_busy) == 0) {
		bma400_rearm_gpio(data);
	}

	if (owns_service) {
		atomic_clear(&data->stream_busy);
	}
}

static void bma400_fifo_complete_cb(struct rtio *r, const struct rtio_sqe *sqe, int result,
				    void *arg)
{
	const struct device *dev = arg;
	struct bma400_data *drv_data = dev->data;
	struct rtio_iodev_sqe *streaming_sqe = drv_data->streaming_sqe;

	ARG_UNUSED(sqe);
	if (streaming_sqe == NULL) {
		bma400_stream_service_done(drv_data);
		return;
	}

	if (bma400_stream_canceled(streaming_sqe)) {
		bma400_stream_cancel_done(drv_data);
		return;
	}

	if (result != 0) {
		drv_data->streaming_sqe = NULL;
		rtio_iodev_sqe_err(streaming_sqe, result);
		bma400_stream_service_done(drv_data);
		return;
	}

	drv_data->streaming_sqe = NULL;
	rtio_iodev_sqe_ok(streaming_sqe, 0);
	bma400_stream_service_done(drv_data);
}

static void bma400_fifo_count_cb(struct rtio *r, const struct rtio_sqe *sqe, int result, void *arg)
{
	const struct device *dev = arg;
	struct bma400_data *drv_data = dev->data;
	struct rtio_iodev *iodev = drv_data->iodev;
	struct rtio_iodev_sqe *streaming_sqe = drv_data->streaming_sqe;
	uint16_t fifo_count;

	ARG_UNUSED(sqe);
	(void)rtio_flush_completion_queue(r);

	if (streaming_sqe == NULL) {
		bma400_stream_service_done(drv_data);
		return;
	}

	if (result != 0) {
		drv_data->streaming_sqe = NULL;
		rtio_iodev_sqe_err(streaming_sqe, result);
		bma400_stream_service_done(drv_data);
		return;
	}

	fifo_count = drv_data->read_buf[1] + ((uint16_t)(drv_data->read_buf[2] & 0x07) << 8);
	if (fifo_count == 0) {
		/* No data to read, complete with no error */
		drv_data->streaming_sqe = NULL;
		rtio_iodev_sqe_ok(streaming_sqe, 0);
		bma400_stream_service_done(drv_data);
		return;
	}
	/* Read all FIFO content */
	uint8_t *buf, *read_buf;
	uint32_t buf_len;
	uint32_t req_len =
		(fifo_count) + sizeof(struct bma400_fifo_data) + 1; /* +1 for SPI dummy byte */

	if (rtio_sqe_rx_buf(streaming_sqe, req_len, req_len, &buf, &buf_len) != 0) {
		LOG_ERR("Failed to get buffer");
		drv_data->streaming_sqe = NULL;
		rtio_iodev_sqe_err(streaming_sqe, -ENOMEM);
		bma400_stream_service_done(drv_data);
		return;
	}

	/* clang-format off */
	struct bma400_fifo_data hdr = {
		.header = {
			.is_fifo = 1,
			.timestamp = drv_data->timestamp,
			.has_motion_trigger =
			((drv_data->int_status & BMA400_ASSERTED_GEN2_INT) != 0),
			.has_fifo_wm_trigger =
			((drv_data->int_status & BMA400_ASSERTED_FIFO_WM_INT) != 0),
			.has_fifo_full_trigger =
			((drv_data->int_status & BMA400_ASSERTED_FIFO_FULL_INT) != 0),
			.accel_fs = drv_data->cfg.accel_fs_range,
		},
		.fifo_count = fifo_count,
		.accel_odr = drv_data->cfg.accel_odr,
	};
	/* clang-format on */

	memcpy(buf, &hdr, sizeof(hdr));
	read_buf = buf + sizeof(hdr);

	struct rtio_sqe *write_fifo_count_reg = rtio_sqe_acquire(r);
	struct rtio_sqe *read_fifo_count = rtio_sqe_acquire(r);
	struct rtio_sqe *check_fifo_count = rtio_sqe_acquire(r);

	if (write_fifo_count_reg == NULL || read_fifo_count == NULL || check_fifo_count == NULL) {
		LOG_ERR("Failed to acquire SQEs");
		drv_data->streaming_sqe = NULL;
		rtio_iodev_sqe_err(streaming_sqe, -ENOMEM);
		bma400_stream_service_done(drv_data);
		return;
	}

	uint8_t reg = 0x80 | BMA400_REG_FIFO_DATA;
	/* reg + dummy byte in SPI */
	rtio_sqe_prep_tiny_write(write_fifo_count_reg, iodev, RTIO_PRIO_NORM, &reg, 1, NULL);
	write_fifo_count_reg->flags = RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_fifo_count, iodev, RTIO_PRIO_NORM, read_buf, (fifo_count + 1),
			   NULL);
	read_fifo_count->flags = RTIO_SQE_CHAINED;

	rtio_sqe_prep_callback_no_cqe(check_fifo_count, bma400_fifo_complete_cb, arg, NULL);

	rtio_submit(r, 0);
}

static inline enum sensor_stream_data_opt
bma400_fifo_trig_to_data_opt(const struct bma400_runtime_config *cfg, const bool has_fifo_wm_trig,
			     const bool has_fifo_full_trig)
{
	if (has_fifo_wm_trig) {
		if (has_fifo_full_trig) {
			/* Both fifo threshold and full */
			return MIN(cfg->fifo_wm_data_opt, cfg->fifo_full_data_opt);
		}
		/* Only care about fifo threshold */
		return cfg->fifo_wm_data_opt;
	}
	if (has_fifo_full_trig) {
		/* Only care about fifo full */
		return cfg->fifo_full_data_opt;
	}
	return SENSOR_STREAM_DATA_NOP;
}

static void bma400_int_status_cb(struct rtio *r, const struct rtio_sqe *sqe, int result, void *arg)
{
	const struct device *dev = arg;
	struct bma400_data *drv_data = dev->data;
	struct rtio_iodev *iodev = drv_data->iodev;
	struct rtio_iodev_sqe *streaming_sqe = drv_data->streaming_sqe;

	ARG_UNUSED(sqe);
	(void)rtio_flush_completion_queue(r);

	if (streaming_sqe == NULL) {
		bma400_stream_service_done(drv_data);
		return;
	}

	if (result != 0) {
		drv_data->streaming_sqe = NULL;
		rtio_iodev_sqe_err(streaming_sqe, result);
		bma400_stream_service_done(drv_data);
		return;
	}

	drv_data->int_status = drv_data->read_buf[1];

	const bool has_fifo_wm_trig = (drv_data->int_status & BMA400_ASSERTED_FIFO_WM_INT) != 0;
	const bool has_fifo_full_trig = (drv_data->int_status & BMA400_ASSERTED_FIFO_FULL_INT) != 0;
	const bool has_motion_trig = (drv_data->int_status & BMA400_ASSERTED_GEN2_INT) != 0;

	if (!has_fifo_wm_trig && !has_fifo_full_trig && !has_motion_trig) {
		/* complete operation with no error */
		drv_data->streaming_sqe = NULL;
		rtio_iodev_sqe_ok(streaming_sqe, 0);
		bma400_stream_service_done(drv_data);
		return;
	}

	enum sensor_stream_data_opt data_opt;

	data_opt =
		bma400_fifo_trig_to_data_opt(&drv_data->cfg, has_fifo_wm_trig, has_fifo_full_trig);

	/* No data included */
	if (has_motion_trig && (data_opt != SENSOR_STREAM_DATA_INCLUDE)) {
		uint8_t *buf;
		uint32_t buf_len;

		drv_data->streaming_sqe = NULL;
		if (rtio_sqe_rx_buf(streaming_sqe, sizeof(struct bma400_fifo_data),
				    sizeof(struct bma400_fifo_data), &buf, &buf_len) != 0) {
			rtio_iodev_sqe_err(streaming_sqe, -ENOMEM);
			bma400_stream_service_done(drv_data);
			return;
		}

		struct bma400_fifo_data *data = (struct bma400_fifo_data *)buf;

		memset(buf, 0, buf_len);
		data->header.timestamp = drv_data->timestamp;
		data->int_status = drv_data->int_status;
		data->fifo_count = 0;
		data->header.has_motion_trigger = has_motion_trig;
		data->header.has_fifo_wm_trigger = has_fifo_wm_trig;
		data->header.has_fifo_full_trigger = has_fifo_full_trig;
		data->header.is_fifo = true;
		rtio_iodev_sqe_ok(streaming_sqe, 0);
		bma400_stream_service_done(drv_data);
		if (data_opt == SENSOR_STREAM_DATA_DROP) {
			struct rtio_sqe *write_reset_fifo = rtio_sqe_acquire(r);

			if (write_reset_fifo != NULL) {
				const uint8_t write_buffer[] = {
					BMA400_REG_CMD,
					BMA400_FIFO_FLUSH_CMD,
				};
				rtio_sqe_prep_tiny_write(write_reset_fifo, iodev, RTIO_PRIO_NORM,
							 write_buffer, ARRAY_SIZE(write_buffer),
							 NULL);
				write_reset_fifo->flags |= RTIO_SQE_NO_RESPONSE;
				rtio_submit(r, 0);
			}
		}
		return;
	}

	/* We need the data, read the fifo length */
	struct rtio_sqe *write_fifo_count_reg = rtio_sqe_acquire(r);
	struct rtio_sqe *read_fifo_count = rtio_sqe_acquire(r);
	struct rtio_sqe *check_fifo_count = rtio_sqe_acquire(r);

	if (write_fifo_count_reg == NULL || read_fifo_count == NULL || check_fifo_count == NULL) {
		LOG_ERR("Failed to acquire SQEs");
		drv_data->streaming_sqe = NULL;
		rtio_iodev_sqe_err(streaming_sqe, -ENOMEM);
		bma400_stream_service_done(drv_data);
		return;
	}

	uint8_t reg = 0x80 | BMA400_REG_FIFO_LENGTH0;
	/* reg + dummy byte in SPI */
	rtio_sqe_prep_tiny_write(write_fifo_count_reg, iodev, RTIO_PRIO_NORM, &reg, 1, NULL);
	write_fifo_count_reg->flags = RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_fifo_count, iodev, RTIO_PRIO_NORM, drv_data->read_buf, 3, NULL);
	read_fifo_count->flags = RTIO_SQE_CHAINED;

	rtio_sqe_prep_callback(check_fifo_count, bma400_fifo_count_cb, arg, NULL);

	rtio_submit(r, 0);
}

void bma400_stream_event(const struct device *dev)
{
	struct bma400_data *drv_data = dev->data;
	struct rtio_iodev *iodev = drv_data->iodev;
	struct rtio *r = drv_data->r;
	uint64_t cycles;
	int rc;

	if (!atomic_cas(&drv_data->stream_busy, 0, 1)) {
		bma400_rearm_gpio(drv_data);
		return;
	}

	if (drv_data->streaming_sqe == NULL) {
		bma400_stream_service_done(drv_data);
		return;
	}

	if (bma400_stream_canceled(drv_data->streaming_sqe)) {
		bma400_stream_cancel_done(drv_data);
		return;
	}

	rc = sensor_clock_get_cycles(&cycles);
	if (rc != 0) {
		struct rtio_iodev_sqe *streaming_sqe = drv_data->streaming_sqe;

		LOG_ERR("Failed to get sensor clock cycles");
		drv_data->streaming_sqe = NULL;
		rtio_iodev_sqe_err(streaming_sqe, rc);
		bma400_stream_service_done(drv_data);
		return;
	}

	drv_data->timestamp = sensor_clock_cycles_to_ns(cycles);

	struct rtio_sqe *write_int_reg = rtio_sqe_acquire(r);
	struct rtio_sqe *read_int_reg = rtio_sqe_acquire(r);
	struct rtio_sqe *check_int_status = rtio_sqe_acquire(r);

	if (write_int_reg == NULL || read_int_reg == NULL || check_int_status == NULL) {
		struct rtio_iodev_sqe *streaming_sqe = drv_data->streaming_sqe;

		LOG_ERR("Failed to acquire SQEs");
		drv_data->streaming_sqe = NULL;
		rtio_iodev_sqe_err(streaming_sqe, -ENOMEM);
		bma400_stream_service_done(drv_data);
		return;
	}

	/* Read INT source */
	uint8_t reg = 0x80 | BMA400_REG_INT_STAT0;
	/* reg + dummy byte in SPI */
	rtio_sqe_prep_tiny_write(write_int_reg, iodev, RTIO_PRIO_NORM, &reg, 1, NULL);
	write_int_reg->flags = RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_int_reg, iodev, RTIO_PRIO_NORM, drv_data->read_buf, 2, NULL);
	read_int_reg->flags = RTIO_SQE_CHAINED;

	rtio_sqe_prep_callback(check_int_status, bma400_int_status_cb, (void *)dev, NULL);
	rtio_submit(r, 0);
}

#endif /* CONFIG_BMA400_STREAM */
