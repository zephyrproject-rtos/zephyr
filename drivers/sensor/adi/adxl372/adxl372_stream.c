/*
 * Copyright (c) 2024 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_clock.h>

#include "adxl372.h"

LOG_MODULE_DECLARE(ADXL372, CONFIG_SENSOR_LOG_LEVEL);

static void adxl372_irq_en_cb(struct rtio *r, const struct rtio_sqe *sqr, void *arg)
{
	const struct device *dev = (const struct device *)arg;
	const struct adxl372_dev_config *cfg = dev->config;

	gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_EDGE_TO_ACTIVE);
}

static void adxl372_fifo_flush_rtio(const struct device *dev)
{
	struct adxl372_data *data = dev->data;
	uint8_t pow_reg = data->pwr_reg;
	const struct adxl372_dev_config *cfg = dev->config;
	uint8_t fifo_config;

	pow_reg &= ~ADXL372_POWER_CTL_MODE_MSK;
	pow_reg |= ADXL372_POWER_CTL_MODE(ADXL372_STANDBY);

	struct rtio_sqe *sqe = rtio_sqe_acquire(data->rtio_ctx);
	const uint8_t reg_addr_w[2] = {ADXL372_REG_WRITE(ADXL372_POWER_CTL), pow_reg};

	rtio_sqe_prep_tiny_write(sqe, data->iodev, RTIO_PRIO_NORM, reg_addr_w, 2, NULL);

	fifo_config = (ADXL372_FIFO_CTL_FORMAT_MODE(data->fifo_config.fifo_format) |
		       ADXL372_FIFO_CTL_MODE_MODE(ADXL372_FIFO_BYPASSED) |
		       ADXL372_FIFO_CTL_SAMPLES_MODE(data->fifo_config.fifo_samples));

	sqe = rtio_sqe_acquire(data->rtio_ctx);
	const uint8_t reg_addr_w2[2] = {ADXL372_REG_WRITE(ADXL372_FIFO_CTL), fifo_config};

	rtio_sqe_prep_tiny_write(sqe, data->iodev, RTIO_PRIO_NORM, reg_addr_w2, 2, NULL);

	fifo_config = (ADXL372_FIFO_CTL_FORMAT_MODE(data->fifo_config.fifo_format) |
		       ADXL372_FIFO_CTL_MODE_MODE(data->fifo_config.fifo_mode) |
		       ADXL372_FIFO_CTL_SAMPLES_MODE(data->fifo_config.fifo_samples));

	sqe = rtio_sqe_acquire(data->rtio_ctx);
	const uint8_t reg_addr_w3[2] = {ADXL372_REG_WRITE(ADXL372_FIFO_CTL), fifo_config};

	rtio_sqe_prep_tiny_write(sqe, data->iodev, RTIO_PRIO_NORM, reg_addr_w3, 2, NULL);

	pow_reg = data->pwr_reg;

	pow_reg &= ~ADXL372_POWER_CTL_MODE_MSK;
	pow_reg |= ADXL372_POWER_CTL_MODE(cfg->op_mode);

	sqe = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *complete_op = rtio_sqe_acquire(data->rtio_ctx);
	const uint8_t reg_addr_w4[2] = {ADXL372_REG_WRITE(ADXL372_POWER_CTL), pow_reg};

	rtio_sqe_prep_tiny_write(sqe, data->iodev, RTIO_PRIO_NORM, reg_addr_w4, 2, NULL);
	sqe->flags |= RTIO_SQE_CHAINED;
	rtio_sqe_prep_callback(complete_op, adxl372_irq_en_cb, (void *)dev, NULL);
	rtio_submit(data->rtio_ctx, 0);
}

void adxl372_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg =
		(const struct sensor_read_config *)iodev_sqe->sqe.iodev->data;
	struct adxl372_data *data = (struct adxl372_data *)dev->data;
	const struct adxl372_dev_config *cfg_372 = dev->config;
	uint8_t int_value = (uint8_t)~ADXL372_INT1_MAP_FIFO_FULL_MSK;
	uint8_t fifo_full_irq = 0;

	int rc = gpio_pin_interrupt_configure_dt(&cfg_372->interrupt, GPIO_INT_DISABLE);

	if (rc < 0) {
		return;
	}

	for (size_t i = 0; i < cfg->count; i++) {
		if ((cfg->triggers[i].trigger == SENSOR_TRIG_FIFO_WATERMARK) ||
		    (cfg->triggers[i].trigger == SENSOR_TRIG_FIFO_FULL)) {
			int_value = ADXL372_INT1_MAP_FIFO_FULL_MSK;
			fifo_full_irq = 1;
		}
	}

	if (fifo_full_irq != data->fifo_full_irq) {
		data->fifo_full_irq = fifo_full_irq;

		rc = data->hw_tf->write_reg_mask(dev, ADXL372_INT1_MAP,
			ADXL372_INT1_MAP_FIFO_FULL_MSK, int_value);
		if (rc < 0) {
			return;
		}

		/* Flush the FIFO by disabling it. Save current mode for after the reset. */
		enum adxl372_fifo_mode current_fifo_mode = data->fifo_config.fifo_mode;

		adxl372_configure_fifo(dev, ADXL372_FIFO_BYPASSED, data->fifo_config.fifo_format,
					data->fifo_config.fifo_samples);

		if (current_fifo_mode == ADXL372_FIFO_BYPASSED) {
			current_fifo_mode = ADXL372_FIFO_STREAMED;
		}

		adxl372_configure_fifo(dev, current_fifo_mode, data->fifo_config.fifo_format,
					data->fifo_config.fifo_samples);

		adxl372_set_op_mode(dev, cfg_372->op_mode);
	}

	rc = gpio_pin_interrupt_configure_dt(&cfg_372->interrupt, GPIO_INT_EDGE_TO_ACTIVE);
	if (rc < 0) {
		return;
	}

	data->sqe = iodev_sqe;
}

