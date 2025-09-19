/* ST Microelectronics LIS2DUX12 6-axis IMU sensor driver
 *
 * Copyright (c) 2023 Google LLC
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/dt-bindings/sensor/lis2dux12.h>
#include <zephyr/drivers/sensor.h>
#include "lis2dux12.h"
#include "lis2dux12_decoder.h"
#include <zephyr/rtio/work.h>
#include <zephyr/drivers/sensor_clock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(LIS2DUX12_RTIO);

/*
 * Create a chain of SQEs representing a bus transaction to read a reg.
 * The RTIO-enabled bus driver will:
 *
 *     - write "reg" address
 *     - read "len" data bytes into "buf".
 *     - call complete_op callback
 *
 * If drdy_xl is active it reads XL data (6 bytes) from LIS2DUX12_OUTX_L_A reg.
 */
static void lis2dux12_rtio_rw_transaction(const struct device *dev, uint8_t reg,
					   uint8_t *buf, uint32_t len,
					   rtio_callback_t complete_op_cb)
{
	struct lis2dux12_data *lis2dux12 = dev->data;
	struct rtio *rtio = lis2dux12->rtio_ctx;
	struct rtio_iodev *iodev = lis2dux12->iodev;
	struct rtio_sqe *write_addr = rtio_sqe_acquire(rtio);
	struct rtio_sqe *read_reg = rtio_sqe_acquire(rtio);
	struct rtio_sqe *complete_op = rtio_sqe_acquire(rtio);
	struct rtio_iodev_sqe *sqe = lis2dux12->streaming_sqe;
	uint8_t reg_bus = lis2dux12_bus_reg(lis2dux12, reg);

	/* check we have been able to acquire sqe */
	if (write_addr == NULL || read_reg == NULL || complete_op == NULL) {
		return;
	}

	rtio_sqe_prep_tiny_write(write_addr, iodev, RTIO_PRIO_NORM, &reg_bus, 1, NULL);
	write_addr->flags = RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_reg, iodev, RTIO_PRIO_NORM, buf, len, NULL);
	read_reg->flags = RTIO_SQE_CHAINED;
	if (lis2dux12->bus_type == BUS_I2C) {
		read_reg->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
	}

	rtio_sqe_prep_callback_no_cqe(complete_op, complete_op_cb, (void *)dev, sqe);
	rtio_submit(rtio, 0);
}

void lis2dux12_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct lis2dux12_data *lis2dux12 = dev->data;
	const struct lis2dux12_config *config = dev->config;
	const struct lis2dux12_chip_api *chip_api = config->chip_api;
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	struct trigger_config trig_cfg = { 0 };

	gpio_pin_interrupt_configure_dt(lis2dux12->drdy_gpio, GPIO_INT_DISABLE);

	for (size_t i = 0; i < cfg->count; i++) {
		if (cfg->triggers[i].trigger == SENSOR_TRIG_FIFO_WATERMARK) {
			trig_cfg.int_fifo_th = 1;
		} else if (cfg->triggers[i].trigger == SENSOR_TRIG_FIFO_FULL) {
			trig_cfg.int_fifo_full = 1;
		} else if (cfg->triggers[i].trigger == SENSOR_TRIG_DATA_READY) {
			trig_cfg.int_drdy = 1;
		}
	}

	/* if any change in trig_cfg for FIFO triggers */
	if (trig_cfg.int_fifo_th != lis2dux12->trig_cfg.int_fifo_th ||
	    trig_cfg.int_fifo_full != lis2dux12->trig_cfg.int_fifo_full) {
		lis2dux12->trig_cfg.int_fifo_th = trig_cfg.int_fifo_th;
		lis2dux12->trig_cfg.int_fifo_full = trig_cfg.int_fifo_full;

		/* enable/disable the FIFO */
		chip_api->stream_config_fifo(dev, trig_cfg);
	}

	/* if any change in trig_cfg for DRDY triggers */
	if (trig_cfg.int_drdy != lis2dux12->trig_cfg.int_drdy) {
		lis2dux12->trig_cfg.int_drdy = trig_cfg.int_drdy;

		/* enable/disable drdy events */
		chip_api->stream_config_drdy(dev, trig_cfg);
	}

	lis2dux12->streaming_sqe = iodev_sqe;

	gpio_pin_interrupt_configure_dt(lis2dux12->drdy_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}

