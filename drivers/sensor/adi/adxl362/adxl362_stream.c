/*
 * Copyright (c) 2024 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>

#include "adxl362.h"

LOG_MODULE_DECLARE(ADXL362, CONFIG_SENSOR_LOG_LEVEL);

static void adxl362_irq_en_cb(struct rtio *r, const struct rtio_sqe *sqr, void *arg)
{
	const struct device *dev = (const struct device *)arg;
	const struct adxl362_config *cfg = dev->config;

	gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_EDGE_TO_ACTIVE);
}

static void adxl362_fifo_flush_rtio(const struct device *dev)
{
	struct adxl362_data *data = dev->data;

	uint8_t fifo_config = ADXL362_FIFO_CTL_FIFO_MODE(ADXL362_FIFO_DISABLE);
	struct rtio_sqe *write_fifo_addr = rtio_sqe_acquire(data->rtio_ctx);
	const uint8_t reg_addr_w[3] = {ADXL362_WRITE_REG, ADXL362_REG_FIFO_CTL, fifo_config};

	rtio_sqe_prep_tiny_write(write_fifo_addr, data->iodev, RTIO_PRIO_NORM, reg_addr_w, 3, NULL);

	fifo_config = ADXL362_FIFO_CTL_FIFO_MODE(data->fifo_mode) |
		   (data->en_temp_read * ADXL362_FIFO_CTL_FIFO_TEMP);
	if (data->water_mark_lvl & 0x100) {
		fifo_config |= ADXL362_FIFO_CTL_AH;
	}
	write_fifo_addr = rtio_sqe_acquire(data->rtio_ctx);
	const uint8_t reg_addr_w2[3] = {ADXL362_WRITE_REG, ADXL362_REG_FIFO_CTL, fifo_config};

	rtio_sqe_prep_tiny_write(write_fifo_addr, data->iodev, RTIO_PRIO_NORM,
		reg_addr_w2, 3, NULL);
	write_fifo_addr->flags |= RTIO_SQE_CHAINED;

	struct rtio_sqe *complete_op = rtio_sqe_acquire(data->rtio_ctx);

	rtio_sqe_prep_callback(complete_op, adxl362_irq_en_cb, (void *)dev, NULL);
	rtio_submit(data->rtio_ctx, 0);
}

void adxl362_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg =
			(const struct sensor_read_config *)iodev_sqe->sqe.iodev->data;
	struct adxl362_data *data = (struct adxl362_data *)dev->data;
	const struct adxl362_config *cfg_362 = dev->config;
	uint8_t int_mask = 0;
	uint8_t int_value = 0;
	uint8_t fifo_wmark_irq = 0;
	uint8_t fifo_full_irq = 0;

	int rc = gpio_pin_interrupt_configure_dt(&cfg_362->interrupt,
					      GPIO_INT_DISABLE);
	if (rc < 0) {
		return;
	}

	for (size_t i = 0; i < cfg->count; i++) {
		if (cfg->triggers[i].trigger == SENSOR_TRIG_FIFO_WATERMARK) {
			int_mask |= ADXL362_INTMAP1_FIFO_WATERMARK;
			int_value |= ADXL362_INTMAP1_FIFO_WATERMARK;
			fifo_wmark_irq = 1;
		}

		if (cfg->triggers[i].trigger == SENSOR_TRIG_FIFO_FULL) {
			int_mask |= ADXL362_INTMAP1_FIFO_OVERRUN;
			int_value |= ADXL362_INTMAP1_FIFO_OVERRUN;
			fifo_full_irq = 1;
		}
	}

	if (data->fifo_wmark_irq && (fifo_wmark_irq == 0)) {
		int_mask |= ADXL362_INTMAP1_FIFO_WATERMARK;
	}

	if (data->fifo_full_irq && (fifo_full_irq == 0)) {
		int_mask |= ADXL362_INTMAP1_FIFO_OVERRUN;
	}

	/* Do not flush the FIFO if interrupts are already enabled. */
	if ((fifo_wmark_irq != data->fifo_wmark_irq) || (fifo_full_irq != data->fifo_full_irq)) {
		data->fifo_wmark_irq = fifo_wmark_irq;
		data->fifo_full_irq = fifo_full_irq;

		rc = adxl362_reg_write_mask(dev, ADXL362_REG_INTMAP1, int_mask, int_value);
		if (rc < 0) {
			return;
		}

		/* Flush the FIFO by disabling it. Save current mode for after the reset. */
		uint8_t fifo_mode = data->fifo_mode;
		uint8_t en_temp_read = data->en_temp_read;
		uint16_t water_mark_lvl = data->water_mark_lvl;

		rc = adxl362_fifo_setup(dev, ADXL362_FIFO_DISABLE, 0, 0);
		if (rc < 0) {
			return;
		}

		if (fifo_mode == ADXL362_FIFO_DISABLE) {
			fifo_mode = ADXL362_FIFO_STREAM;
		}

		if (en_temp_read == 0) {
			en_temp_read = 1;
		}

		if (water_mark_lvl == 0) {
			water_mark_lvl = 0x80;
		}

		rc = adxl362_fifo_setup(dev, fifo_mode, water_mark_lvl, en_temp_read);
		if (rc < 0) {
			return;
		}
	}

	rc = gpio_pin_interrupt_configure_dt(&cfg_362->interrupt,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (rc < 0) {
		return;
	}

	data->sqe = iodev_sqe;
}

