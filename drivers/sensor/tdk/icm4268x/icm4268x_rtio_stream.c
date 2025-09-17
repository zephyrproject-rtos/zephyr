/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor_clock.h>
#include "icm4268x.h"
#include "icm4268x_decoder.h"
#include "icm4268x_reg.h"
#include "icm4268x_rtio.h"

LOG_MODULE_DECLARE(ICM4268X_RTIO, CONFIG_SENSOR_LOG_LEVEL);

void icm4268x_submit_stream(const struct device *sensor, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
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
			LOG_ERR("Failed to configure sensor");
			rtio_iodev_sqe_err(iodev_sqe, rc);
			return;
		}
	}

	data->streaming_sqe = iodev_sqe;
}

static void icm4268x_complete_cb(struct rtio *r, const struct rtio_sqe *sqe, int result, void *arg)
{
	ARG_UNUSED(result);

	const struct device *dev = arg;
	struct icm4268x_dev_data *drv_data = dev->data;
	const struct icm4268x_dev_cfg *drv_cfg = dev->config;
	struct rtio_iodev_sqe *iodev_sqe = sqe->userdata;

	rtio_iodev_sqe_ok(iodev_sqe, drv_data->fifo_count);

	gpio_pin_interrupt_configure_dt(&drv_cfg->gpio_int1, GPIO_INT_EDGE_TO_ACTIVE);
}