/*
 * Called by bus driver to complete the sqe.
 */
static void lis2dux12_complete_op_cb(struct rtio *r, const struct rtio_sqe *sqe,
				     int result, void *arg)
{
	ARG_UNUSED(result);

	const struct device *dev = arg;
	struct lis2dux12_data *lis2dux12 = dev->data;

	/*
	 * Mark operation completed
	 */
	rtio_iodev_sqe_ok(sqe->userdata, 0);
	lis2dux12->streaming_sqe = NULL;
	gpio_pin_interrupt_configure_dt(lis2dux12->drdy_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}

/*
 * Called by bus driver to complete the LIS2DUX12_FIFO_STATUS read op (2 bytes).
 * If FIFO threshold or FIFO full events are active it reads all FIFO entries.
 */
static void lis2dux12_read_fifo_cb(struct rtio *r, const struct rtio_sqe *sqe,
				   int result, void *arg)
{
	ARG_UNUSED(result);

	const struct device *dev = arg;
	struct lis2dux12_data *lis2dux12 = dev->data;
	const struct lis2dux12_config *cfg = dev->config;
	struct rtio *rtio = lis2dux12->rtio_ctx;
	struct gpio_dt_spec *irq_gpio = lis2dux12->drdy_gpio;
	struct rtio_iodev *iodev = lis2dux12->iodev;
	struct sensor_read_config *read_config;
	uint8_t fifo_th = 0, fifo_full = 0;
	uint16_t fifo_count;

	/* At this point, no sqe request is queued should be considered as a bug */
	__ASSERT_NO_MSG(lis2dux12->streaming_sqe != NULL);

	read_config = (struct sensor_read_config *)lis2dux12->streaming_sqe->sqe.iodev->data;
	__ASSERT_NO_MSG(read_config != NULL);
	__ASSERT_NO_MSG(read_config->is_streaming == true);

	/* parse the configuration in search for any configured trigger */
	struct sensor_stream_trigger *fifo_ths_cfg = NULL;
	struct sensor_stream_trigger *fifo_full_cfg = NULL;

	for (int i = 0; i < read_config->count; ++i) {
		if (read_config->triggers[i].trigger == SENSOR_TRIG_FIFO_WATERMARK) {
			fifo_ths_cfg = &read_config->triggers[i];
			continue;
		}

		if (read_config->triggers[i].trigger == SENSOR_TRIG_FIFO_FULL) {
			fifo_full_cfg = &read_config->triggers[i];
			continue;
		}
	}

	/* fill fifo h/w status */
	fifo_th = (lis2dux12->fifo_status[0] & 0x80) ? 1 : 0;
	fifo_full = (lis2dux12->fifo_status[0] & 0x40) ? 1 : 0;
	fifo_count = (uint16_t)lis2dux12->fifo_status[1];
	lis2dux12->fifo_count = fifo_count;

	bool has_fifo_ths_trig = fifo_ths_cfg != NULL && fifo_th == 1;
	bool has_fifo_full_trig = fifo_full_cfg != NULL && fifo_full == 1;

	/* check if no theshold/full fifo interrupt or spurious interrupts */
	if (!has_fifo_ths_trig && !has_fifo_full_trig) {
		/* complete operation with no error */
		rtio_iodev_sqe_ok(sqe->userdata, 0);

		lis2dux12->streaming_sqe = NULL;
		gpio_pin_interrupt_configure_dt(irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	/* flush completion */
	struct rtio_cqe *cqe;
	int res = 0;

	do {
		cqe = rtio_cqe_consume(rtio);
		if (cqe != NULL) {
			if ((cqe->result < 0) && (res == 0)) {
				LOG_ERR("Bus error: %d", cqe->result);
				res = cqe->result;
			}
			rtio_cqe_release(rtio, cqe);
		}
	} while (cqe != NULL);

	/* Bail/cancel attempt to read sensor on any error */
	if (res != 0) {
		rtio_iodev_sqe_err(lis2dux12->streaming_sqe, res);
		lis2dux12->streaming_sqe = NULL;
		gpio_pin_interrupt_configure_dt(irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

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
		if (rtio_sqe_rx_buf(lis2dux12->streaming_sqe, sizeof(struct lis2dux12_fifo_data),
				    sizeof(struct lis2dux12_fifo_data), &buf, &buf_len) != 0) {
			rtio_iodev_sqe_err(lis2dux12->streaming_sqe, -ENOMEM);
			lis2dux12->streaming_sqe = NULL;
			gpio_pin_interrupt_configure_dt(irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
			return;
		}

		struct lis2dux12_fifo_data *rx_data = (struct lis2dux12_fifo_data *)buf;

		memset(buf, 0, buf_len);
		rx_data->header.is_fifo = 1;
		rx_data->header.timestamp = lis2dux12->timestamp;
		rx_data->header.int_status = lis2dux12->fifo_status[0];
		rx_data->fifo_count = 0;
		rx_data->fifo_mode_sel = 0;

		/* complete request with ok */
		rtio_iodev_sqe_ok(lis2dux12->streaming_sqe, 0);
		lis2dux12->streaming_sqe = NULL;
		gpio_pin_interrupt_configure_dt(irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);

		if (data_opt == SENSOR_STREAM_DATA_DROP) {

			/*
			 * Flush the FIFO by setting the mode to LIS2DUX12_BYPASS_MODE
			 *
			 * STMEMSC API equivalent code:
			 *
			 *   lis2dux12_fifo_mode_set(ctx, LIS2DUX12_BYPASS_MODE);
			 */
			struct rtio_sqe *write_fifo_mode = rtio_sqe_acquire(rtio);
			uint8_t lis2dux12_fifo_mode_set[] = {
				LIS2DUXXX_DT_FIFO_CTRL,
				LIS2DUXXX_DT_BYPASS_MODE,
			};

			write_fifo_mode->flags |= RTIO_SQE_NO_RESPONSE;
			rtio_sqe_prep_tiny_write(write_fifo_mode, iodev,
						 RTIO_PRIO_NORM, lis2dux12_fifo_mode_set,
						 ARRAY_SIZE(lis2dux12_fifo_mode_set), NULL);

			rtio_submit(rtio, 0);
		}

		return;
	}

	uint8_t *buf, *read_buf;
	uint32_t buf_len, buf_avail;
	uint32_t req_len = LIS2DUX12_FIFO_SIZE(fifo_count) + sizeof(struct lis2dux12_fifo_data);

	if (rtio_sqe_rx_buf(lis2dux12->streaming_sqe, req_len, req_len, &buf, &buf_len) != 0) {
		LOG_ERR("Failed to get buffer");
		rtio_iodev_sqe_err(lis2dux12->streaming_sqe, -ENOMEM);
		lis2dux12->streaming_sqe = NULL;
		gpio_pin_interrupt_configure_dt(irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	/* clang-format off */
	struct lis2dux12_fifo_data hdr = {
		.header = {
			.is_fifo = 1,
			.range = lis2dux12->range,
			.timestamp = lis2dux12->timestamp,
			.int_status = lis2dux12->fifo_status[0],
		},
		.fifo_count = fifo_count,
		.fifo_mode_sel = cfg->fifo_mode_sel,
		.accel_batch_odr = lis2dux12->accel_batch_odr,
		.accel_odr = lis2dux12->odr,
	};
	/* clang-format on */

	memcpy(buf, &hdr, sizeof(hdr));
	read_buf = buf + sizeof(hdr);
	buf_avail = buf_len - sizeof(hdr);

	/*
	 * Prepare rtio enabled bus to read all fifo_count entries from
	 * LIS2DUX12_FIFO_DATA_OUT_TAG.  Then lis2dux12_complete_op_cb
	 * callback will be invoked.
	 *
	 * STMEMSC API equivalent code:
	 *
	 *   num = fifo_status.fifo_level;
	 *
	 *   while (num--) {
	 *     lis2dux12_fifo_out_raw_t f_data;
	 *
	 *     lis2dux12_fifo_out_raw_get(&dev_ctx, &f_data);
	 *   }
	 */
	lis2dux12_rtio_rw_transaction(dev, LIS2DUXXX_DT_FIFO_DATA_OUT_TAG,
				       read_buf, buf_avail, lis2dux12_complete_op_cb);
}

/*
 * Called by bus driver to complete the LIS2DUX12_STATUS_REG read op.
 * If drdy_xl is active it reads XL data (6 bytes) from LIS2DUX12_OUTX_L_A reg.
 */
static void lis2dux12_read_status_cb(struct rtio *r, const struct rtio_sqe *sqe,
				     int result, void *arg)
{
	ARG_UNUSED(result);

	const struct device *dev = arg;
	struct lis2dux12_data *lis2dux12 = dev->data;
	struct rtio *rtio = lis2dux12->rtio_ctx;
	struct gpio_dt_spec *irq_gpio = lis2dux12->drdy_gpio;
	struct sensor_read_config *read_config;

	/* At this point, no sqe request is queued should be considered as a bug */
	__ASSERT_NO_MSG(lis2dux12->streaming_sqe != NULL);

	read_config = (struct sensor_read_config *)lis2dux12->streaming_sqe->sqe.iodev->data;
	__ASSERT_NO_MSG(read_config != NULL);
	__ASSERT_NO_MSG(read_config->is_streaming == true);

	/* parse the configuration in search for any configured trigger */
	struct sensor_stream_trigger *data_ready = NULL;

	for (int i = 0; i < read_config->count; ++i) {
		if (read_config->triggers[i].trigger == SENSOR_TRIG_DATA_READY) {
			data_ready = &read_config->triggers[i];
			break;
		}
	}

	/* flush completion */
	struct rtio_cqe *cqe;
	int res = 0;

	do {
		cqe = rtio_cqe_consume(rtio);
		if (cqe != NULL) {
			if ((cqe->result < 0) && (res == 0)) {
				LOG_ERR("Bus error: %d", cqe->result);
				res = cqe->result;
			}
			rtio_cqe_release(rtio, cqe);
		}
	} while (cqe != NULL);

	/* Bail/cancel attempt to read sensor on any error */
	if (res != 0) {
		rtio_iodev_sqe_err(lis2dux12->streaming_sqe, res);
		lis2dux12->streaming_sqe = NULL;
		return;
	}

	if (data_ready != NULL &&
	    (data_ready->opt == SENSOR_STREAM_DATA_NOP ||
	     data_ready->opt == SENSOR_STREAM_DATA_DROP)) {
		uint8_t *buf;
		uint32_t buf_len;

		/* Clear streaming_sqe since we're done with the call */
		if (rtio_sqe_rx_buf(lis2dux12->streaming_sqe, sizeof(struct lis2dux12_rtio_data),
				    sizeof(struct lis2dux12_rtio_data), &buf, &buf_len) != 0) {
			rtio_iodev_sqe_err(lis2dux12->streaming_sqe, -ENOMEM);
			lis2dux12->streaming_sqe = NULL;
			gpio_pin_interrupt_configure_dt(irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
			return;
		}

		struct lis2dux12_rtio_data *rx_data = (struct lis2dux12_rtio_data *)buf;

		memset(buf, 0, buf_len);
		rx_data->header.is_fifo = 0;
		rx_data->header.timestamp = lis2dux12->timestamp;
		rx_data->has_accel = 0;
		rx_data->has_temp = 0;

		/* complete request with ok */
		rtio_iodev_sqe_ok(lis2dux12->streaming_sqe, 0);
		lis2dux12->streaming_sqe = NULL;
		gpio_pin_interrupt_configure_dt(irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);

		return;
	}

	/*
	 * Read XL data
	 *
	 * lis2dux12_data_ready_t drdy;
	 * if (drdy.drdy_xl) {
	 */
	if (lis2dux12->status & 0x1) {
		uint8_t *buf, *read_buf;
		uint32_t buf_len;
		uint32_t req_len = 6 + sizeof(struct lis2dux12_rtio_data);

		if (rtio_sqe_rx_buf(lis2dux12->streaming_sqe,
				    req_len, req_len, &buf, &buf_len) != 0) {
			LOG_ERR("Failed to get buffer");
			rtio_iodev_sqe_err(lis2dux12->streaming_sqe, -ENOMEM);
			lis2dux12->streaming_sqe = NULL;
			gpio_pin_interrupt_configure_dt(irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
			return;
		}

		/* clang-format off */
		struct lis2dux12_rtio_data hdr = {
			.header = {
				.is_fifo = 0,
				.range = lis2dux12->range,
				.timestamp = lis2dux12->timestamp,
				.int_status = lis2dux12->status,
			},
			.has_accel = 1,
			.has_temp = 0,
		};
		/* clang-format on */

		memcpy(buf, &hdr, sizeof(hdr));
		read_buf = (uint8_t *)&((struct lis2dux12_rtio_data *)buf)->acc[0];

		/*
		 * Prepare rtio enabled bus to read LIS2DUX12_OUTX_L_A register
		 * where accelerometer data is available.
		 * Then lis2dux12_complete_op_cb callback will be invoked.
		 *
		 * STMEMSC API equivalent code:
		 *
		 *   uint8_t accel_raw[6];
		 *
		 *   lis2dux12_acceleration_raw_get(&dev_ctx, accel_raw);
		 */
		lis2dux12_rtio_rw_transaction(dev, LIS2DUXXX_DT_OUTX_L,
					       read_buf, 6, lis2dux12_complete_op_cb);
	}
}

/*
 * Called when one of the following trigger is active:
 *
 *     - int_fifo_th (SENSOR_TRIG_FIFO_WATERMARK)
 *     - int_fifo_full (SENSOR_TRIG_FIFO_FULL)
 *     - int_drdy (SENSOR_TRIG_DATA_READY)
 */
void lis2dux12_stream_irq_handler(const struct device *dev)
{
	struct lis2dux12_data *lis2dux12 = dev->data;
	uint64_t cycles;
	int rc;

	if (lis2dux12->streaming_sqe == NULL) {
		return;
	}

	rc = sensor_clock_get_cycles(&cycles);
	if (rc != 0) {
		LOG_ERR("Failed to get sensor clock cycles");
		rtio_iodev_sqe_err(lis2dux12->streaming_sqe, rc);
		return;
	}

	/* get timestamp as soon as the irq is served */
	lis2dux12->timestamp = sensor_clock_cycles_to_ns(cycles);

	/* handle FIFO triggers */
	if (lis2dux12->trig_cfg.int_fifo_th || lis2dux12->trig_cfg.int_fifo_full) {
		lis2dux12->fifo_status[0] = lis2dux12->fifo_status[1] = 0;

		/*
		 * Prepare rtio enabled bus to read LIS2DUX12_FIFO_STATUS1 and
		 * LIS2DUX12_FIFO_STATUS2 registers where FIFO threshold condition and
		 * count are reported. Then lis2dux12_read_fifo_cb callback will be
		 * invoked.
		 *
		 * STMEMSC API equivalent code:
		 *
		 *   uint8_t wmflag = 0;
		 *   lis2duxs12_fifo_wtm_flag_get(&dev_ctx, &wmflag);
		 *
		 *   uint16_t num;
		 *   lis2duxs12_fifo_data_level_get(&dev_ctx, &num);
		 */
		lis2dux12_rtio_rw_transaction(dev, LIS2DUXXX_DT_FIFO_STATUS1,
				lis2dux12->fifo_status, 2, lis2dux12_read_fifo_cb);
	}

	/* handle drdy trigger */
	if (lis2dux12->trig_cfg.int_drdy) {
		lis2dux12->status = 0;

		/*
		 * Prepare rtio enabled bus to read LIS2DUX12_STATUS_REG register
		 * where accelerometer and gyroscope data ready status is available.
		 * Then lis2dux12_read_status_cb callback will be invoked.
		 *
		 * STMEMSC API equivalent code:
		 *
		 *   lis2dux12_data_ready_t drdy;
		 *
		 *   lis2dux12_flag_data_ready_get(&dev_ctx, &drdy);
		 */
		lis2dux12_rtio_rw_transaction(dev, LIS2DUXXX_DT_STATUS,
					       &lis2dux12->status, 1, lis2dux12_read_status_cb);
	}
}
