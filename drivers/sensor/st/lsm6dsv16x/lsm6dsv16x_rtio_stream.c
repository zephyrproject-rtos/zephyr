/* ST Microelectronics LSM6DSV16X 6-axis IMU sensor driver
 *
 * Copyright (c) 2023 Google LLC
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lsm6dsv16x

#include <zephyr/dt-bindings/sensor/lsm6dsv16x.h>
#include <zephyr/drivers/sensor.h>
#include "lsm6dsv16x.h"
#include "lsm6dsv16x_decoder.h"
#include <zephyr/rtio/work.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(LSM6DSV16X_RTIO);

#define FIFO_TH 1
#define FIFO_FULL 2

static void lsm6dsv16x_config_fifo(const struct device *dev, uint8_t fifo_irq)
{
	struct lsm6dsv16x_data *lsm6dsv16x = dev->data;
	const struct lsm6dsv16x_config *config = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&config->ctx;
	uint8_t fifo_wtm = 0;
	lsm6dsv16x_pin_int_route_t pin_int = { 0 };
	lsm6dsv16x_fifo_xl_batch_t xl_batch = LSM6DSV16X_DT_XL_NOT_BATCHED;
	lsm6dsv16x_fifo_gy_batch_t gy_batch = LSM6DSV16X_DT_GY_NOT_BATCHED;
	lsm6dsv16x_fifo_temp_batch_t temp_batch = LSM6DSV16X_DT_TEMP_NOT_BATCHED;
	lsm6dsv16x_fifo_mode_t fifo_mode = LSM6DSV16X_BYPASS_MODE;
	lsm6dsv16x_sflp_data_rate_t sflp_odr = LSM6DSV16X_SFLP_120Hz;
	lsm6dsv16x_fifo_sflp_raw_t sflp_fifo = { 0 };

	/* disable FIFO as first thing */
	lsm6dsv16x_fifo_mode_set(ctx, LSM6DSV16X_BYPASS_MODE);

	pin_int.fifo_th = PROPERTY_DISABLE;
	pin_int.fifo_full = PROPERTY_DISABLE;

	if (fifo_irq != 0) {
		pin_int.fifo_th = (fifo_irq & FIFO_TH) ? PROPERTY_ENABLE : PROPERTY_DISABLE;
		pin_int.fifo_full = (fifo_irq & FIFO_FULL) ? PROPERTY_ENABLE : PROPERTY_DISABLE;

		xl_batch = config->accel_batch;
		gy_batch = config->gyro_batch;
		temp_batch = config->temp_batch;

		fifo_mode = LSM6DSV16X_STREAM_MODE;
		fifo_wtm = config->fifo_wtm;

		if (config->sflp_fifo_en & LSM6DSV16X_DT_SFLP_FIFO_GAME_ROTATION) {
			sflp_fifo.game_rotation = 1;
		}

		if (config->sflp_fifo_en & LSM6DSV16X_DT_SFLP_FIFO_GRAVITY) {
			sflp_fifo.gravity = 1;
		}

		if (config->sflp_fifo_en & LSM6DSV16X_DT_SFLP_FIFO_GBIAS) {
			sflp_fifo.gbias = 1;
		}

		sflp_odr = config->sflp_odr;
	}

	/*
	 * Set FIFO watermark (number of unread sensor data TAG + 6 bytes
	 * stored in FIFO) to FIFO_WATERMARK samples
	 */
	lsm6dsv16x_fifo_watermark_set(ctx, config->fifo_wtm);

	/* Turn on/off FIFO */
	lsm6dsv16x_fifo_mode_set(ctx, fifo_mode);

	/* Set FIFO batch rates */
	lsm6dsv16x_fifo_xl_batch_set(ctx, xl_batch);
	lsm6dsv16x->accel_batch_odr = xl_batch;
	lsm6dsv16x_fifo_gy_batch_set(ctx, gy_batch);
	lsm6dsv16x->gyro_batch_odr = gy_batch;