static void icm4268x_fifo_count_cb(struct rtio *r, const struct rtio_sqe *sqe,
				   int result, void *arg)
{
	ARG_UNUSED(result);

	const struct device *dev = arg;
	struct icm4268x_dev_data *drv_data = dev->data;
	const struct icm4268x_dev_cfg *drv_cfg = dev->config;
	struct rtio_iodev *spi_iodev = drv_data->spi_iodev;
	uint8_t *fifo_count_buf = (uint8_t *)&drv_data->fifo_count;
	uint16_t fifo_count = ((fifo_count_buf[0] << 8) | fifo_count_buf[1]);

	drv_data->fifo_count = fifo_count;

	/* Pull a operation from our device iodev queue, validated to only be reads */
	struct rtio_iodev_sqe *iodev_sqe = drv_data->streaming_sqe;

	drv_data->streaming_sqe = NULL;

	/* Not inherently an underrun/overrun as we may have a buffer to fill next time */
	if (iodev_sqe == NULL) {
		LOG_DBG("No pending SQE");
		gpio_pin_interrupt_configure_dt(&drv_cfg->gpio_int1, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	const size_t packet_size = drv_data->cfg.fifo_hires ? 20 : 16;
	const size_t min_read_size = sizeof(struct icm4268x_fifo_data) + packet_size;
	const size_t ideal_read_size = sizeof(struct icm4268x_fifo_data) + fifo_count;
	uint8_t *buf;
	uint32_t buf_len;

	if (rtio_sqe_rx_buf(iodev_sqe, min_read_size, ideal_read_size, &buf, &buf_len) != 0) {
		LOG_ERR("Failed to get buffer");
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}
	LOG_DBG("Requesting buffer [%u, %u] got %u", (unsigned int)min_read_size,
		(unsigned int)ideal_read_size, buf_len);

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

	/* Read FIFO and call back to rtio with rtio_sqe completion */
	/* TODO is packet format even needed? the fifo has a header per packet
	 * already
	 */
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
		.rtc_freq = drv_data->cfg.rtc_freq,
	};
	uint32_t buf_avail = buf_len;

	memcpy(buf, &hdr, sizeof(hdr));
	buf_avail -= sizeof(hdr);

	uint32_t read_len = MIN(fifo_count, buf_avail);
	uint32_t pkts = read_len / packet_size;

	read_len = pkts * packet_size;
	((struct icm4268x_fifo_data *)buf)->fifo_count = read_len;

	__ASSERT_NO_MSG(read_len % packet_size == 0);

	uint8_t *read_buf = buf + sizeof(hdr);

	/* Flush out completions  */
	struct rtio_cqe *cqe;

	do {
		cqe = rtio_cqe_consume(r);
		if (cqe != NULL) {
			rtio_cqe_release(r, cqe);
		}
	} while (cqe != NULL);

	/* Setup new rtio chain to read the fifo data and report then check the
	 * result
	 */
	struct rtio_sqe *write_fifo_addr = rtio_sqe_acquire(r);
	__ASSERT_NO_MSG(write_fifo_addr != NULL);
	struct rtio_sqe *read_fifo_data = rtio_sqe_acquire(r);
	__ASSERT_NO_MSG(read_fifo_data != NULL);
	struct rtio_sqe *complete_op = rtio_sqe_acquire(r);
	__ASSERT_NO_MSG(complete_op != NULL);
	const uint8_t reg_addr = REG_SPI_READ_BIT | FIELD_GET(REG_ADDRESS_MASK, REG_FIFO_DATA);

	rtio_sqe_prep_tiny_write(write_fifo_addr, spi_iodev, RTIO_PRIO_NORM, &reg_addr, 1, NULL);
	write_fifo_addr->flags = RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_fifo_data, spi_iodev, RTIO_PRIO_NORM, read_buf, read_len,
			   iodev_sqe);
	read_fifo_data->flags = RTIO_SQE_CHAINED;
	rtio_sqe_prep_callback(complete_op, icm4268x_complete_cb, (void *)dev, iodev_sqe);

	rtio_submit(r, 0);
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

static void icm4268x_int_status_cb(struct rtio *r, const struct rtio_sqe *sqr,
				   int result, void *arg)
{
	ARG_UNUSED(result);

	const struct device *dev = arg;
	struct icm4268x_dev_data *drv_data = dev->data;
	const struct icm4268x_dev_cfg *drv_cfg = dev->config;
	struct rtio_iodev *spi_iodev = drv_data->spi_iodev;
	struct rtio_iodev_sqe *streaming_sqe = drv_data->streaming_sqe;
	struct sensor_read_config *read_config;

	if (streaming_sqe == NULL) {
		return;
	}

	read_config = (struct sensor_read_config *)streaming_sqe->sqe.iodev->data;
	__ASSERT_NO_MSG(read_config != NULL);

	if (!read_config->is_streaming) {
		/* Oops, not really configured for streaming data */
		return;
	}

	struct sensor_stream_trigger *fifo_ths_cfg =
		icm4268x_get_read_config_trigger(read_config, SENSOR_TRIG_FIFO_WATERMARK);
	bool has_fifo_ths_trig = fifo_ths_cfg != NULL &&
				 FIELD_GET(BIT_FIFO_THS_INT, drv_data->int_status) != 0;

	struct sensor_stream_trigger *fifo_full_cfg =
		icm4268x_get_read_config_trigger(read_config, SENSOR_TRIG_FIFO_FULL);
	bool has_fifo_full_trig = fifo_full_cfg != NULL &&
				  FIELD_GET(BIT_FIFO_FULL_INT, drv_data->int_status) != 0;

	if (!has_fifo_ths_trig && !has_fifo_full_trig) {
		LOG_DBG("No FIFO trigger is configured");
		gpio_pin_interrupt_configure_dt(&drv_cfg->gpio_int1, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	/* Flush completions */
	struct rtio_cqe *cqe;

	do {
		cqe = rtio_cqe_consume(r);
		if (cqe != NULL) {
			rtio_cqe_release(r, cqe);
		}
	} while (cqe != NULL);

	enum sensor_stream_data_opt data_opt;

	if (has_fifo_ths_trig && !has_fifo_full_trig) {
		/* Only care about fifo threshold */
		data_opt = fifo_ths_cfg->opt;
	} else if (!has_fifo_ths_trig && has_fifo_full_trig) {
		/* Only care about fifo full */
		data_opt = fifo_full_cfg->opt;
	} else {
		/* Both fifo threshold and full */
		data_opt = MIN(fifo_ths_cfg->opt, fifo_full_cfg->opt);
	}

	if (data_opt == SENSOR_STREAM_DATA_NOP || data_opt == SENSOR_STREAM_DATA_DROP) {
		uint8_t *buf;
		uint32_t buf_len;

		/* Clear streaming_sqe since we're done with the call */
		drv_data->streaming_sqe = NULL;
		if (rtio_sqe_rx_buf(streaming_sqe, sizeof(struct icm4268x_fifo_data),
				    sizeof(struct icm4268x_fifo_data), &buf, &buf_len) != 0) {
			rtio_iodev_sqe_err(streaming_sqe, -ENOMEM);
			return;
		}

		struct icm4268x_fifo_data *data = (struct icm4268x_fifo_data *)buf;

		memset(buf, 0, buf_len);
		data->header.timestamp = drv_data->timestamp;
		data->int_status = drv_data->int_status;
		data->fifo_count = 0;
		rtio_iodev_sqe_ok(streaming_sqe, 0);
		gpio_pin_interrupt_configure_dt(&drv_cfg->gpio_int1, GPIO_INT_EDGE_TO_ACTIVE);
		if (data_opt == SENSOR_STREAM_DATA_DROP) {
			/* Flush the FIFO */
			struct rtio_sqe *write_signal_path_reset = rtio_sqe_acquire(r);
			uint8_t write_buffer[] = {
				FIELD_GET(REG_ADDRESS_MASK, REG_SIGNAL_PATH_RESET),
				BIT_FIFO_FLUSH,
			};

			rtio_sqe_prep_tiny_write(write_signal_path_reset, spi_iodev, RTIO_PRIO_NORM,
						 write_buffer, ARRAY_SIZE(write_buffer), NULL);
			/* TODO Add a new flag for fire-and-forget so we don't have to block here */
			rtio_submit(r, 1);
			ARG_UNUSED(rtio_cqe_consume(r));
		}
		return;
	}

	/* We need the data, read the fifo length */
	struct rtio_sqe *write_fifo_count_reg = rtio_sqe_acquire(r);
	struct rtio_sqe *read_fifo_count = rtio_sqe_acquire(r);
	struct rtio_sqe *check_fifo_count = rtio_sqe_acquire(r);
	uint8_t reg = REG_SPI_READ_BIT | FIELD_GET(REG_ADDRESS_MASK, REG_FIFO_COUNTH);
	uint8_t *read_buf = (uint8_t *)&drv_data->fifo_count;

	rtio_sqe_prep_tiny_write(write_fifo_count_reg, spi_iodev, RTIO_PRIO_NORM, &reg, 1, NULL);
	write_fifo_count_reg->flags = RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_fifo_count, spi_iodev, RTIO_PRIO_NORM, read_buf, 2, NULL);
	read_fifo_count->flags = RTIO_SQE_CHAINED;
	rtio_sqe_prep_callback(check_fifo_count, icm4268x_fifo_count_cb, arg, NULL);

	rtio_submit(r, 0);
}

void icm4268x_fifo_event(const struct device *dev)
{
	struct icm4268x_dev_data *drv_data = dev->data;
	struct rtio_iodev *spi_iodev = drv_data->spi_iodev;
	struct rtio *r = drv_data->r;
	uint64_t cycles;
	int rc;

	if (drv_data->streaming_sqe == NULL) {
		return;
	}

	rc = sensor_clock_get_cycles(&cycles);
	if (rc != 0) {
		LOG_ERR("Failed to get sensor clock cycles");
		rtio_iodev_sqe_err(drv_data->streaming_sqe, rc);
		return;
	}

	drv_data->timestamp = sensor_clock_cycles_to_ns(cycles);

	/*
	 * Setup rtio chain of ops with inline calls to make decisions
	 * 1. read int status
	 * 2. call to check int status and get pending RX operation
	 * 4. read fifo len
	 * 5. call to determine read len
	 * 6. read fifo
	 * 7. call to report completion
	 */
	struct rtio_sqe *write_int_reg = rtio_sqe_acquire(r);
	struct rtio_sqe *read_int_reg = rtio_sqe_acquire(r);
	struct rtio_sqe *check_int_status = rtio_sqe_acquire(r);
	uint8_t reg = REG_SPI_READ_BIT | FIELD_GET(REG_ADDRESS_MASK, REG_INT_STATUS);

	rtio_sqe_prep_tiny_write(write_int_reg, spi_iodev, RTIO_PRIO_NORM, &reg, 1, NULL);
	write_int_reg->flags = RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_int_reg, spi_iodev, RTIO_PRIO_NORM, &drv_data->int_status, 1, NULL);
	read_int_reg->flags = RTIO_SQE_CHAINED;
	rtio_sqe_prep_callback(check_int_status, icm4268x_int_status_cb, (void *)dev, NULL);
	rtio_submit(r, 0);
}
