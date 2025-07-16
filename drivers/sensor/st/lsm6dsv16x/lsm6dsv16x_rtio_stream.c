/* ST Microelectronics LSM6DSV16X 6-axis IMU sensor driver
 *
 * Copyright (c) 2023 Google LLC
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/dt-bindings/sensor/lsm6dsv16x.h>
#include <zephyr/drivers/sensor.h>
#include "lsm6dsv16x.h"
#include "lsm6dsv16x_decoder.h"
#include <zephyr/rtio/work.h>
#include <zephyr/drivers/sensor_clock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(LSM6DSV16X_RTIO);

static void lsm6dsv16x_config_drdy(const struct device *dev, struct trigger_config trig_cfg)
{
	const struct lsm6dsv16x_config *config = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&config->ctx;
	lsm6dsv16x_pin_int_route_t pin_int = { 0 };
	int16_t buf[3];

	/* dummy read: re-trigger interrupt */
	lsm6dsv16x_acceleration_raw_get(ctx, buf);

	pin_int.drdy_xl = PROPERTY_ENABLE;

	/* Set pin interrupt */
	if ((config->drdy_pin == 1) || (ON_I3C_BUS(config) && (!I3C_INT_PIN(config)))) {
		lsm6dsv16x_pin_int1_route_set(ctx, &pin_int);
	} else {
		lsm6dsv16x_pin_int2_route_set(ctx, &pin_int);
	}
}

