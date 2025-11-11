/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/sys/check.h>
#include <zephyr/drivers/sensor_clock.h>
#include "icm4268x.h"
#include "icm4268x_decoder.h"
#include "icm4268x_reg.h"
#include "icm4268x_rtio.h"
#include "icm4268x_bus.h"

LOG_MODULE_DECLARE(ICM4268X_RTIO, CONFIG_SENSOR_LOG_LEVEL);

void icm4268x_submit_stream(const struct device *sensor, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct icm4268x_dev_cfg *dev_cfg = (const struct icm4268x_dev_cfg *)sensor->config;
	struct icm4268x_dev_data *data = sensor->data;
	struct icm4268x_cfg new_config = data->cfg;

	new_config.interrupt1_drdy = false;
	new_config.interrupt1_fifo_ths = false;
	new_config.interrupt1_fifo_full = false;
	for (int i = 0; i < cfg->count; ++i) {
		switch (cfg->triggers[i].trigger) {
		case SENSOR_TRIG_DATA_READY:
			new_config.interrupt1_drdy = true;
			break;
		case SENSOR_TRIG_FIFO_WATERMARK:
			new_config.interrupt1_fifo_ths = true;
			break;
		case SENSOR_TRIG_FIFO_FULL:
			new_config.interrupt1_fifo_full = true;
			break;
		default:
			LOG_DBG("Trigger (%d) not supported", cfg->triggers[i].trigger);
			rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
			return;
		}
	}

	if (new_config.interrupt1_drdy != data->cfg.interrupt1_drdy ||
	    new_config.interrupt1_fifo_ths != data->cfg.interrupt1_fifo_ths ||
	    new_config.interrupt1_fifo_full != data->cfg.interrupt1_fifo_full) {
		int rc = icm4268x_safely_configure(sensor, &new_config);

		if (rc != 0) {
			LOG_ERR("%p Failed to configure sensor", sensor);
			rtio_iodev_sqe_err(iodev_sqe, rc);
			return;
		}
	}

	(void)atomic_set(&data->state, ICM4268X_STREAM_ON);
	data->streaming_sqe = iodev_sqe;
	(void)gpio_pin_interrupt_configure_dt(&dev_cfg->gpio_int1, GPIO_INT_EDGE_TO_ACTIVE);
}