#if defined(CONFIG_LSM6DSV16X_ENABLE_TEMP)
	lsm6dsv16x_fifo_temp_batch_set(ctx, temp_batch);
	lsm6dsv16x->temp_batch_odr = temp_batch;
#endif

	lsm6dsv16x_sflp_data_rate_set(ctx, sflp_odr);
	lsm6dsv16x->sflp_batch_odr = sflp_odr;
	lsm6dsv16x_fifo_sflp_batch_set(ctx, sflp_fifo);
	lsm6dsv16x_sflp_game_rotation_set(ctx, PROPERTY_ENABLE);

	/* Set pin interrupt (fifo_th could be on or off) */
	if ((config->drdy_pin == 1) || (ON_I3C_BUS(config) && (!I3C_INT_PIN(config)))) {
		lsm6dsv16x_pin_int1_route_set(ctx, &pin_int);
	}  else {
		lsm6dsv16x_pin_int2_route_set(ctx, &pin_int);
	}
}

void lsm6dsv16x_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct lsm6dsv16x_data *lsm6dsv16x = dev->data;
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
	const struct lsm6dsv16x_config *config = dev->config;
#endif
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	uint8_t fifo_irq = 0;

	if (!ON_I3C_BUS(config) || (I3C_INT_PIN(config))) {
		gpio_pin_interrupt_configure_dt(lsm6dsv16x->drdy_gpio, GPIO_INT_DISABLE);
	}

	for (size_t i = 0; i < cfg->count; i++) {
		if (cfg->triggers[i].trigger == SENSOR_TRIG_FIFO_WATERMARK) {
			fifo_irq = FIFO_TH;
		} else if (cfg->triggers[i].trigger == SENSOR_TRIG_FIFO_FULL) {
			fifo_irq = FIFO_FULL;
		}
	}

	/* if any change in fifo irq */
	if (fifo_irq != lsm6dsv16x->fifo_irq) {
		lsm6dsv16x->fifo_irq = fifo_irq;

		/* enable/disable the FIFO */
		lsm6dsv16x_config_fifo(dev, fifo_irq);
	}

	lsm6dsv16x->streaming_sqe = iodev_sqe;

	if (!ON_I3C_BUS(config) || (I3C_INT_PIN(config))) {
		gpio_pin_interrupt_configure_dt(lsm6dsv16x->drdy_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	}
}

