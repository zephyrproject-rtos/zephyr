/* ST Microelectronics IIS3DWB accelerometer senor
 *
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis3dwb.pdf
 */

#include <zephyr/dt-bindings/sensor/iis3dwb.h>
#include <zephyr/drivers/sensor.h>
#include "iis3dwb.h"
#include <zephyr/rtio/work.h>
#include <zephyr/drivers/sensor_clock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(IIS3DWB);

static void iis3dwb_config_fifo(const struct device *dev, struct trigger_config trig_cfg)
{
	struct iis3dwb_data *iis3dwb = dev->data;
	const struct iis3dwb_config *config = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&config->ctx;

	/* disable FIFO as first thing */
	iis3dwb_fifo_watermark_set(ctx, 0);
	iis3dwb_fifo_xl_batch_set(ctx, IIS3DWB_DT_XL_NOT_BATCHED);
	iis3dwb_fifo_temp_batch_set(ctx, IIS3DWB_DT_TEMP_NOT_BATCHED);
	iis3dwb_fifo_timestamp_batch_set(ctx, IIS3DWB_DT_TS_NOT_BATCHED);
	iis3dwb_fifo_mode_set(ctx, IIS3DWB_BYPASS_MODE);

	if (trig_cfg.int_fifo_th || trig_cfg.int_fifo_full) {
		iis3dwb_fifo_watermark_set(ctx, config->fifo_wtm);
		iis3dwb_fifo_xl_batch_set(ctx, config->accel_batch);
		iis3dwb_fifo_temp_batch_set(ctx, config->temp_batch);
		iis3dwb_fifo_timestamp_batch_set(ctx, config->ts_batch);

		iis3dwb->accel_batch_odr = config->accel_batch;
		iis3dwb->temp_batch_odr = config->temp_batch;
		iis3dwb->ts_batch_odr = config->ts_batch;

		iis3dwb_fifo_mode_set(ctx, IIS3DWB_STREAM_MODE);
	}
}

void iis3dwb_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct iis3dwb_data *iis3dwb = dev->data;
	const struct iis3dwb_config *config = dev->config;
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	struct trigger_config trig_cfg = {0};
	bool cfg_changed = 0;

	gpio_pin_interrupt_configure_dt(iis3dwb->drdy_gpio, GPIO_INT_DISABLE);

	for (size_t i = 0; i < cfg->count; i++) {
		switch (cfg->triggers[i].trigger) {
		case SENSOR_TRIG_FIFO_WATERMARK:
			trig_cfg.int_fifo_th = 1;
			break;
		case SENSOR_TRIG_FIFO_FULL:
			trig_cfg.int_fifo_full = 1;
			break;
		case SENSOR_TRIG_DATA_READY:
			trig_cfg.int_drdy = 1;
			break;
		default:
			/* Unhandled trigger */
			break;
		}
	}

	/* if any change in trig_cfg for FIFO triggers */
	if (trig_cfg.int_fifo_th != iis3dwb->trig_cfg.int_fifo_th ||
	    trig_cfg.int_fifo_full != iis3dwb->trig_cfg.int_fifo_full) {
		iis3dwb->trig_cfg.int_fifo_th = trig_cfg.int_fifo_th;
		iis3dwb->trig_cfg.int_fifo_full = trig_cfg.int_fifo_full;

		/* enable/disable the FIFO */
		iis3dwb_config_fifo(dev, trig_cfg);
		cfg_changed = 1;
	}

	/* if any change in trig_cfg for DRDY triggers */
	if (trig_cfg.int_drdy != iis3dwb->trig_cfg.int_drdy) {
		iis3dwb->trig_cfg.int_drdy = trig_cfg.int_drdy;
		cfg_changed = 1;
	}

	if (!cfg_changed) {
		/* skip drdy pin routing */
		goto done;
	}

	/* Set pin interrupt */
	if (config->drdy_pin == 1) {
		iis3dwb_pin_int1_route_t pin_int = {0};

		pin_int.fifo_th = (trig_cfg.int_fifo_th) ? 1 : 0;
		pin_int.fifo_full = (trig_cfg.int_fifo_full) ? 1 : 0;
		pin_int.drdy_xl = (trig_cfg.int_drdy) ? 1 : 0;
		iis3dwb_route_int1(dev, pin_int);
	} else if (config->drdy_pin == 2) {
		iis3dwb_pin_int2_route_t pin_int = {0};

		pin_int.fifo_th = (trig_cfg.int_fifo_th) ? 1 : 0;
		pin_int.fifo_full = (trig_cfg.int_fifo_full) ? 1 : 0;
		pin_int.drdy_xl = (trig_cfg.int_drdy) ? 1 : 0;
		iis3dwb_route_int2(dev, pin_int);
	} else {
		LOG_ERR("Bad drdy pin number");
		return;
	}