static void adxl362_fifo_read_cb(struct rtio *rtio_ctx, const struct rtio_sqe *sqe, void *arg)
{
	const struct device *dev = (const struct device *)arg;
	const struct adxl362_config *cfg = (const struct adxl362_config *)dev->config;
	struct rtio_iodev_sqe *iodev_sqe = sqe->userdata;

	rtio_iodev_sqe_ok(iodev_sqe, 0);

	gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_EDGE_TO_ACTIVE);
}

static void adxl362_process_fifo_samples_cb(struct rtio *r, const struct rtio_sqe *sqr, void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct adxl362_data *data = (struct adxl362_data *)dev->data;
	const struct adxl362_config *cfg = (const struct adxl362_config *)dev->config;
	struct rtio_iodev_sqe *current_sqe = data->sqe;
	uint16_t fifo_samples = ((data->fifo_ent[0]) | ((data->fifo_ent[1] & 0x3) << 8));
	size_t sample_set_size = 6;

	if (data->en_temp_read) {
		sample_set_size += 2;
	}

	uint16_t fifo_bytes = fifo_samples * 2 /*sample size*/;

	data->sqe = NULL;

	/* Not inherently an underrun/overrun as we may have a buffer to fill next time */
	if (current_sqe == NULL) {
		LOG_ERR("No pending SQE");
		gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	const size_t min_read_size = sizeof(struct adxl362_fifo_data) + sample_set_size;
	const size_t ideal_read_size = sizeof(struct adxl362_fifo_data) + fifo_bytes;

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
	struct adxl362_fifo_data *hdr = (struct adxl362_fifo_data *) buf;

	hdr->is_fifo = 1;
	hdr->timestamp = data->timestamp;
	hdr->int_status = data->status;
	hdr->selected_range = data->selected_range;
	hdr->has_tmp = data->en_temp_read;

	uint32_t buf_avail = buf_len;

	buf_avail -= sizeof(*hdr);

	uint32_t read_len = MIN(fifo_bytes, buf_avail);
	uint32_t pkts = read_len / sample_set_size;

	read_len = pkts * sample_set_size;

	((struct adxl362_fifo_data *)buf)->fifo_byte_count = read_len;

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
	const uint8_t reg_addr = ADXL362_READ_FIFO;

	rtio_sqe_prep_tiny_write(write_fifo_addr, data->iodev, RTIO_PRIO_NORM, &reg_addr, 1, NULL);
	write_fifo_addr->flags = RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_fifo_data, data->iodev, RTIO_PRIO_NORM, read_buf, read_len,
			   current_sqe);
	read_fifo_data->flags = RTIO_SQE_CHAINED;
	rtio_sqe_prep_callback(complete_op, adxl362_fifo_read_cb, (void *)dev, current_sqe);

	rtio_submit(data->rtio_ctx, 0);
}