static struct sensor_stream_trigger *
icm4268x_get_read_config_trigger(const struct sensor_read_config *cfg,
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

static inline void icm4268x_stream_result(const struct device *dev, int result)
{
	struct icm4268x_dev_data *drv_data = dev->data;
	struct rtio_iodev_sqe *streaming_sqe = drv_data->streaming_sqe;

	drv_data->streaming_sqe = NULL;
	if (result < 0) {
		rtio_iodev_sqe_err(streaming_sqe, result);
	} else {
		rtio_iodev_sqe_ok(streaming_sqe, result);
	}
}

static void icm4268x_complete_cb(struct rtio *r, const struct rtio_sqe *sqe, int result, void *arg)
{
	ARG_UNUSED(result);

	const struct device *dev = arg;
	struct icm4268x_dev_data *drv_data = dev->data;
	const struct icm4268x_dev_cfg *dev_cfg = (const struct icm4268x_dev_cfg *)dev->config;
	struct rtio_iodev_sqe *streaming_sqe = drv_data->streaming_sqe;
	uint8_t *buf;
	uint32_t buf_len;
	int rc;

	if (drv_data->streaming_sqe == NULL ||
	    FIELD_GET(RTIO_SQE_CANCELED, drv_data->streaming_sqe->sqe.flags)) {
		LOG_ERR("%p Complete CB triggered with NULL handle. Disabling Interrupt", dev);
		(void)gpio_pin_interrupt_configure_dt(&dev_cfg->gpio_int1, GPIO_INT_DISABLE);
		(void)atomic_set(&drv_data->state, ICM4268X_STREAM_OFF);
		return;
	}

	/** FSR are fixed for high-resolution, at which point we should
	 * override driver FS config.
	 */
	uint8_t accel_fs_hr, gyro_fs_hr;

	switch (drv_data->cfg.variant) {
	case ICM4268X_VARIANT_ICM42688:
		accel_fs_hr = ICM42688_DT_ACCEL_FS_16;
		gyro_fs_hr = ICM42688_DT_GYRO_FS_2000;
		break;
	case ICM4268X_VARIANT_ICM42686:
		accel_fs_hr = ICM42686_DT_ACCEL_FS_32;
		gyro_fs_hr = ICM42686_DT_GYRO_FS_4000;
		break;
	default:
		CODE_UNREACHABLE;
	}
	/* Even if we flushed the fifo, we still need room for the header to return result info */
	size_t required_len = sizeof(struct icm4268x_fifo_data);

	rc = rtio_sqe_rx_buf(streaming_sqe, required_len, required_len, &buf, &buf_len);
	CHECKIF(rc < 0 || !buf) {
		LOG_ERR("%p Failed to obtain SQE buffer: %d", dev, rc);
		icm4268x_stream_result(dev, -ENOMEM);
		return;
	}

	struct icm4268x_fifo_data *edata = (struct icm4268x_fifo_data *)buf;
	struct icm4268x_fifo_data hdr = {
		.header = {
			.is_fifo = true,
			.variant = drv_data->cfg.variant,
			.gyro_fs = drv_data->cfg.fifo_hires ? gyro_fs_hr : drv_data->cfg.gyro_fs,
			.accel_fs = drv_data->cfg.fifo_hires ? accel_fs_hr : drv_data->cfg.accel_fs,
			.timestamp = drv_data->timestamp,
			.axis_align[0] = drv_data->cfg.axis_align[0],
			.axis_align[1] = drv_data->cfg.axis_align[1],
			.axis_align[2] = drv_data->cfg.axis_align[2]
		},
		.int_status = drv_data->int_status,
		.gyro_odr = drv_data->cfg.gyro_odr,
		.accel_odr = drv_data->cfg.accel_odr,
		.fifo_count = drv_data->cfg.fifo_wm,
		.rtc_freq = drv_data->cfg.rtc_freq,
	};
	*edata = hdr;

	if (FIELD_GET(BIT_FIFO_FULL_INT, edata->int_status) == true) {
		uint8_t val = BIT_FIFO_FLUSH;

		LOG_WRN("%p FIFO Full bit is set. Flushing FIFO...", dev);
		rc = icm4268x_prep_reg_write_rtio_async(&drv_data->bus, REG_SIGNAL_PATH_RESET,
							&val, 1, NULL);
		CHECKIF(rc < 0) {
			LOG_ERR("%p Failed to flush the FIFO buffer: %d", dev, rc);
			icm4268x_stream_result(dev, rc);
			return;
		}
		rtio_submit(drv_data->bus.rtio.ctx, 0);
	}

	struct rtio_cqe *cqe;

	do {
		cqe = rtio_cqe_consume(drv_data->bus.rtio.ctx);
		if (cqe != NULL) {
			if (rc >= 0) {
				rc = cqe->result;
			}
			rtio_cqe_release(drv_data->bus.rtio.ctx, cqe);
		}
	} while (cqe != NULL);

	(void)atomic_set(&drv_data->state, ICM4268X_STREAM_OFF);
	icm4268x_stream_result(dev, rc);
}

void icm4268x_fifo_event(const struct device *dev)
{
	struct icm4268x_dev_data *drv_data = dev->data;
	const struct icm4268x_dev_cfg *dev_cfg = (const struct icm4268x_dev_cfg *)dev->config;
	struct rtio_sqe *sqe;
	uint64_t cycles;
	int rc;

	if (drv_data->streaming_sqe == NULL ||
	    FIELD_GET(RTIO_SQE_CANCELED, drv_data->streaming_sqe->sqe.flags)) {
		LOG_ERR("%p FIFO event triggered with no stream submisssion. Disabling IRQ", dev);
		(void)gpio_pin_interrupt_configure_dt(&dev_cfg->gpio_int1, GPIO_INT_DISABLE);
		(void)atomic_set(&drv_data->state, ICM4268X_STREAM_OFF);
		return;
	}
	if (atomic_cas(&drv_data->state, ICM4268X_STREAM_ON, ICM4268X_STREAM_BUSY) == false) {
		LOG_WRN("%p Callback triggered while stream is busy. Ignoring request", dev);
		return;
	}

	rc = sensor_clock_get_cycles(&cycles);
	if (rc != 0) {
		LOG_ERR("%p Failed to get sensor clock cycles", dev);
		icm4268x_stream_result(dev, rc);
		return;
	}
	drv_data->timestamp = sensor_clock_cycles_to_ns(cycles);

	rc = icm4268x_prep_reg_read_rtio_async(&drv_data->bus, REG_INT_STATUS | REG_SPI_READ_BIT,
					       &drv_data->int_status, 1, &sqe);
	CHECKIF(rc < 0 || !sqe) {
		LOG_ERR("%p Could not prepare async read: %d", dev, rc);
		icm4268x_stream_result(dev, -ENOMEM);
		return;
	}
	sqe->flags |= RTIO_SQE_CHAINED;

	struct sensor_read_config *read_config =
		(struct sensor_read_config *)drv_data->streaming_sqe->sqe.iodev->data;
	struct sensor_stream_trigger *fifo_ths_cfg =
		icm4268x_get_read_config_trigger(read_config, SENSOR_TRIG_FIFO_WATERMARK);

	if (fifo_ths_cfg && fifo_ths_cfg->opt == SENSOR_STREAM_DATA_INCLUDE) {
		uint8_t *buf;
		uint32_t buf_len;
		uint16_t payload_read_len = drv_data->cfg.fifo_wm;
		size_t required_len = sizeof(struct icm4268x_fifo_data) + payload_read_len;

		rc = rtio_sqe_rx_buf(drv_data->streaming_sqe, required_len, required_len, &buf,
				     &buf_len);
		if (rc < 0) {
			LOG_ERR("%p Failed to allocate buffer for the FIFO read: %d", dev, rc);
			icm4268x_stream_result(dev, rc);
			return;
		}

		/** We fill we data first, the header we'll fill once we have
		 * read all the data.
		 */
		uint8_t *read_buf = buf + sizeof(struct icm4268x_fifo_data);

		rc = icm4268x_prep_reg_read_rtio_async(&drv_data->bus,
						       REG_FIFO_DATA | REG_SPI_READ_BIT,
						       read_buf, payload_read_len, &sqe);
		if (rc < 0 || !sqe) {
			LOG_ERR("%p Could not prepare async read: %d", dev, rc);
			icm4268x_stream_result(dev, -ENOMEM);
			return;
		}
		sqe->flags |= RTIO_SQE_CHAINED;
	} else {
		/** Because we don't want the data, flush it and be done with
		 * it. The trigger can be passed on to the user regardless.
		 */
		uint8_t val = BIT_FIFO_FLUSH;

		rc = icm4268x_prep_reg_write_rtio_async(&drv_data->bus, REG_SIGNAL_PATH_RESET,
							&val, 1, &sqe);
		if (rc < 0 || !sqe) {
			LOG_ERR("%p Could not prepare async read: %d", dev, rc);
			icm4268x_stream_result(dev, -ENOMEM);
			return;
		}
		sqe->flags |= RTIO_SQE_CHAINED;
	}

	struct rtio_sqe *cb_sqe = rtio_sqe_acquire(drv_data->bus.rtio.ctx);

	rtio_sqe_prep_callback(cb_sqe, icm4268x_complete_cb, (void *)dev, NULL);
	rtio_submit(drv_data->bus.rtio.ctx, 0);
}