done:
	iis3dwb->streaming_sqe = iodev_sqe;

	gpio_pin_interrupt_configure_dt(iis3dwb->drdy_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}

/*
 * Called by bus driver to complete the sqe.
 */
static void iis3dwb_complete_op_cb(struct rtio *r, const struct rtio_sqe *sqe, int result,
				   void *arg)
{
	const struct device *dev = arg;
	struct iis3dwb_data *iis3dwb = dev->data;

	ARG_UNUSED(result);

	/*
	 * Mark operation completed
	 */
	iis3dwb->streaming_sqe = NULL;
	rtio_iodev_sqe_ok(sqe->userdata, 0);
	gpio_pin_interrupt_configure_dt(iis3dwb->drdy_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}

/*
 * Called by bus driver to complete the IIS3DWB_FIFO_STATUS read op (2 bytes).
 * If FIFO threshold or FIFO full events are active it reads all FIFO entries.
 */
static void iis3dwb_read_fifo_cb(struct rtio *r, const struct rtio_sqe *sqe, int result, void *arg)
{
	const struct device *dev = arg;
	struct iis3dwb_data *iis3dwb = dev->data;
	struct rtio *rtio = iis3dwb->rtio_ctx;
	const struct gpio_dt_spec *irq_gpio = iis3dwb->drdy_gpio;
	struct rtio_iodev *iodev = iis3dwb->iodev;
	struct sensor_read_config *read_config;
	uint8_t fifo_th = 0, fifo_full = 0;
	uint16_t fifo_count;

	ARG_UNUSED(result);

	/* At this point, no sqe request is queued should be considered as a bug */
	__ASSERT_NO_MSG(iis3dwb->streaming_sqe != NULL);

	read_config = (struct sensor_read_config *)iis3dwb->streaming_sqe->sqe.iodev->data;
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
	fifo_th = (iis3dwb->fifo_status[1] & 0x80) ? 1 : 0;
	fifo_full = (iis3dwb->fifo_status[1] & 0x20) ? 1 : 0;
	fifo_count = (uint16_t)(iis3dwb->fifo_status[0] | (iis3dwb->fifo_status[1] & 0x3) << 8);
	iis3dwb->fifo_count = fifo_count;

	bool has_fifo_ths_trig = fifo_ths_cfg != NULL && fifo_th == 1;
	bool has_fifo_full_trig = fifo_full_cfg != NULL && fifo_full == 1;

	/* check if no theshold/full fifo interrupt or spurious interrupts */
	if (!has_fifo_ths_trig && !has_fifo_full_trig) {
		/* complete operation with no error */
		rtio_iodev_sqe_ok(sqe->userdata, 0);

		iis3dwb->streaming_sqe = NULL;
		gpio_pin_interrupt_configure_dt(irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	/* flush completion */
	int res = 0;

	res = rtio_flush_completion_queue(rtio);

	/* Bail/cancel attempt to read sensor on any error */
	if (res != 0) {
		rtio_iodev_sqe_err(iis3dwb->streaming_sqe, res);
		iis3dwb->streaming_sqe = NULL;
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
		if (rtio_sqe_rx_buf(iis3dwb->streaming_sqe, sizeof(struct iis3dwb_fifo_data),
				    sizeof(struct iis3dwb_fifo_data), &buf, &buf_len) != 0) {
			rtio_iodev_sqe_err(iis3dwb->streaming_sqe, -ENOMEM);
			iis3dwb->streaming_sqe = NULL;
			gpio_pin_interrupt_configure_dt(irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
			return;
		}

		struct iis3dwb_fifo_data *rx_data = (struct iis3dwb_fifo_data *)buf;

		memset(buf, 0, buf_len);
		rx_data->header.is_fifo = 1;
		rx_data->header.timestamp = iis3dwb->timestamp;
		rx_data->header.int_status = iis3dwb->fifo_status[0];
		rx_data->fifo_count = 0;
		rx_data->fifo_mode_sel = 0;

		/* complete request with ok */
		rtio_iodev_sqe_ok(iis3dwb->streaming_sqe, 0);
		iis3dwb->streaming_sqe = NULL;
		gpio_pin_interrupt_configure_dt(irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);

		if (data_opt == SENSOR_STREAM_DATA_DROP) {

			/*
			 * Flush the FIFO by setting the mode to IIS3DWB_BYPASS_MODE
			 *
			 * STMEMSC API equivalent code:
			 *
			 *   iis3dwb_fifo_mode_set(ctx, IIS3DWB_BYPASS_MODE);
			 */
			struct rtio_sqe *write_fifo_mode = rtio_sqe_acquire(rtio);
			uint8_t iis3dwb_fifo_mode_set[] = {
				IIS3DWB_FIFO_CTRL4,
				IIS3DWB_BYPASS_MODE,
			};

			write_fifo_mode->flags |= RTIO_SQE_NO_RESPONSE;
			rtio_sqe_prep_tiny_write(write_fifo_mode, iodev, RTIO_PRIO_NORM,
						 iis3dwb_fifo_mode_set,
						 ARRAY_SIZE(iis3dwb_fifo_mode_set), NULL);

			rtio_submit(rtio, 0);
		}

		return;
	}

	uint8_t *buf, *read_buf;
	uint32_t buf_len, buf_avail;
	uint32_t req_len = IIS3DWB_FIFO_SIZE(fifo_count) + sizeof(struct iis3dwb_fifo_data);

	if (rtio_sqe_rx_buf(iis3dwb->streaming_sqe, req_len, req_len, &buf, &buf_len) != 0) {
		LOG_ERR("Failed to get buffer");
		rtio_iodev_sqe_err(iis3dwb->streaming_sqe, -ENOMEM);
		iis3dwb->streaming_sqe = NULL;
		gpio_pin_interrupt_configure_dt(irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	/* clang-format off */
	struct iis3dwb_fifo_data hdr = {
		.header = {
			.is_fifo = 1,
			.range = iis3dwb->range,
			.timestamp = iis3dwb->timestamp,
			.int_status = iis3dwb->fifo_status[0],
		},
		.fifo_count = fifo_count,
		.accel_batch_odr = iis3dwb->accel_batch_odr,
		.accel_odr = iis3dwb->odr,
	};
	/* clang-format on */

	memcpy(buf, &hdr, sizeof(hdr));
	read_buf = buf + sizeof(hdr);
	buf_avail = buf_len - sizeof(hdr);

	struct rtio_regs fifo_regs;
	struct rtio_regs_list regs_list[] = {
		{
			0x80 | IIS3DWB_FIFO_DATA_OUT_TAG, /* mark the SPI read transaction */
			read_buf,
			buf_avail,
		},
	};

	fifo_regs.rtio_regs_list = regs_list;
	fifo_regs.rtio_regs_num = ARRAY_SIZE(regs_list);

	/*
	 * Prepare rtio enabled bus to read all fifo_count entries from
	 * IIS3DWB_FIFO_DATA_OUT_TAG.  Then iis3dwb_complete_op_cb
	 * callback will be invoked.
	 *
	 * STMEMSC API equivalent code:
	 *
	 *   num = fifo_status.fifo_level;
	 *
	 *   iis3dwb_fifo_out_raw_t f_data;
	 *
	 *   iis3dwb_fifo_out_multi_raw_get(&dev_ctx, &f_data, num);
	 *   }
	 */
	rtio_read_regs_async(iis3dwb->rtio_ctx, iis3dwb->iodev, RTIO_BUS_SPI, &fifo_regs,
			     iis3dwb->streaming_sqe, dev, iis3dwb_complete_op_cb);
}

/*
 * Called by bus driver to complete the IIS3DWB_STATUS_REG read op.
 * If drdy_xl is active it reads XL data (6 bytes) from IIS3DWB_OUTX_L_A reg.
 */
static void iis3dwb_read_status_cb(struct rtio *r, const struct rtio_sqe *sqe, int result,
				   void *arg)
{
	const struct device *dev = arg;
	struct iis3dwb_data *iis3dwb = dev->data;
	struct rtio *rtio = iis3dwb->rtio_ctx;
	const struct gpio_dt_spec *irq_gpio = iis3dwb->drdy_gpio;
	struct sensor_read_config *read_config;

	ARG_UNUSED(result);

	/* At this point, no sqe request is queued should be considered as a bug */
	__ASSERT_NO_MSG(iis3dwb->streaming_sqe != NULL);

	read_config = (struct sensor_read_config *)iis3dwb->streaming_sqe->sqe.iodev->data;
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
	int res = 0;

	res = rtio_flush_completion_queue(rtio);

	/* Bail/cancel attempt to read sensor on any error */
	if (res != 0) {
		rtio_iodev_sqe_err(iis3dwb->streaming_sqe, res);
		iis3dwb->streaming_sqe = NULL;
		return;
	}

	if (data_ready->opt == SENSOR_STREAM_DATA_NOP ||
	    data_ready->opt == SENSOR_STREAM_DATA_DROP) {
		uint8_t *buf;
		uint32_t buf_len;

		/* Clear streaming_sqe since we're done with the call */
		if (rtio_sqe_rx_buf(iis3dwb->streaming_sqe, sizeof(struct iis3dwb_rtio_data),
				    sizeof(struct iis3dwb_rtio_data), &buf, &buf_len) != 0) {
			rtio_iodev_sqe_err(iis3dwb->streaming_sqe, -ENOMEM);
			iis3dwb->streaming_sqe = NULL;
			gpio_pin_interrupt_configure_dt(irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
			return;
		}

		struct iis3dwb_rtio_data *rx_data = (struct iis3dwb_rtio_data *)buf;

		memset(buf, 0, buf_len);
		rx_data->header.is_fifo = 0;
		rx_data->header.timestamp = iis3dwb->timestamp;
		rx_data->has_accel = 0;
		rx_data->has_temp = 0;

		/* complete request with ok */
		rtio_iodev_sqe_ok(iis3dwb->streaming_sqe, 0);
		iis3dwb->streaming_sqe = NULL;
		gpio_pin_interrupt_configure_dt(irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	}

	/*
	 * Read XL data
	 *
	 * iis3dwb_status_reg_t val;
	 * if (val.xlda) {
	 */
	if (iis3dwb->status & 0x1) {
		uint8_t *buf, *read_buf;
		uint32_t buf_len;
		uint32_t req_len = 6 + sizeof(struct iis3dwb_rtio_data);

		if (rtio_sqe_rx_buf(iis3dwb->streaming_sqe, req_len, req_len, &buf, &buf_len) !=
		    0) {
			LOG_ERR("Failed to get buffer");
			rtio_iodev_sqe_err(iis3dwb->streaming_sqe, -ENOMEM);
			iis3dwb->streaming_sqe = NULL;
			gpio_pin_interrupt_configure_dt(irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
			return;
		}

		/* clang-format off */
		struct iis3dwb_rtio_data hdr = {
			.header = {
				.is_fifo = 0,
				.range = iis3dwb->range,
				.timestamp = iis3dwb->timestamp,
				.int_status = iis3dwb->status,
			},
			.has_accel = 1,
			.has_temp = 0,
		};
		/* clang-format on */

		memcpy(buf, &hdr, sizeof(hdr));
		read_buf = (uint8_t *)&((struct iis3dwb_rtio_data *)buf)->accel[0];

		struct rtio_regs fifo_regs;
		struct rtio_regs_list regs_list[] = {
			{
				0x80 | IIS3DWB_OUTX_L_A, /* mark the SPI read transaction */
				read_buf,
				6,
			},
		};

		fifo_regs.rtio_regs_list = regs_list;
		fifo_regs.rtio_regs_num = ARRAY_SIZE(regs_list);

		/*
		 * Prepare rtio enabled bus to read IIS3DWB_OUTX_L_A register
		 * where accelerometer data is available.
		 * Then iis3dwb_complete_op_cb callback will be invoked.
		 *
		 * STMEMSC API equivalent code:
		 *
		 *   uint8_t accel_raw[6];
		 *
		 *   iis3dwb_acceleration_raw_get(&dev_ctx, accel_raw);
		 */
		rtio_read_regs_async(iis3dwb->rtio_ctx, iis3dwb->iodev, RTIO_BUS_SPI, &fifo_regs,
				     iis3dwb->streaming_sqe, dev, iis3dwb_complete_op_cb);
	}
}

/*
 * Called when one of the following trigger is active:
 *
 *     - int_fifo_th (SENSOR_TRIG_FIFO_WATERMARK)
 *     - int_fifo_full (SENSOR_TRIG_FIFO_FULL)
 *     - int_drdy (SENSOR_TRIG_DATA_READY)
 */
void iis3dwb_stream_irq_handler(const struct device *dev)
{
	struct iis3dwb_data *iis3dwb = dev->data;
	uint64_t cycles;
	int rc;

	if (iis3dwb->streaming_sqe == NULL) {
		return;
	}

	rc = sensor_clock_get_cycles(&cycles);
	if (rc != 0) {
		LOG_ERR("Failed to get sensor clock cycles");
		rtio_iodev_sqe_err(iis3dwb->streaming_sqe, rc);
		return;
	}

	/* get timestamp as soon as the irq is served */
	iis3dwb->timestamp = sensor_clock_cycles_to_ns(cycles);

	/* handle FIFO triggers */
	if (iis3dwb->trig_cfg.int_fifo_th || iis3dwb->trig_cfg.int_fifo_full) {
		iis3dwb->fifo_status[0] = iis3dwb->fifo_status[1] = 0;

		struct rtio_regs fifo_regs;
		struct rtio_regs_list regs_list[] = {
			{
				0x80 | IIS3DWB_FIFO_STATUS1, /* mark the SPI read transaction */
				iis3dwb->fifo_status,
				2,
			},
		};

		fifo_regs.rtio_regs_list = regs_list;
		fifo_regs.rtio_regs_num = ARRAY_SIZE(regs_list);

		/*
		 * Prepare rtio enabled bus to read IIS3DWB_FIFO_STATUS1 and
		 * IIS3DWB_FIFO_STATUS2 registers where FIFO threshold condition and
		 * count are reported. Then iis3dwb_read_fifo_cb callback will be
		 * invoked.
		 *
		 * STMEMSC API equivalent code:
		 *
		 *   iis3dwb_fifo_status_t fifo_status;
		 *   iis3dwb_fifo_status_get(&dev_ctx, &fifo_status);
		 */
		rtio_read_regs_async(iis3dwb->rtio_ctx, iis3dwb->iodev, RTIO_BUS_SPI, &fifo_regs,
				     iis3dwb->streaming_sqe, dev, iis3dwb_read_fifo_cb);
	}

	/* handle drdy trigger */
	if (iis3dwb->trig_cfg.int_drdy) {
		iis3dwb->status = 0;

		struct rtio_regs fifo_regs;
		struct rtio_regs_list regs_list[] = {
			{
				0x80 | IIS3DWB_STATUS_REG, /* mark the SPI read transaction */
				&iis3dwb->status,
				1,
			},
		};

		fifo_regs.rtio_regs_list = regs_list;
		fifo_regs.rtio_regs_num = ARRAY_SIZE(regs_list);

		/*
		 * Prepare rtio enabled bus to read IIS3DWB_STATUS_REG register
		 * where accelerometer and gyroscope data ready status is available.
		 * Then iis3dwb_read_status_cb callback will be invoked.
		 *
		 * STMEMSC API equivalent code:
		 *
		 *   uint8_t val;
		 *
		 *   iis3dwb_xl_flag_data_ready_get(&dev_ctx, &val);
		 */
		rtio_read_regs_async(iis3dwb->rtio_ctx, iis3dwb->iodev, RTIO_BUS_SPI, &fifo_regs,
				     iis3dwb->streaming_sqe, dev, iis3dwb_read_status_cb);
	}
}
