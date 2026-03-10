/* ST Microelectronics LSM6DSVXXX family IMU sensor
 *
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lsm6dsv320x.pdf
 */

#include "lsm6dsvxxx.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(LSM6DSVXXX, CONFIG_SENSOR_LOG_LEVEL);

void lsm6dsvxxx_gpio_pin_enable(const struct lsm6dsvxxx_config *config,
				const struct gpio_dt_spec *irq_gpio)
{
	if (!ON_I3C_BUS(config) || (I3C_INT_PIN(config))) {
		gpio_pin_interrupt_configure_dt(irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	}
}

void lsm6dsvxxx_gpio_pin_disable(const struct lsm6dsvxxx_config *config,
				 const struct gpio_dt_spec *irq_gpio)
{
	if (!ON_I3C_BUS(config) || (I3C_INT_PIN(config))) {
		gpio_pin_interrupt_configure_dt(irq_gpio, GPIO_INT_DISABLE);
	}
}

int lsm6dsvxxx_gbias_config(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	struct lsm6dsvxxx_data *lsm6dsvxxx = dev->data;

	switch (attr) {
	case SENSOR_ATTR_OFFSET:
		lsm6dsvxxx->gbias_x_udps = 10 * sensor_rad_to_10udegrees(&val[0]);
		lsm6dsvxxx->gbias_y_udps = 10 * sensor_rad_to_10udegrees(&val[1]);
		lsm6dsvxxx->gbias_z_udps = 10 * sensor_rad_to_10udegrees(&val[2]);
		break;
	default:
		LOG_DBG("Accel attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}

int lsm6dsvxxx_gbias_get_config(const struct device *dev, enum sensor_channel chan,
				enum sensor_attribute attr, struct sensor_value *val)
{
	struct lsm6dsvxxx_data *lsm6dsvxxx = dev->data;

	switch (attr) {
	case SENSOR_ATTR_OFFSET:
		sensor_10udegrees_to_rad(lsm6dsvxxx->gbias_x_udps / 10, &val[0]);
		sensor_10udegrees_to_rad(lsm6dsvxxx->gbias_y_udps / 10, &val[1]);
		sensor_10udegrees_to_rad(lsm6dsvxxx->gbias_z_udps / 10, &val[2]);
		break;
	default:
		LOG_DBG("Accel attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}

void lsm6dsvxxx_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct lsm6dsvxxx_data *data = dev->data;
	const struct lsm6dsvxxx_config *config = dev->config;
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	struct trigger_config trig_cfg = { 0 };

	lsm6dsvxxx_gpio_pin_disable(config, data->drdy_gpio);

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
			break;
		}
	}

	/* if any change in trig_cfg for FIFO triggers */
	if (trig_cfg.int_fifo_th != data->trig_cfg.int_fifo_th ||
	    trig_cfg.int_fifo_full != data->trig_cfg.int_fifo_full) {
		data->trig_cfg.int_fifo_th = trig_cfg.int_fifo_th;
		data->trig_cfg.int_fifo_full = trig_cfg.int_fifo_full;

		/* enable/disable the FIFO */
		config->chip_api->config_fifo(dev, data->trig_cfg);
	}

	/* if any change in trig_cfg for DRDY triggers */
	if (trig_cfg.int_drdy != data->trig_cfg.int_drdy) {
		data->trig_cfg.int_drdy = trig_cfg.int_drdy;

		/* enable/disable drdy events */
		config->chip_api->config_drdy(dev, data->trig_cfg);
	}

	data->streaming_sqe = iodev_sqe;

	lsm6dsvxxx_gpio_pin_enable(config, data->drdy_gpio);
}

/*
 * Called by bus driver to complete the sqe.
 */
static void lsm6dsvxxx_complete_op_cb(struct rtio *r, const struct rtio_sqe *sqe,
				      int result, void *arg)
{
	ARG_UNUSED(result);

	const struct device *dev = arg;
	struct lsm6dsvxxx_data *data = dev->data;

	/*
	 * Mark operation completed
	 */
	data->streaming_sqe = NULL;
	rtio_iodev_sqe_ok(sqe->userdata, 0);
	lsm6dsvxxx_gpio_pin_enable(dev->config, data->drdy_gpio);
}

/*
 * Called by bus driver to complete the LSM6DSVXXX_FIFO_STATUS read op (2 bytes).
 * If FIFO threshold or FIFO full events are active it reads all FIFO entries.
 */
static void lsm6dsvxxx_read_fifo_cb(struct rtio *r, const struct rtio_sqe *sqe,
				    int result, void *arg)
{
	ARG_UNUSED(result);

	const struct device *dev = arg;
	const struct lsm6dsvxxx_config *cfg = dev->config;
	struct lsm6dsvxxx_data *data = dev->data;
	struct rtio *rtio = data->rtio_ctx;
	const struct gpio_dt_spec *irq_gpio = data->drdy_gpio;
	struct rtio_iodev *iodev = data->iodev;
	struct sensor_read_config *read_config;
	uint8_t fifo_th = 0, fifo_full = 0;
	uint16_t fifo_count;

	/* At this point, no sqe request is queued should be considered as a bug */
	__ASSERT_NO_MSG(data->streaming_sqe != NULL);

	read_config = (struct sensor_read_config *)data->streaming_sqe->sqe.iodev->data;
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
	fifo_th = (data->fifo_status[1] & 0x80) ? 1 : 0;
	fifo_full = (data->fifo_status[1] & 0x20) ? 1 : 0;
	fifo_count = (uint16_t)data->fifo_status[1] & 0x1U;
	fifo_count = (fifo_count << 8) | data->fifo_status[0];
	data->fifo_count = fifo_count;

	bool has_fifo_ths_trig = fifo_ths_cfg != NULL && fifo_th == 1;
	bool has_fifo_full_trig = fifo_full_cfg != NULL && fifo_full == 1;

	/* check if no theshold/full fifo interrupt or spurious interrupts */
	if (!has_fifo_ths_trig && !has_fifo_full_trig) {
		/* complete operation with no error */
		rtio_iodev_sqe_ok(sqe->userdata, 0);

		data->streaming_sqe = NULL;
		lsm6dsvxxx_gpio_pin_enable(cfg, irq_gpio);
		return;
	}

	/* flush completion */
	int res = 0;

	res = rtio_flush_completion_queue(rtio);

	/* Bail/cancel attempt to read sensor on any error */
	if (res != 0) {
		rtio_iodev_sqe_err(data->streaming_sqe, res);
		data->streaming_sqe = NULL;
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
		if (rtio_sqe_rx_buf(data->streaming_sqe, sizeof(struct lsm6dsvxxx_fifo_data),
				    sizeof(struct lsm6dsvxxx_fifo_data), &buf, &buf_len) != 0) {
			rtio_iodev_sqe_err(data->streaming_sqe, -ENOMEM);
			data->streaming_sqe = NULL;
			lsm6dsvxxx_gpio_pin_enable(cfg, irq_gpio);
			return;
		}

		struct lsm6dsvxxx_fifo_data *rx_data = (struct lsm6dsvxxx_fifo_data *)buf;

		memset(buf, 0, buf_len);
		rx_data->header.cfg = cfg;
		rx_data->header.is_fifo = 1;
		rx_data->header.timestamp = data->timestamp;
		rx_data->header.int_status = data->fifo_status[1];
		rx_data->fifo_count = 0;

		/* complete request with ok */
		rtio_iodev_sqe_ok(data->streaming_sqe, 0);
		data->streaming_sqe = NULL;
		lsm6dsvxxx_gpio_pin_enable(cfg, irq_gpio);

		if (data_opt == SENSOR_STREAM_DATA_DROP) {

			/*
			 * Flush the FIFO by setting the mode to LSM6DSVXXX_BYPASS_MODE
			 *
			 * STMEMSC API equivalent code:
			 *
			 *   lsm6dsvxxx_fifo_mode_set(ctx, LSM6DSVXXX_BYPASS_MODE);
			 */
			struct rtio_sqe *write_fifo_mode = rtio_sqe_acquire(rtio);
			uint8_t lsm6dsvxxx_fifo_mode_set[] = {
				LSM6DSVXXX_FIFO_CTRL4,
				LSM6DSVXXX_BYPASS_MODE,
			};

			write_fifo_mode->flags |= RTIO_SQE_NO_RESPONSE;
			rtio_sqe_prep_tiny_write(write_fifo_mode, iodev,
						 RTIO_PRIO_NORM, lsm6dsvxxx_fifo_mode_set,
						 ARRAY_SIZE(lsm6dsvxxx_fifo_mode_set), NULL);

			rtio_submit(rtio, 0);
		}

		return;
	}

	uint8_t *buf, *read_buf;
	uint32_t buf_len, buf_avail;
	uint32_t req_len = LSM6DSVXXX_FIFO_SIZE(fifo_count) + sizeof(struct lsm6dsvxxx_fifo_data);

	if (rtio_sqe_rx_buf(data->streaming_sqe, req_len, req_len, &buf, &buf_len) != 0) {
		LOG_ERR("Failed to get buffer");
		rtio_iodev_sqe_err(data->streaming_sqe, -ENOMEM);
		data->streaming_sqe = NULL;
		lsm6dsvxxx_gpio_pin_enable(cfg, irq_gpio);
		return;
	}

	/* clang-format off */
	struct lsm6dsvxxx_fifo_data hdr = {
		.header = {
			.cfg = cfg,
			.is_fifo = true,
			.accel_fs = data->accel_fs,
			.gyro_fs = data->gyro_fs,
			.timestamp = data->timestamp,
		},
		.fifo_count = fifo_count,
		.accel_batch_odr = data->accel_batch_odr,
		.gyro_batch_odr = data->gyro_batch_odr,
#if defined(CONFIG_LSM6DSVXXX_ENABLE_TEMP)
		.temp_batch_odr = data->temp_batch_odr,
#endif
		.sflp_batch_odr = data->sflp_batch_odr,
	};
	/* clang-format on */

	memcpy(buf, &hdr, sizeof(hdr));
	read_buf = buf + sizeof(hdr);
	buf_avail = buf_len - sizeof(hdr);

	uint8_t reg_addr = lsm6dsvxxx_bus_reg(data->bus_type, LSM6DSVXXX_FIFO_DATA_OUT_TAG);
	struct rtio_regs fifo_regs;
	struct rtio_regs_list regs_list[] = {
		{
			reg_addr,
			read_buf,
			buf_avail,
		},
	};

	fifo_regs.rtio_regs_list = regs_list;
	fifo_regs.rtio_regs_num = ARRAY_SIZE(regs_list);

	/*
	 * Prepare rtio enabled bus to read all fifo_count entries from
	 * LSM6DSVXXX_FIFO_DATA_OUT_TAG.  Then lsm6dsvxxx_complete_op_cb
	 * callback will be invoked.
	 *
	 * STMEMSC API equivalent code:
	 *
	 *   num = fifo_status.fifo_level;
	 *
	 *   while (num--) {
	 *     lsm6dsvxxx_fifo_out_raw_t f_data;
	 *
	 *     lsm6dsvxxx_fifo_out_raw_get(&dev_ctx, &f_data);
	 *   }
	 */
	rtio_read_regs_async(data->rtio_ctx, data->iodev, data->bus_type,
			     &fifo_regs, data->streaming_sqe, dev, lsm6dsvxxx_complete_op_cb);
}

/*
 * Called by bus driver to complete the LSM6DSVXXX_STATUS_REG read op.
 * If drdy_xl is active it reads XL data (6 bytes) from LSM6DSVXXX_OUTX_L_A reg.
 */
static void lsm6dsvxxx_read_status_cb(struct rtio *r, const struct rtio_sqe *sqe,
				      int result, void *arg)
{
	ARG_UNUSED(result);

	const struct device *dev = arg;
	const struct lsm6dsvxxx_config *config = dev->config;
	struct lsm6dsvxxx_data *data = dev->data;
	struct rtio *rtio = data->rtio_ctx;
	const struct gpio_dt_spec *irq_gpio = data->drdy_gpio;
	struct sensor_read_config *read_config;

	/* At this point, no sqe request is queued should be considered as a bug */
	__ASSERT_NO_MSG(data->streaming_sqe != NULL);

	read_config = (struct sensor_read_config *)data->streaming_sqe->sqe.iodev->data;
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

	if (res != 0) {
		rtio_iodev_sqe_err(data->streaming_sqe, res);
		data->streaming_sqe = NULL;
		return;
	}

	if (data_ready != NULL &&
	    (data_ready->opt == SENSOR_STREAM_DATA_NOP ||
	     data_ready->opt == SENSOR_STREAM_DATA_DROP)) {
		uint8_t *buf;
		uint32_t buf_len;

		/* Clear streaming_sqe since we're done with the call */
		if (rtio_sqe_rx_buf(data->streaming_sqe, sizeof(struct lsm6dsvxxx_fifo_data),
				    sizeof(struct lsm6dsvxxx_fifo_data), &buf, &buf_len) != 0) {
			rtio_iodev_sqe_err(data->streaming_sqe, -ENOMEM);
			data->streaming_sqe = NULL;
			lsm6dsvxxx_gpio_pin_enable(config, irq_gpio);
			return;
		}

		struct lsm6dsvxxx_fifo_data *rx_data = (struct lsm6dsvxxx_fifo_data *)buf;

		memset(buf, 0, buf_len);
		rx_data->header.cfg = config;
		rx_data->header.is_fifo = 1;
		rx_data->header.timestamp = data->timestamp;
		rx_data->header.int_status = data->fifo_status[1];
		rx_data->fifo_count = 0;

		/* complete request with ok */
		rtio_iodev_sqe_ok(data->streaming_sqe, 0);
		data->streaming_sqe = NULL;
		lsm6dsvxxx_gpio_pin_enable(config, irq_gpio);

		if (data_ready->opt == SENSOR_STREAM_DATA_DROP) {

			/*
			 * Flush the FIFO by setting the mode to LSM6DSVXXX_BYPASS_MODE
			 *
			 * STMEMSC API equivalent code:
			 *
			 *   lsm6dsvxxx_fifo_mode_set(ctx, LSM6DSVXXX_BYPASS_MODE);
			 */
			struct rtio_sqe *write_fifo_mode = rtio_sqe_acquire(rtio);
			uint8_t lsm6dsvxxx_fifo_mode_set[] = {
				LSM6DSVXXX_FIFO_CTRL4,
				LSM6DSVXXX_BYPASS_MODE,
			};

			write_fifo_mode->flags |= RTIO_SQE_NO_RESPONSE;
			rtio_sqe_prep_tiny_write(write_fifo_mode, data->iodev,
						 RTIO_PRIO_NORM, lsm6dsvxxx_fifo_mode_set,
						 ARRAY_SIZE(lsm6dsvxxx_fifo_mode_set), NULL);

			rtio_submit(rtio, 0);
		}

		return;
	}

	/*
	 * Read XL data
	 *
	 * STMEMSC API equivalent code:
	 *
	 *   lsm6dsvxxx_data_ready_t drdy;
	 *   if (drdy.drdy_xl) {
	 */
	if (data->status & 0x1) {
		uint8_t *buf, *read_buf;
		uint32_t buf_len;
		uint32_t req_len = 6 + sizeof(struct lsm6dsvxxx_rtio_data);

		if (rtio_sqe_rx_buf(data->streaming_sqe,
				    req_len, req_len, &buf, &buf_len) != 0) {
			LOG_ERR("Failed to get buffer");
			rtio_iodev_sqe_err(data->streaming_sqe, -ENOMEM);
			data->streaming_sqe = NULL;
			lsm6dsvxxx_gpio_pin_enable(config, irq_gpio);
			return;
		}

		struct lsm6dsvxxx_rtio_data hdr = {
			.header = {
				.cfg = config,
				.is_fifo = false,
				.accel_fs = data->accel_fs,
				.gyro_fs = data->gyro_fs,
				.timestamp = data->timestamp,
			},
			.has_accel = 1,
			.has_temp = 0,
		};

		memcpy(buf, &hdr, sizeof(hdr));
		read_buf = (uint8_t *)&((struct lsm6dsvxxx_rtio_data *)buf)->accel[0];

		uint8_t reg_addr = lsm6dsvxxx_bus_reg(data->bus_type, data->out_xl);
		struct rtio_regs drdy_regs;
		struct rtio_regs_list regs_list[] = {
			{
				reg_addr,
				read_buf,
				6,
			},
		};

		drdy_regs.rtio_regs_list = regs_list;
		drdy_regs.rtio_regs_num = ARRAY_SIZE(regs_list);

		/*
		 * Prepare rtio enabled bus to read LSM6DSVXXX_OUTX_L_A register
		 * where accelerometer data is available.
		 * Then lsm6dsvxxx_complete_op_cb callback will be invoked.
		 *
		 * STMEMSC API equivalent code:
		 *
		 *   uint8_t accel_raw[6];
		 *
		 *   lsm6dsvxxx_acceleration_raw_get(&dev_ctx, accel_raw);
		 */
		rtio_read_regs_async(data->rtio_ctx, data->iodev, data->bus_type,
				     &drdy_regs, data->streaming_sqe, dev,
				     lsm6dsvxxx_complete_op_cb);
	}
}

/*
 * Called when one of the following trigger is active:
 *
 *     - int_fifo_th (SENSOR_TRIG_FIFO_WATERMARK)
 *     - int_fifo_full (SENSOR_TRIG_FIFO_FULL)
 *     - int_drdy (SENSOR_TRIG_DATA_READY)
 */
void lsm6dsvxxx_stream_irq_handler(const struct device *dev)
{
	struct lsm6dsvxxx_data *data = dev->data;
	uint64_t cycles;
	int rc;

	if (data->streaming_sqe == NULL) {
		return;
	}

	rc = sensor_clock_get_cycles(&cycles);
	if (rc != 0) {
		LOG_ERR("Failed to get sensor clock cycles");
		rtio_iodev_sqe_err(data->streaming_sqe, rc);
		return;
	}

	/* get timestamp as soon as the irq is served */
	data->timestamp = sensor_clock_cycles_to_ns(cycles);

	/* handle FIFO triggers */
	if (data->trig_cfg.int_fifo_th || data->trig_cfg.int_fifo_full) {
#if LSM6DSVXXX_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
		if (ON_I3C_BUS(config) && (!I3C_INT_PIN(config))) {
			/*
			 * If we are on an I3C bus, then it should be expected that the fifo status
			 * was already received in the IBI payload and we don't need to read it
			 * again.
			 */
			data->fifo_status[0] = data->ibi_payload.fifo_status1;
			data->fifo_status[1] = data->ibi_payload.fifo_status2;

			struct rtio_sqe *check_fifo_status_reg = rtio_sqe_acquire(rtio);

			rtio_sqe_prep_callback_no_cqe(check_fifo_status_reg,
					      data_read_fifo_cb, (void *)dev, NULL);
			rtio_submit(rtio, 0);
		} else {
#endif
			data->fifo_status[0] = data->fifo_status[1] = 0;

			uint8_t reg_addr =
				lsm6dsvxxx_bus_reg(data->bus_type, LSM6DSVXXX_FIFO_STATUS1);
			struct rtio_regs fifo_regs;
			struct rtio_regs_list regs_list[] = {
				{
					reg_addr,
					data->fifo_status,
					2,
				},
			};

			fifo_regs.rtio_regs_list = regs_list;
			fifo_regs.rtio_regs_num = ARRAY_SIZE(regs_list);

			/*
			 * Prepare rtio enabled bus to read LSM6DSVXXX_FIFO_STATUS1 and
			 * LSM6DSVXXX_FIFO_STATUS2 registers where FIFO threshold condition and
			 * count are reported. Then lsm6dsvxxx_read_fifo_cb callback will be
			 * invoked.
			 *
			 * STMEMSC API equivalent code:
			 *
			 *   lsm6dsvxxx_fifo_status_t fifo_status;
			 *
			 *   lsm6dsvxxx_fifo_status_get(&dev_ctx, &fifo_status);
			 */
			rtio_read_regs_async(data->rtio_ctx, data->iodev,
					     data->bus_type, &fifo_regs,
					     data->streaming_sqe, dev,
					     lsm6dsvxxx_read_fifo_cb);
#if LSM6DSVXXX_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
		}
#endif
	}

	/* handle drdy trigger */
	if (data->trig_cfg.int_drdy) {
		data->status = 0;

		uint8_t reg_addr =
			lsm6dsvxxx_bus_reg(data->bus_type, LSM6DSVXXX_STATUS_REG);
		struct rtio_regs drdy_regs;
		struct rtio_regs_list regs_list[] = {
			{
				reg_addr,
				&data->status,
				1,
			},
		};

		drdy_regs.rtio_regs_list = regs_list;
		drdy_regs.rtio_regs_num = ARRAY_SIZE(regs_list);

		/*
		 * Prepare rtio enabled bus to read LSM6DSVXXX_STATUS_REG register
		 * where accelerometer and gyroscope data ready status is available.
		 * Then data_read_status_cb callback will be invoked.
		 *
		 * STMEMSC API equivalent code:
		 *
		 *   uint8_t val;
		 *
		 *   data_xl_flag_data_ready_get(&dev_ctx, &val);
		 */
		rtio_read_regs_async(data->rtio_ctx, data->iodev,
				     data->bus_type, &drdy_regs,
				     data->streaming_sqe, dev,
				     lsm6dsvxxx_read_status_cb);
	}
}