static void adxl362_process_status_cb(struct rtio *r, const struct rtio_sqe *sqr, void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct adxl362_data *data = (struct adxl362_data *) dev->data;
	const struct adxl362_config *cfg = (const struct adxl362_config *) dev->config;
	struct rtio_iodev_sqe *current_sqe = data->sqe;
	struct sensor_read_config *read_config;
	uint8_t status = data->status;

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
	bool fifo_wmark_irq = false;

	if ((fifo_wmark_cfg != NULL) && ADXL362_STATUS_CHECK_FIFO_WTR(status)) {
		fifo_wmark_irq = true;
	}

	if ((fifo_full_cfg != NULL) && ADXL362_STATUS_CHECK_FIFO_OVR(status)) {
		fifo_full_irq = true;
	}

	if (!fifo_full_irq && !fifo_wmark_irq) {
		gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

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
		if (rtio_sqe_rx_buf(current_sqe, sizeof(struct adxl362_fifo_data),
				    sizeof(struct adxl362_fifo_data), &buf, &buf_len) != 0) {
			rtio_iodev_sqe_err(current_sqe, -ENOMEM);
			gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_EDGE_TO_ACTIVE);
			return;
		}

		struct adxl362_fifo_data *rx_data = (struct adxl362_fifo_data *)buf;

		memset(buf, 0, buf_len);
		rx_data->is_fifo = 1;
		rx_data->timestamp = data->timestamp;
		rx_data->int_status = status;
		rx_data->fifo_byte_count = 0;
		rtio_iodev_sqe_ok(current_sqe, 0);

		if (data_opt == SENSOR_STREAM_DATA_DROP) {
			/* Flush the FIFO by disabling it. Save current mode for after the reset. */
			adxl362_fifo_flush_rtio(dev);
			return;
		}

		gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	struct rtio_sqe *write_fifo_addr = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *read_fifo_data = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *complete_op = rtio_sqe_acquire(data->rtio_ctx);
	const uint8_t reg[2] = {ADXL362_READ_REG, ADXL362_REG_FIFO_L};

	rtio_sqe_prep_tiny_write(write_fifo_addr, data->iodev, RTIO_PRIO_NORM, reg, 2, NULL);
	write_fifo_addr->flags = RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_fifo_data, data->iodev, RTIO_PRIO_NORM, data->fifo_ent, 2,
						current_sqe);
	read_fifo_data->flags = RTIO_SQE_CHAINED;
	rtio_sqe_prep_callback(complete_op, adxl362_process_fifo_samples_cb, (void *)dev,
							current_sqe);

	rtio_submit(data->rtio_ctx, 0);
}

void adxl362_stream_irq_handler(const struct device *dev)
{
	struct adxl362_data *data = (struct adxl362_data *) dev->data;

	if (data->sqe == NULL) {
		return;
	}

	data->timestamp = k_ticks_to_ns_floor64(k_uptime_ticks());

	struct rtio_sqe *write_status_addr = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *read_status_reg = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *check_status_reg = rtio_sqe_acquire(data->rtio_ctx);
	const uint8_t reg[2] = {ADXL362_READ_REG, ADXL362_REG_STATUS};

	rtio_sqe_prep_tiny_write(write_status_addr, data->iodev, RTIO_PRIO_NORM, reg, 2, NULL);
	write_status_addr->flags = RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_status_reg, data->iodev, RTIO_PRIO_NORM, &data->status, 1, NULL);
	read_status_reg->flags = RTIO_SQE_CHAINED;
	rtio_sqe_prep_callback(check_status_reg, adxl362_process_status_cb, (void *)dev, NULL);
	rtio_submit(data->rtio_ctx, 0);
}