static void adxl372_fifo_read_cb(struct rtio *rtio_ctx, const struct rtio_sqe *sqe, void *arg)
{
	const struct device *dev = (const struct device *)arg;
	const struct adxl372_dev_config *cfg = (const struct adxl372_dev_config *)dev->config;
	struct rtio_iodev_sqe *iodev_sqe = sqe->userdata;

	rtio_iodev_sqe_ok(iodev_sqe, 0);

	gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_EDGE_TO_ACTIVE);
}

size_t adxl372_get_packet_size(const struct adxl372_dev_config *cfg)
{
	/* If one sample contains XYZ values. */
	size_t packet_size;

	switch (cfg->fifo_config.fifo_format) {
	case ADXL372_X_FIFO:
	case ADXL372_Y_FIFO:
	case ADXL372_Z_FIFO:
		packet_size = 2;
		break;

	case ADXL372_XY_FIFO:
	case ADXL372_XZ_FIFO:
	case ADXL372_YZ_FIFO:
		packet_size = 4;
		break;

	default:
		packet_size = 6;
		break;
	}

	return packet_size;
}

static void adxl372_process_fifo_samples_cb(struct rtio *r, const struct rtio_sqe *sqr, void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct adxl372_data *data = (struct adxl372_data *)dev->data;
	const struct adxl372_dev_config *cfg = (const struct adxl372_dev_config *)dev->config;
	struct rtio_iodev_sqe *current_sqe = data->sqe;
	uint16_t fifo_samples = (((data->fifo_ent[0] & 0x3) << 8) | data->fifo_ent[1]);
	size_t sample_set_size = adxl372_get_packet_size(cfg);

	/* At least one sample set must remain in FIFO to encure that data
	 * is not overwritten and stored out of order.
	 */
	if (fifo_samples > sample_set_size / 2) {
		fifo_samples -= sample_set_size / 2;
	} else {
		LOG_ERR("fifo sample count error %d\n", fifo_samples);
		gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	uint16_t fifo_bytes = fifo_samples * 2 /*sample size*/;

	data->sqe = NULL;

	/* Not inherently an underrun/overrun as we may have a buffer to fill next time */
	if (current_sqe == NULL) {
		LOG_ERR("No pending SQE");
		gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	const size_t min_read_size = sizeof(struct adxl372_fifo_data) + sample_set_size;
	const size_t ideal_read_size = sizeof(struct adxl372_fifo_data) + fifo_bytes;

	uint8_t *buf;
	uint32_t buf_len;

	if (rtio_sqe_rx_buf(current_sqe, min_read_size, ideal_read_size, &buf, &buf_len) != 0) {
		LOG_ERR("Failed to get buffer");
		rtio_iodev_sqe_err(current_sqe, -ENOMEM);
		gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	LOG_DBG("Requesting buffer [%u, %u] got %u", (unsigned int)min_read_size,
		(unsigned int)ideal_read_size, buf_len);

	/* Read FIFO and call back to rtio with rtio_sqe completion */
	struct adxl372_fifo_data *hdr = (struct adxl372_fifo_data *)buf;

	hdr->is_fifo = 1;
	hdr->timestamp = data->timestamp;
	hdr->int_status = data->status1;
	hdr->accel_odr = data->odr;
	hdr->sample_set_size = sample_set_size;

	if ((cfg->fifo_config.fifo_format == ADXL372_X_FIFO) ||
	    (cfg->fifo_config.fifo_format == ADXL372_XY_FIFO) ||
	    (cfg->fifo_config.fifo_format == ADXL372_XZ_FIFO) ||
	    (cfg->fifo_config.fifo_format == ADXL372_XYZ_FIFO)) {
		hdr->has_x = 1;
	}

	if ((cfg->fifo_config.fifo_format == ADXL372_Y_FIFO) ||
	    (cfg->fifo_config.fifo_format == ADXL372_XY_FIFO) ||
	    (cfg->fifo_config.fifo_format == ADXL372_YZ_FIFO) ||
	    (cfg->fifo_config.fifo_format == ADXL372_XYZ_FIFO))	{
		hdr->has_y = 1;
	}

	if ((cfg->fifo_config.fifo_format == ADXL372_Z_FIFO) ||
	    (cfg->fifo_config.fifo_format == ADXL372_XZ_FIFO) ||
	    (cfg->fifo_config.fifo_format == ADXL372_YZ_FIFO) ||
	    (cfg->fifo_config.fifo_format == ADXL372_XYZ_FIFO))	{
		hdr->has_z = 1;
	}

	uint32_t buf_avail = buf_len;

	buf_avail -= sizeof(*hdr);

	uint32_t read_len = MIN(fifo_bytes, buf_avail);
	uint32_t pkts = read_len / sample_set_size;

	read_len = pkts * sample_set_size;

	((struct adxl372_fifo_data *)buf)->fifo_byte_count = read_len;

	__ASSERT_NO_MSG(read_len % sample_set_size == 0);

	uint8_t *read_buf = buf + sizeof(*hdr);

	/* Flush completions */
	struct rtio_cqe *cqe;
	int res = 0;

	do {
		cqe = rtio_cqe_consume(data->rtio_ctx);
		if (cqe != NULL) {
			if ((cqe->result < 0 && res == 0)) {
				LOG_ERR("Bus error: %d", cqe->result);
				res = cqe->result;
			}
			rtio_cqe_release(data->rtio_ctx, cqe);
		}
	} while (cqe != NULL);

	/* Bail/cancel attempt to read sensor on any error */
	if (res != 0) {
		rtio_iodev_sqe_err(current_sqe, res);
		return;
	}

	/* Setup new rtio chain to read the fifo data and report then check the
	 * result
	 */
	struct rtio_sqe *write_fifo_addr = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *read_fifo_data = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *complete_op = rtio_sqe_acquire(data->rtio_ctx);
	const uint8_t reg_addr = ADXL372_REG_READ(ADXL372_FIFO_DATA);

	rtio_sqe_prep_tiny_write(write_fifo_addr, data->iodev, RTIO_PRIO_NORM, &reg_addr, 1, NULL);
	write_fifo_addr->flags = RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_fifo_data, data->iodev, RTIO_PRIO_NORM, read_buf, read_len,
			   current_sqe);
	read_fifo_data->flags = RTIO_SQE_CHAINED;
	rtio_sqe_prep_callback(complete_op, adxl372_fifo_read_cb, (void *)dev, current_sqe);

	rtio_submit(data->rtio_ctx, 0);
}

static void adxl372_process_status1_cb(struct rtio *r, const struct rtio_sqe *sqr, void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct adxl372_data *data = (struct adxl372_data *)dev->data;
	const struct adxl372_dev_config *cfg = (const struct adxl372_dev_config *)dev->config;
	struct rtio_iodev_sqe *current_sqe = data->sqe;
	struct sensor_read_config *read_config;
	uint8_t status1 = data->status1;

	if (data->sqe == NULL) {
		return;
	}

	read_config = (struct sensor_read_config *)data->sqe->sqe.iodev->data;

	if (read_config == NULL) {
		return;
	}

	if (read_config->is_streaming == false) {
		return;
	}

	gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_DISABLE);

	struct sensor_stream_trigger *fifo_wmark_cfg = NULL;
	struct sensor_stream_trigger *fifo_full_cfg = NULL;

	for (int i = 0; i < read_config->count; ++i) {
		if (read_config->triggers[i].trigger == SENSOR_TRIG_FIFO_WATERMARK) {
			fifo_wmark_cfg = &read_config->triggers[i];
			continue;
		}

		if (read_config->triggers[i].trigger == SENSOR_TRIG_FIFO_FULL) {
			fifo_full_cfg = &read_config->triggers[i];
			continue;
		}
	}

	bool fifo_full_irq = false;

	if ((fifo_wmark_cfg != NULL) && (fifo_full_cfg != NULL) &&
	    FIELD_GET(ADXL372_INT1_MAP_FIFO_FULL_MSK, status1)) {
		fifo_full_irq = true;
	}

	if (!fifo_full_irq) {
		gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	struct rtio_cqe *cqe;
	int res = 0;

	do {
		cqe = rtio_cqe_consume(data->rtio_ctx);
		if (cqe != NULL) {
			if ((cqe->result < 0) && (res == 0)) {
				LOG_ERR("Bus error: %d", cqe->result);
				res = cqe->result;
			}
			rtio_cqe_release(data->rtio_ctx, cqe);
		}
	} while (cqe != NULL);

	/* Bail/cancel attempt to read sensor on any error */
	if (res != 0) {
		rtio_iodev_sqe_err(current_sqe, res);
		return;
	}

	enum sensor_stream_data_opt data_opt;

	if ((fifo_wmark_cfg != NULL) && (fifo_full_cfg == NULL)) {
		data_opt = fifo_wmark_cfg->opt;
	} else if ((fifo_wmark_cfg == NULL) && (fifo_full_cfg != NULL)) {
		data_opt = fifo_full_cfg->opt;
	} else {
		data_opt = MIN(fifo_wmark_cfg->opt, fifo_full_cfg->opt);
	}

	if (data_opt == SENSOR_STREAM_DATA_NOP || data_opt == SENSOR_STREAM_DATA_DROP) {
		uint8_t *buf;
		uint32_t buf_len;

		/* Clear streaming_sqe since we're done with the call */
		data->sqe = NULL;
		if (rtio_sqe_rx_buf(current_sqe, sizeof(struct adxl372_fifo_data),
				    sizeof(struct adxl372_fifo_data), &buf, &buf_len) != 0) {
			rtio_iodev_sqe_err(current_sqe, -ENOMEM);
			gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_EDGE_TO_ACTIVE);
			return;
		}

		struct adxl372_fifo_data *rx_data = (struct adxl372_fifo_data *)buf;

		memset(buf, 0, buf_len);
		rx_data->is_fifo = 1;
		rx_data->timestamp = data->timestamp;
		rx_data->int_status = status1;
		rx_data->fifo_byte_count = 0;
		rtio_iodev_sqe_ok(current_sqe, 0);

		if (data_opt == SENSOR_STREAM_DATA_DROP) {
			/* Flush the FIFO by disabling it. Save current mode for after the reset. */
			adxl372_fifo_flush_rtio(dev);
			return;
		}

		gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	struct rtio_sqe *write_fifo_addr = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *read_fifo_data = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *complete_op = rtio_sqe_acquire(data->rtio_ctx);
	const uint8_t reg_addr = ADXL372_REG_READ(ADXL372_FIFO_ENTRIES_2);

	rtio_sqe_prep_tiny_write(write_fifo_addr, data->iodev, RTIO_PRIO_NORM, &reg_addr, 1, NULL);
	write_fifo_addr->flags = RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_fifo_data, data->iodev, RTIO_PRIO_NORM, data->fifo_ent, 2,
			   current_sqe);
	read_fifo_data->flags = RTIO_SQE_CHAINED;
	rtio_sqe_prep_callback(complete_op, adxl372_process_fifo_samples_cb, (void *)dev,
			   current_sqe);

	rtio_submit(data->rtio_ctx, 0);
}

void adxl372_stream_irq_handler(const struct device *dev)
{
	struct adxl372_data *data = (struct adxl372_data *)dev->data;
	uint64_t cycles;
	int rc;
	if (data->sqe == NULL) {
		return;
	}

	rc = sensor_clock_get_cycles(&cycles);
	if (rc != 0) {
		LOG_ERR("Failed to get sensor clock cycles");
		rtio_iodev_sqe_err(data->sqe, rc);
		return;
	}

	data->timestamp = sensor_clock_cycles_to_ns(cycles);

	struct rtio_sqe *write_status_addr = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *read_status_reg = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *check_status_reg = rtio_sqe_acquire(data->rtio_ctx);
	uint8_t reg = ADXL372_REG_READ(ADXL372_STATUS_1);

	rtio_sqe_prep_tiny_write(write_status_addr, data->iodev, RTIO_PRIO_NORM, &reg, 1, NULL);
	write_status_addr->flags = RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_status_reg, data->iodev, RTIO_PRIO_NORM, &data->status1, 1, NULL);
	read_status_reg->flags = RTIO_SQE_CHAINED;
	rtio_sqe_prep_callback(check_status_reg, adxl372_process_status1_cb, (void *)dev, NULL);
	rtio_submit(data->rtio_ctx, 0);
}