static void lsm6dsv16x_complete_op_cb(struct rtio *r, const struct rtio_sqe *sqe, void *arg)
{
	const struct device *dev = arg;
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
	const struct lsm6dsv16x_config *config = dev->config;
#endif
	struct lsm6dsv16x_data *lsm6dsv16x = dev->data;

	/*
	 * Mark operation completed
	 */
	rtio_iodev_sqe_ok(sqe->userdata, 0);
	lsm6dsv16x->streaming_sqe = NULL;
	if (!ON_I3C_BUS(config) || (I3C_INT_PIN(config))) {
		gpio_pin_interrupt_configure_dt(lsm6dsv16x->drdy_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	}
}

static void lsm6dsv16x_read_fifo_cb(struct rtio *r, const struct rtio_sqe *sqe, void *arg)
{
	const struct device *dev = arg;
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
	const struct lsm6dsv16x_config *config = dev->config;
#endif
	struct lsm6dsv16x_data *lsm6dsv16x = dev->data;
	struct gpio_dt_spec *irq_gpio = lsm6dsv16x->drdy_gpio;
	struct rtio_iodev *iodev = lsm6dsv16x->iodev;
	struct sensor_read_config *read_config;
	uint8_t fifo_th = 0, fifo_full = 0;
	uint16_t fifo_count;

	/* At this point, no sqe request is queued should be considered as a bug */
	__ASSERT_NO_MSG(lsm6dsv16x->streaming_sqe != NULL);

	read_config = (struct sensor_read_config *)lsm6dsv16x->streaming_sqe->sqe.iodev->data;
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
	fifo_th = (lsm6dsv16x->fifo_status[1] & 0x80) ? 1 : 0;
	fifo_full = (lsm6dsv16x->fifo_status[1] & 0x20) ? 1 : 0;
	fifo_count = (uint16_t)lsm6dsv16x->fifo_status[1] & 0x1U;
	fifo_count = (fifo_count << 8) | lsm6dsv16x->fifo_status[0];
	lsm6dsv16x->fifo_count = fifo_count;

	bool has_fifo_ths_trig = fifo_ths_cfg != NULL && fifo_th == 1;
	bool has_fifo_full_trig = fifo_full_cfg != NULL && fifo_full == 1;

	/* check if no theshold/full fifo interrupt or spurious interrupts */
	if (!has_fifo_ths_trig && !has_fifo_full_trig) {
		/* complete operation with no error */
		rtio_iodev_sqe_ok(sqe->userdata, 0);

		lsm6dsv16x->streaming_sqe = NULL;
		if (!ON_I3C_BUS(config) || (I3C_INT_PIN(config))) {
			gpio_pin_interrupt_configure_dt(irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		}
		return;
	}

	struct rtio_cqe *cqe;
	int res = 0;

	do {
		cqe = rtio_cqe_consume(lsm6dsv16x->rtio_ctx);
		if (cqe != NULL) {
			if ((cqe->result < 0) && (res == 0)) {
				LOG_ERR("Bus error: %d", cqe->result);
				res = cqe->result;
			}
			rtio_cqe_release(lsm6dsv16x->rtio_ctx, cqe);
		}
	} while (cqe != NULL);

	/* Bail/cancel attempt to read sensor on any error */
	if (res != 0) {
		rtio_iodev_sqe_err(lsm6dsv16x->streaming_sqe, res);
		lsm6dsv16x->streaming_sqe = NULL;
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
		if (rtio_sqe_rx_buf(lsm6dsv16x->streaming_sqe, sizeof(struct lsm6dsv16x_fifo_data),
				    sizeof(struct lsm6dsv16x_fifo_data), &buf, &buf_len) != 0) {
			rtio_iodev_sqe_err(lsm6dsv16x->streaming_sqe, -ENOMEM);
			lsm6dsv16x->streaming_sqe = NULL;
			if (!ON_I3C_BUS(config) || (I3C_INT_PIN(config))) {
				gpio_pin_interrupt_configure_dt(irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
			}
			return;
		}

		struct lsm6dsv16x_fifo_data *rx_data = (struct lsm6dsv16x_fifo_data *)buf;

		memset(buf, 0, buf_len);
		rx_data->header.is_fifo = 1;
		rx_data->header.timestamp = lsm6dsv16x->fifo_timestamp;
		rx_data->int_status = lsm6dsv16x->fifo_status[1];
		rx_data->fifo_count = 0;

		/* complete request with ok */
		rtio_iodev_sqe_ok(lsm6dsv16x->streaming_sqe, 0);
		lsm6dsv16x->streaming_sqe = NULL;
		if (!ON_I3C_BUS(config) || (I3C_INT_PIN(config))) {
			gpio_pin_interrupt_configure_dt(irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		}

		if (data_opt == SENSOR_STREAM_DATA_DROP) {

			/*
			 * Flush the FIFO by setting the mode to LSM6DSV16X_BYPASS_MODE
			 *
			 * STMEMSC API equivalent code:
			 *
			 *   lsm6dsv16x_fifo_mode_set(ctx, LSM6DSV16X_BYPASS_MODE);
			 */
			struct rtio_sqe *write_fifo_mode = rtio_sqe_acquire(lsm6dsv16x->rtio_ctx);
			uint8_t lsm6dsv16x_fifo_mode_set[] = {
				LSM6DSV16X_FIFO_CTRL4,
				LSM6DSV16X_BYPASS_MODE,
			};

			write_fifo_mode->flags |= RTIO_SQE_NO_RESPONSE;
			rtio_sqe_prep_tiny_write(write_fifo_mode, iodev,
						 RTIO_PRIO_NORM, lsm6dsv16x_fifo_mode_set,
						 ARRAY_SIZE(lsm6dsv16x_fifo_mode_set), NULL);

			rtio_submit(lsm6dsv16x->rtio_ctx, 0);
		}

		return;
	}

	uint8_t *buf, *read_buf;
	uint32_t buf_len, buf_avail;
	uint32_t req_len = LSM6DSV16X_FIFO_SIZE(fifo_count) + sizeof(struct lsm6dsv16x_fifo_data);

	if (rtio_sqe_rx_buf(lsm6dsv16x->streaming_sqe, req_len, req_len, &buf, &buf_len) != 0) {
		LOG_ERR("Failed to get buffer");
		rtio_iodev_sqe_err(lsm6dsv16x->streaming_sqe, -ENOMEM);
		lsm6dsv16x->streaming_sqe = NULL;
		if (!ON_I3C_BUS(config) || (I3C_INT_PIN(config))) {
			gpio_pin_interrupt_configure_dt(irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		}
		return;
	}

	struct lsm6dsv16x_fifo_data hdr = {
		.header = {
			.is_fifo = true,
			.accel_fs = lsm6dsv16x->accel_fs,
			.gyro_fs = lsm6dsv16x->gyro_fs,
			.timestamp = lsm6dsv16x->fifo_timestamp,
		},
		.fifo_count = fifo_count,
		.accel_batch_odr = lsm6dsv16x->accel_batch_odr,
		.gyro_batch_odr = lsm6dsv16x->gyro_batch_odr,
#if defined(CONFIG_LSM6DSV16X_ENABLE_TEMP)
		.temp_batch_odr = lsm6dsv16x->temp_batch_odr,
#endif
		.sflp_batch_odr = lsm6dsv16x->sflp_batch_odr,
	};

	memcpy(buf, &hdr, sizeof(hdr));
	read_buf = buf + sizeof(hdr);
	buf_avail = buf_len - sizeof(hdr);

	/*
	 * Prepare rtio enabled bus to read all fifo_count entries from
	 * LSM6DSV16X_FIFO_DATA_OUT_TAG.  Then lsm6dsv16x_complete_op_cb
	 * callback will be invoked.
	 *
	 * STMEMSC API equivalent code:
	 *
	 *   num = fifo_status.fifo_level;
	 *
	 *   while (num--) {
	 *     lsm6dsv16x_fifo_out_raw_t f_data;
	 *
	 *     lsm6dsv16x_fifo_out_raw_get(&dev_ctx, &f_data);
	 *   }
	 */
	struct rtio_sqe *write_fifo_dout_addr = rtio_sqe_acquire(lsm6dsv16x->rtio_ctx);
	struct rtio_sqe *read_fifo_dout_reg = rtio_sqe_acquire(lsm6dsv16x->rtio_ctx);
	struct rtio_sqe *complete_op = rtio_sqe_acquire(lsm6dsv16x->rtio_ctx);
	uint8_t reg = lsm6dsv16x_bus_reg(lsm6dsv16x, LSM6DSV16X_FIFO_DATA_OUT_TAG);

	rtio_sqe_prep_tiny_write(write_fifo_dout_addr, iodev, RTIO_PRIO_NORM, &reg, 1, NULL);
	write_fifo_dout_addr->flags = RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_fifo_dout_reg, iodev, RTIO_PRIO_NORM,
			   read_buf, buf_avail, lsm6dsv16x->streaming_sqe);
	read_fifo_dout_reg->flags = RTIO_SQE_CHAINED;
	if (lsm6dsv16x->bus_type == BUS_I2C) {
		read_fifo_dout_reg->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
	} else if (lsm6dsv16x->bus_type == BUS_I3C) {
		read_fifo_dout_reg->iodev_flags |= RTIO_IODEV_I3C_STOP | RTIO_IODEV_I3C_RESTART;
	}
	rtio_sqe_prep_callback_no_cqe(complete_op, lsm6dsv16x_complete_op_cb, (void *)dev,
				      lsm6dsv16x->streaming_sqe);

	rtio_submit(lsm6dsv16x->rtio_ctx, 0);
}

void lsm6dsv16x_stream_irq_handler(const struct device *dev)
{
	struct lsm6dsv16x_data *lsm6dsv16x = dev->data;
	struct rtio_iodev *iodev = lsm6dsv16x->iodev;
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
	const struct lsm6dsv16x_config *config = dev->config;
#endif

	if (lsm6dsv16x->streaming_sqe == NULL) {
		return;
	}

	/* get timestamp as soon as the irq is served */
	lsm6dsv16x->fifo_timestamp = k_ticks_to_ns_floor64(k_uptime_ticks());

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
	if (ON_I3C_BUS(config) && (!I3C_INT_PIN(config))) {
		/*
		 * If we are on an I3C bus, then it should be expected that the fifo status was
		 * already received in the IBI payload and we don't need to read it again.
		 */
		lsm6dsv16x->fifo_status[0] = lsm6dsv16x->ibi_payload.fifo_status1;
		lsm6dsv16x->fifo_status[1] = lsm6dsv16x->ibi_payload.fifo_status2;
	} else
#endif
	{
		lsm6dsv16x->fifo_status[0] = lsm6dsv16x->fifo_status[1] = 0;

		/*
		 * Prepare rtio enabled bus to read LSM6DSV16X_FIFO_STATUS1 and
		 * LSM6DSV16X_FIFO_STATUS2 registers where FIFO threshold condition and count are
		 * reported. Then lsm6dsv16x_read_fifo_cb callback will be invoked.
		 *
		 * STMEMSC API equivalent code:
		 *
		 *   lsm6dsv16x_fifo_status_t fifo_status;
		 *
		 *   lsm6dsv16x_fifo_status_get(&dev_ctx, &fifo_status);
		 */
		struct rtio_sqe *write_fifo_status_addr = rtio_sqe_acquire(lsm6dsv16x->rtio_ctx);
		struct rtio_sqe *read_fifo_status_reg = rtio_sqe_acquire(lsm6dsv16x->rtio_ctx);
		uint8_t reg = lsm6dsv16x_bus_reg(lsm6dsv16x, LSM6DSV16X_FIFO_STATUS1);

		rtio_sqe_prep_tiny_write(write_fifo_status_addr, iodev, RTIO_PRIO_NORM, &reg, 1,
					 NULL);
		write_fifo_status_addr->flags = RTIO_SQE_TRANSACTION;
		rtio_sqe_prep_read(read_fifo_status_reg, iodev, RTIO_PRIO_NORM,
				   lsm6dsv16x->fifo_status, 2, NULL);
		read_fifo_status_reg->flags = RTIO_SQE_CHAINED;
		if (lsm6dsv16x->bus_type == BUS_I2C) {
			read_fifo_status_reg->iodev_flags |=
				RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
		} else if (lsm6dsv16x->bus_type == BUS_I3C) {
			read_fifo_status_reg->iodev_flags |=
				RTIO_IODEV_I3C_STOP | RTIO_IODEV_I3C_RESTART;
		}
	}
	struct rtio_sqe *check_fifo_status_reg = rtio_sqe_acquire(lsm6dsv16x->rtio_ctx);

	rtio_sqe_prep_callback_no_cqe(check_fifo_status_reg,
				      lsm6dsv16x_read_fifo_cb, (void *)dev, NULL);
	rtio_submit(lsm6dsv16x->rtio_ctx, 0);
}
