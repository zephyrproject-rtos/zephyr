/*
 * Copyright (c) 2024 Cienet
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bma4xx

#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor_clock.h>

#include "bma4xx.h"
#include "bma4xx_defs.h"
#include "bma4xx_decoder.h"
#include "bma4xx_rtio.h"

#ifdef CONFIG_BMA4XX_STREAM

LOG_MODULE_DECLARE(bma4xx, CONFIG_SENSOR_LOG_LEVEL);

void bma4xx_submit_stream(const struct device *sensor, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	struct bma4xx_data *data = sensor->data;
	struct bma4xx_runtime_config new_config = data->cfg;
	const struct bma4xx_config *cfg_bma4xx = sensor->config;
	int ret;

	ret = gpio_pin_interrupt_configure_dt(&cfg_bma4xx->gpio_interrupt, GPIO_INT_DISABLE);
	if (ret) {
		LOG_DBG("Failed to disable interrupt");
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}

	new_config.interrupt1_fifo_wm = false;
	new_config.interrupt1_fifo_full = false;
	/* Only one trigger will be sent per time */
	for (int i = 0; i < cfg->count; ++i) {
		switch (cfg->triggers[i].trigger) {
		case SENSOR_TRIG_FIFO_WATERMARK:
			new_config.interrupt1_fifo_wm = true;
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

	if (new_config.interrupt1_fifo_wm != data->cfg.interrupt1_fifo_wm ||
	    new_config.interrupt1_fifo_full != data->cfg.interrupt1_fifo_full) {

		ret = bma4xx_safely_configure(sensor, &new_config);

		if (ret != 0) {
			LOG_ERR("Failed to configure sensor");
			rtio_iodev_sqe_err(iodev_sqe, ret);
			return;
		}
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg_bma4xx->gpio_interrupt, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret) {
		LOG_DBG("Failed to set interrupt");
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}

	data->streaming_sqe = iodev_sqe;
}

static struct sensor_stream_trigger *
bma4xx_get_read_config_trigger(const struct sensor_read_config *cfg, enum sensor_trigger_type trig)
{
	for (int i = 0; i < cfg->count; ++i) {
		if (cfg->triggers[i].trigger == trig) {
			return &cfg->triggers[i];
		}
	}
	LOG_DBG("Unsupported trigger (%d)", trig);
	return NULL;
}

static void bma4xx_complete_cb(struct rtio *r, const struct rtio_sqe *sqe, void *arg)
{
	const struct device *dev = arg;
	struct bma4xx_data *drv_data = dev->data;
	const struct bma4xx_config *drv_cfg = dev->config;
	struct rtio_iodev_sqe *iodev_sqe = sqe->userdata;

	rtio_iodev_sqe_ok(iodev_sqe, drv_data->fifo_count);

	gpio_pin_interrupt_configure_dt(&drv_cfg->gpio_interrupt, GPIO_INT_EDGE_TO_ACTIVE);
}

static void bma4xx_fifo_count_cb(struct rtio *r, const struct rtio_sqe *sqe, void *arg)
{
	const struct device *dev = arg;
	struct bma4xx_data *drv_data = dev->data;
	const struct bma4xx_config *drv_cfg = dev->config;
	struct rtio_iodev *iodev = drv_data->iodev;
	uint8_t *fifo_count_buf = (uint8_t *)&drv_data->fifo_count;
	uint16_t fifo_count = (((fifo_count_buf[1] & 0x3F) << 8) | fifo_count_buf[0]);

	drv_data->fifo_count = fifo_count;

	/* Pull a operation from our device iodev queue, validated to only be reads */
	struct rtio_iodev_sqe *iodev_sqe = drv_data->streaming_sqe;

	drv_data->streaming_sqe = NULL;

	/* Not inherently an underrun/overrun as we may have a buffer to fill next time */
	if (iodev_sqe == NULL) {
		LOG_DBG("No pending SQE");
		gpio_pin_interrupt_configure_dt(&drv_cfg->gpio_interrupt, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	const size_t packet_size = (drv_data->cfg.accel_pwr_mode && drv_data->cfg.aux_pwr_mode)
					   ? BMA4XX_FIFO_MA_LENGTH + BMA4XX_FIFO_HEADER_LENGTH
					   : BMA4XX_FIFO_A_LENGTH + BMA4XX_FIFO_HEADER_LENGTH;
	const size_t min_read_size = sizeof(struct bma4xx_fifo_data) + packet_size;
	const size_t ideal_read_size = sizeof(struct bma4xx_fifo_data) + fifo_count;
	uint8_t *buf;
	uint32_t buf_len;

	if (rtio_sqe_rx_buf(iodev_sqe, min_read_size, ideal_read_size, &buf, &buf_len) != 0) {
		LOG_ERR("Failed to get buffer");
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}
	LOG_DBG("Requesting buffer [%u, %u] got %u", (unsigned int)min_read_size,
		(unsigned int)ideal_read_size, buf_len);

	/* Read FIFO and call back to rtio with rtio_sqe completion */
	struct bma4xx_fifo_data hdr;

	hdr.header.is_fifo = true;
	hdr.header.accel_fs = drv_data->cfg.accel_fs_range;
	hdr.header.timestamp = drv_data->timestamp;
	hdr.int_status = drv_data->int_status;
	hdr.accel_odr = drv_data->cfg.accel_odr;

	uint32_t buf_avail = buf_len;

	memcpy(buf, &hdr, sizeof(hdr));
	buf_avail -= sizeof(hdr);

	uint32_t read_len = MIN(fifo_count, buf_avail);
	uint32_t pkts = read_len / packet_size;

	read_len = pkts * packet_size;
	((struct bma4xx_fifo_data *)buf)->fifo_count = read_len;

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
	struct rtio_sqe *read_fifo_data = rtio_sqe_acquire(r);
	struct rtio_sqe *complete_op = rtio_sqe_acquire(r);
	const uint8_t reg_addr = BMA4XX_REG_FIFO_DATA;

	rtio_sqe_prep_tiny_write(write_fifo_addr, iodev, RTIO_PRIO_NORM, &reg_addr, 1, NULL);
	write_fifo_addr->flags = RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_fifo_data, iodev, RTIO_PRIO_NORM, read_buf, read_len, iodev_sqe);
	read_fifo_data->flags = RTIO_SQE_CHAINED;

	if (drv_cfg->bus_type == BMA4XX_BUS_I2C) {
		read_fifo_data->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
	}

	rtio_sqe_prep_callback(complete_op, bma4xx_complete_cb, (void *)dev, iodev_sqe);

	rtio_submit(r, 0);
}

static void bma4xx_int_status_cb(struct rtio *r, const struct rtio_sqe *sqr, void *arg)
{
	const struct device *dev = arg;
	struct bma4xx_data *drv_data = dev->data;
	const struct bma4xx_config *drv_cfg = dev->config;
	struct rtio_iodev *iodev = drv_data->iodev;
	struct rtio_iodev_sqe *streaming_sqe = drv_data->streaming_sqe;
	struct sensor_read_config *read_config;

	if (streaming_sqe == NULL) {
		return;
	}

	read_config = (struct sensor_read_config *)streaming_sqe->sqe.iodev->data;
	__ASSERT_NO_MSG(read_config != NULL);

	if (!read_config->is_streaming) {
		return;
	}

	gpio_pin_interrupt_configure_dt(&drv_cfg->gpio_interrupt, GPIO_INT_DISABLE);

	struct sensor_stream_trigger *fifo_wm_cfg =
		bma4xx_get_read_config_trigger(read_config, SENSOR_TRIG_FIFO_WATERMARK);
	bool has_fifo_wm_trig = fifo_wm_cfg != NULL &&
				FIELD_GET(BMA4XX_BIT_INT_STAT_1_FWM_INT, drv_data->int_status) != 0;

	struct sensor_stream_trigger *fifo_full_cfg =
		bma4xx_get_read_config_trigger(read_config, SENSOR_TRIG_FIFO_FULL);
	bool has_fifo_full_trig =
		fifo_full_cfg != NULL &&
		FIELD_GET(BMA4XX_BIT_INT_STAT_1_FFULL_INT, drv_data->int_status) != 0;

	if (!has_fifo_wm_trig && !has_fifo_full_trig) {
		gpio_pin_interrupt_configure_dt(&drv_cfg->gpio_interrupt, GPIO_INT_EDGE_TO_ACTIVE);
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

	if (has_fifo_wm_trig && !has_fifo_full_trig) {
		/* Only care about fifo threshold */
		data_opt = fifo_wm_cfg->opt;
	} else if (!has_fifo_wm_trig && has_fifo_full_trig) {
		/* Only care about fifo full */
		data_opt = fifo_full_cfg->opt;
	} else {
		/* Both fifo threshold and full */
		data_opt = MIN(fifo_wm_cfg->opt, fifo_full_cfg->opt);
	}

	if (data_opt == SENSOR_STREAM_DATA_NOP || data_opt == SENSOR_STREAM_DATA_DROP) {
		uint8_t *buf;
		uint32_t buf_len;

		/* Clear streaming_sqe since we're done with the call */
		drv_data->streaming_sqe = NULL;
		if (rtio_sqe_rx_buf(streaming_sqe, sizeof(struct bma4xx_fifo_data),
				    sizeof(struct bma4xx_fifo_data), &buf, &buf_len) != 0) {
			rtio_iodev_sqe_err(streaming_sqe, -ENOMEM);
			return;
		}

		struct bma4xx_fifo_data *data = (struct bma4xx_fifo_data *)buf;

		memset(buf, 0, buf_len);
		data->header.timestamp = drv_data->timestamp;
		data->int_status = drv_data->int_status;
		data->fifo_count = 0;
		data->header.is_fifo = true;
		rtio_iodev_sqe_ok(streaming_sqe, 0);
		gpio_pin_interrupt_configure_dt(&drv_cfg->gpio_interrupt, GPIO_INT_EDGE_TO_ACTIVE);
		if (data_opt == SENSOR_STREAM_DATA_DROP) {
			/* Flush the FIFO */
			struct rtio_sqe *write_signal_path_reset = rtio_sqe_acquire(r);
			uint8_t write_buffer[] = {
				FIELD_GET(BMA4XX_REG_ADDRESS_MASK, BMA4XX_REG_CMD),
				BMA4XX_CMD_FIFO_FLUSH,
			};

			rtio_sqe_prep_tiny_write(write_signal_path_reset, iodev, RTIO_PRIO_NORM,
						 write_buffer, ARRAY_SIZE(write_buffer), NULL);
			rtio_submit(r, 0);
			ARG_UNUSED(rtio_cqe_consume(r));
		}
		return;
	}

	/* We need the data, read the fifo length */
	struct rtio_sqe *write_fifo_count_reg = rtio_sqe_acquire(r);
	struct rtio_sqe *read_fifo_count = rtio_sqe_acquire(r);
	struct rtio_sqe *check_fifo_count = rtio_sqe_acquire(r);
	uint8_t reg = BMA4XX_REG_FIFO_LENGTH_0;
	uint8_t *read_buf = (uint8_t *)&drv_data->fifo_count;

	rtio_sqe_prep_tiny_write(write_fifo_count_reg, iodev, RTIO_PRIO_NORM, &reg, 1, NULL);
	write_fifo_count_reg->flags = RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_fifo_count, iodev, RTIO_PRIO_NORM, read_buf,
			   BMA4XX_FIFO_DATA_LENGTH, NULL);
	read_fifo_count->flags = RTIO_SQE_CHAINED;

	if (drv_cfg->bus_type == BMA4XX_BUS_I2C) {
		read_fifo_count->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
	}

	rtio_sqe_prep_callback(check_fifo_count, bma4xx_fifo_count_cb, arg, NULL);

	rtio_submit(r, 0);
}

void bma4xx_fifo_event(const struct device *dev)
{
	struct bma4xx_data *drv_data = dev->data;
	const struct bma4xx_config *drv_cfg = dev->config;
	struct rtio_iodev *iodev = drv_data->iodev;
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

	struct rtio_sqe *write_int_reg = rtio_sqe_acquire(r);
	struct rtio_sqe *read_int_reg = rtio_sqe_acquire(r);
	struct rtio_sqe *check_int_status = rtio_sqe_acquire(r);
	uint8_t reg = BMA4XX_REG_INT_STAT_1;

	rtio_sqe_prep_tiny_write(write_int_reg, iodev, RTIO_PRIO_NORM, &reg, 1, NULL);
	write_int_reg->flags = RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_int_reg, iodev, RTIO_PRIO_NORM, &drv_data->int_status, 1, NULL);
	read_int_reg->flags = RTIO_SQE_CHAINED;

	if (drv_cfg->bus_type == BMA4XX_BUS_I2C) {
		read_int_reg->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
	}

	rtio_sqe_prep_callback(check_int_status, bma4xx_int_status_cb, (void *)dev, NULL);
	rtio_submit(r, 0);
}

#endif /* CONFIG_BMA4XX_STREAM */