int lsm6dsv16x_gbias_config(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	struct lsm6dsv16x_data *lsm6dsv16x = dev->data;

	switch (attr) {
	case SENSOR_ATTR_OFFSET:
		lsm6dsv16x->gbias_x_udps = 10 * sensor_rad_to_10udegrees(&val[0]);
		lsm6dsv16x->gbias_y_udps = 10 * sensor_rad_to_10udegrees(&val[1]);
		lsm6dsv16x->gbias_z_udps = 10 * sensor_rad_to_10udegrees(&val[2]);
		break;
	default:
		LOG_DBG("Accel attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}

int lsm6dsv16x_gbias_get_config(const struct device *dev, enum sensor_channel chan,
				enum sensor_attribute attr, struct sensor_value *val)
{
	struct lsm6dsv16x_data *lsm6dsv16x = dev->data;

	switch (attr) {
	case SENSOR_ATTR_OFFSET:
		sensor_10udegrees_to_rad(lsm6dsv16x->gbias_x_udps / 10, &val[0]);
		sensor_10udegrees_to_rad(lsm6dsv16x->gbias_y_udps / 10, &val[1]);
		sensor_10udegrees_to_rad(lsm6dsv16x->gbias_z_udps / 10, &val[2]);
		break;
	default:
		LOG_DBG("Accel attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static void lsm6dsv16x_config_fifo(const struct device *dev, struct trigger_config trig_cfg)
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
	lsm6dsv16x_sflp_gbias_t gbias;

	/* disable FIFO as first thing */
	lsm6dsv16x_fifo_mode_set(ctx, LSM6DSV16X_BYPASS_MODE);

	pin_int.fifo_th = PROPERTY_DISABLE;
	pin_int.fifo_full = PROPERTY_DISABLE;

	if (trig_cfg.int_fifo_th || trig_cfg.int_fifo_full) {
		pin_int.fifo_th = (trig_cfg.int_fifo_th) ? PROPERTY_ENABLE : PROPERTY_DISABLE;
		pin_int.fifo_full = (trig_cfg.int_fifo_full) ? PROPERTY_ENABLE : PROPERTY_DISABLE;

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

	/*
	 * Temporarly set Accel and gyro odr same as sensor fusion LP in order to
	 * make the SFLP gbias setting effective. Then restore it to saved values.
	 */
	switch (sflp_odr) {
	case LSM6DSV16X_DT_SFLP_ODR_AT_480Hz:
		lsm6dsv16x_accel_set_odr_raw(dev, LSM6DSV16X_DT_ODR_AT_480Hz);
		lsm6dsv16x_gyro_set_odr_raw(dev, LSM6DSV16X_DT_ODR_AT_480Hz);
		break;

	case LSM6DSV16X_DT_SFLP_ODR_AT_240Hz:
		lsm6dsv16x_accel_set_odr_raw(dev, LSM6DSV16X_DT_ODR_AT_240Hz);
		lsm6dsv16x_gyro_set_odr_raw(dev, LSM6DSV16X_DT_ODR_AT_240Hz);
		break;

	case LSM6DSV16X_DT_SFLP_ODR_AT_120Hz:
		lsm6dsv16x_accel_set_odr_raw(dev, LSM6DSV16X_DT_ODR_AT_120Hz);
		lsm6dsv16x_gyro_set_odr_raw(dev, LSM6DSV16X_DT_ODR_AT_120Hz);
		break;

	case LSM6DSV16X_DT_SFLP_ODR_AT_60Hz:
		lsm6dsv16x_accel_set_odr_raw(dev, LSM6DSV16X_DT_ODR_AT_60Hz);
		lsm6dsv16x_gyro_set_odr_raw(dev, LSM6DSV16X_DT_ODR_AT_60Hz);
		break;

	case LSM6DSV16X_DT_SFLP_ODR_AT_30Hz:
		lsm6dsv16x_accel_set_odr_raw(dev, LSM6DSV16X_DT_ODR_AT_30Hz);
		lsm6dsv16x_gyro_set_odr_raw(dev, LSM6DSV16X_DT_ODR_AT_30Hz);
		break;

	case LSM6DSV16X_DT_SFLP_ODR_AT_15Hz:
		lsm6dsv16x_accel_set_odr_raw(dev, LSM6DSV16X_DT_ODR_AT_15Hz);
		lsm6dsv16x_gyro_set_odr_raw(dev, LSM6DSV16X_DT_ODR_AT_15Hz);
		break;
	}

	/* set sflp gbias */
	gbias.gbias_x = (float)lsm6dsv16x->gbias_x_udps / 1000000;
	gbias.gbias_y = (float)lsm6dsv16x->gbias_y_udps / 1000000;
	gbias.gbias_z = (float)lsm6dsv16x->gbias_z_udps / 1000000;
	lsm6dsv16x_sflp_game_gbias_set(ctx, &gbias);

	/* restore accel/gyro odr to saved values */
	lsm6dsv16x_accel_set_odr_raw(dev, lsm6dsv16x->accel_freq);
	lsm6dsv16x_gyro_set_odr_raw(dev, lsm6dsv16x->gyro_freq);

	/* Set pin interrupt (fifo_th could be on or off) */
	if ((config->drdy_pin == 1) || (ON_I3C_BUS(config) && (!I3C_INT_PIN(config)))) {
		lsm6dsv16x_pin_int1_route_set(ctx, &pin_int);
	} else {
		lsm6dsv16x_pin_int2_route_set(ctx, &pin_int);
	}
}

void lsm6dsv16x_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct lsm6dsv16x_data *lsm6dsv16x = dev->data;
#if LSM6DSVXXX_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
	const struct lsm6dsv16x_config *config = dev->config;
#endif
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	struct trigger_config trig_cfg = { 0 };

	if (!ON_I3C_BUS(config) || (I3C_INT_PIN(config))) {
		gpio_pin_interrupt_configure_dt(lsm6dsv16x->drdy_gpio, GPIO_INT_DISABLE);
	}

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
	if (trig_cfg.int_fifo_th != lsm6dsv16x->trig_cfg.int_fifo_th ||
	    trig_cfg.int_fifo_full != lsm6dsv16x->trig_cfg.int_fifo_full) {
		lsm6dsv16x->trig_cfg.int_fifo_th = trig_cfg.int_fifo_th;
		lsm6dsv16x->trig_cfg.int_fifo_full = trig_cfg.int_fifo_full;

		/* enable/disable the FIFO */
		lsm6dsv16x_config_fifo(dev, trig_cfg);
	}

	/* if any change in trig_cfg for DRDY triggers */
	if (trig_cfg.int_drdy != lsm6dsv16x->trig_cfg.int_drdy) {
		lsm6dsv16x->trig_cfg.int_drdy = trig_cfg.int_drdy;

		/* enable/disable drdy events */
		lsm6dsv16x_config_drdy(dev, trig_cfg);
	}

	lsm6dsv16x->streaming_sqe = iodev_sqe;

	if (!ON_I3C_BUS(config) || (I3C_INT_PIN(config))) {
		gpio_pin_interrupt_configure_dt(lsm6dsv16x->drdy_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	}
}

/*
 * Called by bus driver to complete the sqe.
 */
static void lsm6dsv16x_complete_op_cb(struct rtio *r, const struct rtio_sqe *sqe,
				      int result, void *arg)
{
	ARG_UNUSED(result);

	const struct device *dev = arg;
#if LSM6DSVXXX_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
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

/*
 * Called by bus driver to complete the LSM6DSV16X_FIFO_STATUS read op (2 bytes).
 * If FIFO threshold or FIFO full events are active it reads all FIFO entries.
 */
static void lsm6dsv16x_read_fifo_cb(struct rtio *r, const struct rtio_sqe *sqe,
				    int result, void *arg)
{
	ARG_UNUSED(result);

	const struct device *dev = arg;
	const struct lsm6dsv16x_config *config = dev->config;
	struct lsm6dsv16x_data *lsm6dsv16x = dev->data;
	struct rtio *rtio = lsm6dsv16x->rtio_ctx;
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
		rx_data->header.timestamp = lsm6dsv16x->timestamp;
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
			struct rtio_sqe *write_fifo_mode = rtio_sqe_acquire(rtio);
			uint8_t lsm6dsv16x_fifo_mode_set[] = {
				LSM6DSV16X_FIFO_CTRL4,
				LSM6DSV16X_BYPASS_MODE,
			};

			write_fifo_mode->flags |= RTIO_SQE_NO_RESPONSE;
			rtio_sqe_prep_tiny_write(write_fifo_mode, iodev,
						 RTIO_PRIO_NORM, lsm6dsv16x_fifo_mode_set,
						 ARRAY_SIZE(lsm6dsv16x_fifo_mode_set), NULL);

			rtio_submit(rtio, 0);
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

	/* clang-format off */
	struct lsm6dsv16x_fifo_data hdr = {
		.header = {
			.is_fifo = true,
			.accel_fs_idx = LSM6DSV16X_ACCEL_FS_VAL_TO_FS_IDX(
				config->accel_fs_map[lsm6dsv16x->accel_fs]),
			.gyro_fs = lsm6dsv16x->gyro_fs,
			.timestamp = lsm6dsv16x->timestamp,
		},
		.fifo_count = fifo_count,
		.accel_batch_odr = lsm6dsv16x->accel_batch_odr,
		.gyro_batch_odr = lsm6dsv16x->gyro_batch_odr,
#if defined(CONFIG_LSM6DSV16X_ENABLE_TEMP)
		.temp_batch_odr = lsm6dsv16x->temp_batch_odr,
#endif
		.sflp_batch_odr = lsm6dsv16x->sflp_batch_odr,
	};
	/* clang-format on */

	memcpy(buf, &hdr, sizeof(hdr));
	read_buf = buf + sizeof(hdr);
	buf_avail = buf_len - sizeof(hdr);

	uint8_t reg_addr = lsm6dsv16x_bus_reg(lsm6dsv16x->bus_type, LSM6DSV16X_FIFO_DATA_OUT_TAG);
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
	rtio_read_regs_async(lsm6dsv16x->rtio_ctx, lsm6dsv16x->iodev, lsm6dsv16x->bus_type,
			     &fifo_regs, lsm6dsv16x->streaming_sqe, dev, lsm6dsv16x_complete_op_cb);
}

/*
 * Called by bus driver to complete the LSM6DSV16X_STATUS_REG read op.
 * If drdy_xl is active it reads XL data (6 bytes) from LSM6DSV16X_OUTX_L_A reg.
 */
static void lsm6dsv16x_read_status_cb(struct rtio *r, const struct rtio_sqe *sqe,
				      int result, void *arg)
{
	ARG_UNUSED(result);

	const struct device *dev = arg;
#if LSM6DSVXXX_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
	const struct lsm6dsv16x_config *config = dev->config;
#endif
	struct lsm6dsv16x_data *lsm6dsv16x = dev->data;
	struct rtio *rtio = lsm6dsv16x->rtio_ctx;
	struct gpio_dt_spec *irq_gpio = lsm6dsv16x->drdy_gpio;
	struct sensor_read_config *read_config;

	/* At this point, no sqe request is queued should be considered as a bug */
	__ASSERT_NO_MSG(lsm6dsv16x->streaming_sqe != NULL);

	read_config = (struct sensor_read_config *)lsm6dsv16x->streaming_sqe->sqe.iodev->data;
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
		rtio_iodev_sqe_err(lsm6dsv16x->streaming_sqe, res);
		lsm6dsv16x->streaming_sqe = NULL;
		return;
	}

	if (data_ready != NULL &&
	    (data_ready->opt == SENSOR_STREAM_DATA_NOP ||
	     data_ready->opt == SENSOR_STREAM_DATA_DROP)) {
		uint8_t *buf;
		uint32_t buf_len;

		/* Clear streaming_sqe since we're done with the call */
		if (rtio_sqe_rx_buf(lsm6dsv16x->streaming_sqe, sizeof(struct lsm6dsv16x_rtio_data),
				    sizeof(struct lsm6dsv16x_rtio_data), &buf, &buf_len) != 0) {
			rtio_iodev_sqe_err(lsm6dsv16x->streaming_sqe, -ENOMEM);
			lsm6dsv16x->streaming_sqe = NULL;
			if (!ON_I3C_BUS(config) || (I3C_INT_PIN(config))) {
				gpio_pin_interrupt_configure_dt(irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
			}
			return;
		}

		struct lsm6dsv16x_rtio_data *rx_data = (struct lsm6dsv16x_rtio_data *)buf;

		memset(buf, 0, buf_len);
		rx_data->header.is_fifo = 0;
		rx_data->header.timestamp = lsm6dsv16x->timestamp;
		rx_data->has_accel = 0;
		rx_data->has_gyro = 0;
		rx_data->has_temp = 0;

		/* complete request with ok */
		rtio_iodev_sqe_ok(lsm6dsv16x->streaming_sqe, 0);
		lsm6dsv16x->streaming_sqe = NULL;
		if (!ON_I3C_BUS(config) || (I3C_INT_PIN(config))) {
			gpio_pin_interrupt_configure_dt(irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		}

		return;
	}

	/*
	 * Read XL data
	 *
	 * lsm6dsv16x_data_ready_t drdy;
	 * if (drdy.drdy_xl) {
	 */
	if (lsm6dsv16x->status & 0x1) {
		uint8_t *buf, *read_buf;
		uint32_t buf_len;
		uint32_t req_len = 6 + sizeof(struct lsm6dsv16x_rtio_data);

		if (rtio_sqe_rx_buf(lsm6dsv16x->streaming_sqe,
				    req_len, req_len, &buf, &buf_len) != 0) {
			LOG_ERR("Failed to get buffer");
			rtio_iodev_sqe_err(lsm6dsv16x->streaming_sqe, -ENOMEM);
			lsm6dsv16x->streaming_sqe = NULL;
			if (!ON_I3C_BUS(config) || (I3C_INT_PIN(config))) {
				gpio_pin_interrupt_configure_dt(irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
			}
			return;
		}

		struct lsm6dsv16x_rtio_data hdr = {
			.header = {
				.is_fifo = false,
				.accel_fs_idx = lsm6dsv16x->accel_fs,
				.gyro_fs = lsm6dsv16x->gyro_fs,
				.timestamp = lsm6dsv16x->timestamp,
			},
			.has_accel = 1,
			.has_gyro = 0,
			.has_temp = 0,
		};

		memcpy(buf, &hdr, sizeof(hdr));
		read_buf = (uint8_t *)&((struct lsm6dsv16x_rtio_data *)buf)->acc[0];

		uint8_t reg_addr = lsm6dsv16x_bus_reg(lsm6dsv16x->bus_type, LSM6DSV16X_OUTX_L_A);
		struct rtio_regs fifo_regs;
		struct rtio_regs_list regs_list[] = {
			{
				reg_addr,
				read_buf,
				6,
			},
		};

		fifo_regs.rtio_regs_list = regs_list;
		fifo_regs.rtio_regs_num = ARRAY_SIZE(regs_list);

		/*
		 * Prepare rtio enabled bus to read LSM6DSV16X_OUTX_L_A register
		 * where accelerometer data is available.
		 * Then lsm6dsv16x_complete_op_cb callback will be invoked.
		 *
		 * STMEMSC API equivalent code:
		 *
		 *   uint8_t accel_raw[6];
		 *
		 *   lsm6dsv16x_acceleration_raw_get(&dev_ctx, accel_raw);
		 */
		rtio_read_regs_async(lsm6dsv16x->rtio_ctx, lsm6dsv16x->iodev, lsm6dsv16x->bus_type,
				     &fifo_regs, lsm6dsv16x->streaming_sqe, dev,
				     lsm6dsv16x_complete_op_cb);
	}
}

/*
 * Called when one of the following trigger is active:
 *
 *     - int_fifo_th (SENSOR_TRIG_FIFO_WATERMARK)
 *     - int_fifo_full (SENSOR_TRIG_FIFO_FULL)
 *     - int_drdy (SENSOR_TRIG_DATA_READY)
 */
void lsm6dsv16x_stream_irq_handler(const struct device *dev)
{
	struct lsm6dsv16x_data *lsm6dsv16x = dev->data;
#if LSM6DSVXXX_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
	const struct lsm6dsv16x_config *config = dev->config;
	struct rtio *rtio = lsm6dsv16x->rtio_ctx;
#endif
	uint64_t cycles;
	int rc;

	if (lsm6dsv16x->streaming_sqe == NULL) {
		return;
	}

	rc = sensor_clock_get_cycles(&cycles);
	if (rc != 0) {
		LOG_ERR("Failed to get sensor clock cycles");
		rtio_iodev_sqe_err(lsm6dsv16x->streaming_sqe, rc);
		return;
	}

	/* get timestamp as soon as the irq is served */
	lsm6dsv16x->timestamp = sensor_clock_cycles_to_ns(cycles);

	/* handle FIFO triggers */
	if (lsm6dsv16x->trig_cfg.int_fifo_th || lsm6dsv16x->trig_cfg.int_fifo_full) {
#if LSM6DSVXXX_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
		if (ON_I3C_BUS(config) && (!I3C_INT_PIN(config))) {
			/*
			 * If we are on an I3C bus, then it should be expected that the fifo status
			 * was already received in the IBI payload and we don't need to read it
			 * again.
			 */
			lsm6dsv16x->fifo_status[0] = lsm6dsv16x->ibi_payload.fifo_status1;
			lsm6dsv16x->fifo_status[1] = lsm6dsv16x->ibi_payload.fifo_status2;

			struct rtio_sqe *check_fifo_status_reg = rtio_sqe_acquire(rtio);

			rtio_sqe_prep_callback_no_cqe(check_fifo_status_reg,
						      lsm6dsv16x_read_fifo_cb, (void *)dev, NULL);
			rtio_submit(rtio, 0);
		} else {
#endif
			lsm6dsv16x->fifo_status[0] = lsm6dsv16x->fifo_status[1] = 0;

			uint8_t reg_addr =
				lsm6dsv16x_bus_reg(lsm6dsv16x->bus_type, LSM6DSV16X_FIFO_STATUS1);
			struct rtio_regs fifo_regs;
			struct rtio_regs_list regs_list[] = {
				{
					reg_addr,
					lsm6dsv16x->fifo_status,
					2,
				},
			};

			fifo_regs.rtio_regs_list = regs_list;
			fifo_regs.rtio_regs_num = ARRAY_SIZE(regs_list);

			/*
			 * Prepare rtio enabled bus to read LSM6DSV16X_FIFO_STATUS1 and
			 * LSM6DSV16X_FIFO_STATUS2 registers where FIFO threshold condition and
			 * count are reported. Then lsm6dsv16x_read_fifo_cb callback will be
			 * invoked.
			 *
			 * STMEMSC API equivalent code:
			 *
			 *   lsm6dsv16x_fifo_status_t fifo_status;
			 *
			 *   lsm6dsv16x_fifo_status_get(&dev_ctx, &fifo_status);
			 */
			rtio_read_regs_async(lsm6dsv16x->rtio_ctx, lsm6dsv16x->iodev,
					     lsm6dsv16x->bus_type, &fifo_regs,
					     lsm6dsv16x->streaming_sqe, dev,
					     lsm6dsv16x_read_fifo_cb);

#if LSM6DSVXXX_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
		}
#endif
	}

	/* handle drdy trigger */
	if (lsm6dsv16x->trig_cfg.int_drdy) {
		lsm6dsv16x->status = 0;

		uint8_t reg_addr = lsm6dsv16x_bus_reg(lsm6dsv16x->bus_type, LSM6DSV16X_STATUS_REG);
		struct rtio_regs fifo_regs;
		struct rtio_regs_list regs_list[] = {
			{
				reg_addr,
				&lsm6dsv16x->status,
				1,
			},
		};

		fifo_regs.rtio_regs_list = regs_list;
		fifo_regs.rtio_regs_num = ARRAY_SIZE(regs_list);

		/*
		 * Prepare rtio enabled bus to read LSM6DSV16X_STATUS_REG register
		 * where accelerometer and gyroscope data ready status is available.
		 * Then lsm6dsv16x_read_status_cb callback will be invoked.
		 *
		 * STMEMSC API equivalent code:
		 *
		 *   lsm6dsv16x_data_ready_t drdy;
		 *
		 *   lsm6dsv16x_flag_data_ready_get(&dev_ctx, &drdy);
		 */
		rtio_read_regs_async(lsm6dsv16x->rtio_ctx, lsm6dsv16x->iodev, lsm6dsv16x->bus_type,
				     &fifo_regs, lsm6dsv16x->streaming_sqe, dev,
				     lsm6dsv16x_read_status_cb);
	}
}
