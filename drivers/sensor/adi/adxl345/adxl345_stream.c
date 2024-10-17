/*
 * Copyright (c) 2024 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>

#include "adxl345.h"

LOG_MODULE_DECLARE(ADXL345, CONFIG_SENSOR_LOG_LEVEL);

void adxl345_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg =
			(const struct sensor_read_config *) iodev_sqe->sqe.iodev->data;
	struct adxl345_dev_data *data = (struct adxl345_dev_data *)dev->data;
	const struct adxl345_dev_config *cfg_345 = dev->config;
	uint8_t int_value = (uint8_t)~ADXL345_INT_MAP_WATERMARK_MSK;
	uint8_t fifo_watermark_irq = 0;
	int rc = gpio_pin_interrupt_configure_dt(&cfg_345->interrupt,
					      GPIO_INT_DISABLE);

	if (rc < 0) {
		return;
	}

	for (size_t i = 0; i < cfg->count; i++) {
		if (cfg->triggers[i].trigger == SENSOR_TRIG_FIFO_WATERMARK) {
			int_value = ADXL345_INT_MAP_WATERMARK_MSK;
			fifo_watermark_irq = 1;
		}
	}
		uint8_t status;
	if (fifo_watermark_irq != data->fifo_watermark_irq) {
		data->fifo_watermark_irq = fifo_watermark_irq;
		rc = adxl345_reg_write_mask(dev, ADXL345_INT_MAP, ADXL345_INT_MAP_WATERMARK_MSK,
						int_value);
		if (rc < 0) {
			return;
		}

		/* Flush the FIFO by disabling it. Save current mode for after the reset. */
		enum adxl345_fifo_mode current_fifo_mode = data->fifo_config.fifo_mode;

		if (current_fifo_mode == ADXL345_FIFO_BYPASSED) {
			current_fifo_mode = ADXL345_FIFO_STREAMED;
		}
		adxl345_configure_fifo(dev, ADXL345_FIFO_BYPASSED, data->fifo_config.fifo_trigger,
				data->fifo_config.fifo_samples);
		adxl345_configure_fifo(dev, current_fifo_mode, data->fifo_config.fifo_trigger,
				data->fifo_config.fifo_samples);
		rc = adxl345_reg_read_byte(dev, ADXL345_FIFO_STATUS_REG, &status);
	}

	rc = gpio_pin_interrupt_configure_dt(&cfg_345->interrupt,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (rc < 0) {
		return;
	}
	data->sqe = iodev_sqe;
}

static void adxl345_irq_en_cb(struct rtio *r, const struct rtio_sqe *sqr, void *arg)
{
	const struct device *dev = (const struct device *)arg;
	const struct adxl345_dev_config *cfg = dev->config;

	gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_EDGE_TO_ACTIVE);
}

static void adxl345_fifo_flush_rtio(const struct device *dev)
{
	struct adxl345_dev_data *data = dev->data;
	uint8_t fifo_config;

	fifo_config = (ADXL345_FIFO_CTL_TRIGGER_MODE(data->fifo_config.fifo_trigger) |
		       ADXL345_FIFO_CTL_MODE_MODE(ADXL345_FIFO_BYPASSED) |
		       ADXL345_FIFO_CTL_SAMPLES_MODE(data->fifo_config.fifo_samples));

	struct rtio_sqe *write_fifo_addr = rtio_sqe_acquire(data->rtio_ctx);
	const uint8_t reg_addr_w2[2] = {ADXL345_FIFO_CTL_REG, fifo_config};

	rtio_sqe_prep_tiny_write(write_fifo_addr, data->iodev, RTIO_PRIO_NORM, reg_addr_w2,
					2, NULL);

	fifo_config = (ADXL345_FIFO_CTL_TRIGGER_MODE(data->fifo_config.fifo_trigger) |
		       ADXL345_FIFO_CTL_MODE_MODE(data->fifo_config.fifo_mode) |
		       ADXL345_FIFO_CTL_SAMPLES_MODE(data->fifo_config.fifo_samples));

	write_fifo_addr = rtio_sqe_acquire(data->rtio_ctx);
	const uint8_t reg_addr_w3[2] = {ADXL345_FIFO_CTL_REG, fifo_config};

	rtio_sqe_prep_tiny_write(write_fifo_addr, data->iodev, RTIO_PRIO_NORM, reg_addr_w3,
					2, NULL);
	write_fifo_addr->flags |= RTIO_SQE_CHAINED;

	struct rtio_sqe *complete_op = rtio_sqe_acquire(data->rtio_ctx);

	rtio_sqe_prep_callback(complete_op, adxl345_irq_en_cb, (void *)dev, NULL);
	rtio_submit(data->rtio_ctx, 0);
}

static void adxl345_fifo_read_cb(struct rtio *rtio_ctx, const struct rtio_sqe *sqe, void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct adxl345_dev_data *data = (struct adxl345_dev_data *) dev->data;
	const struct adxl345_dev_config *cfg = (const struct adxl345_dev_config *) dev->config;
	struct rtio_iodev_sqe *iodev_sqe = sqe->userdata;

	if (data->fifo_samples == 0) {
		data->fifo_total_bytes = 0;
		rtio_iodev_sqe_ok(iodev_sqe, 0);
		gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_EDGE_TO_ACTIVE);
	}

}

static void adxl345_process_fifo_samples_cb(struct rtio *r, const struct rtio_sqe *sqr, void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct adxl345_dev_data *data = (struct adxl345_dev_data *) dev->data;
	const struct adxl345_dev_config *cfg = (const struct adxl345_dev_config *) dev->config;
	struct rtio_iodev_sqe *current_sqe = data->sqe;
	uint16_t fifo_samples = (data->fifo_ent[0]) & SAMPLE_MASK;
	size_t sample_set_size = SAMPLE_SIZE;
	uint16_t fifo_bytes = fifo_samples * SAMPLE_SIZE;

	data->sqe = NULL;

	/* Not inherently an underrun/overrun as we may have a buffer to fill next time */
	if (current_sqe == NULL) {
		LOG_ERR("No pending SQE");
		gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	const size_t min_read_size = sizeof(struct adxl345_fifo_data) + sample_set_size;
	const size_t ideal_read_size = sizeof(struct adxl345_fifo_data) + fifo_bytes;

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
	struct adxl345_fifo_data *hdr = (struct adxl345_fifo_data *) buf;

	hdr->is_fifo = 1;
	hdr->timestamp = data->timestamp;
	hdr->int_status = data->status1;
	hdr->accel_odr = cfg->odr;
	hdr->sample_set_size = sample_set_size;

	uint32_t buf_avail = buf_len;

	buf_avail -= sizeof(*hdr);

	uint32_t read_len = MIN(fifo_bytes, buf_avail);

	if (buf_avail < fifo_bytes) {
		uint32_t pkts = read_len / sample_set_size;

		read_len = pkts * sample_set_size;
	}

	((struct adxl345_fifo_data *)buf)->fifo_byte_count = read_len;

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


	data->fifo_samples = fifo_samples;
	for (size_t i = 0; i < fifo_samples; i++) {
		struct rtio_sqe *write_fifo_addr = rtio_sqe_acquire(data->rtio_ctx);
		struct rtio_sqe *read_fifo_data = rtio_sqe_acquire(data->rtio_ctx);

		data->fifo_samples--;
		const uint8_t reg_addr = ADXL345_REG_READ(ADXL345_X_AXIS_DATA_0_REG)
				| ADXL345_MULTIBYTE_FLAG;

		rtio_sqe_prep_tiny_write(write_fifo_addr, data->iodev, RTIO_PRIO_NORM, &reg_addr,
								1, NULL);
		write_fifo_addr->flags = RTIO_SQE_TRANSACTION;
		rtio_sqe_prep_read(read_fifo_data, data->iodev, RTIO_PRIO_NORM,
							read_buf + data->fifo_total_bytes,
							SAMPLE_SIZE, current_sqe);
		data->fifo_total_bytes += SAMPLE_SIZE;
		if (i == fifo_samples-1) {
			struct rtio_sqe *complete_op = rtio_sqe_acquire(data->rtio_ctx);

			read_fifo_data->flags = RTIO_SQE_CHAINED;
			rtio_sqe_prep_callback(complete_op, adxl345_fifo_read_cb, (void *)dev,
				current_sqe);
		}
		rtio_submit(data->rtio_ctx, 0);
		ARG_UNUSED(rtio_cqe_consume(data->rtio_ctx));
	}
}

static void adxl345_process_status1_cb(struct rtio *r, const struct rtio_sqe *sqr, void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct adxl345_dev_data *data = (struct adxl345_dev_data *) dev->data;
	const struct adxl345_dev_config *cfg = (const struct adxl345_dev_config *) dev->config;
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

	for (int i = 0; i < read_config->count; ++i) {
		if (read_config->triggers[i].trigger == SENSOR_TRIG_FIFO_WATERMARK) {
			fifo_wmark_cfg = &read_config->triggers[i];
			continue;
		}
	}

	bool fifo_full_irq = false;

	if ((fifo_wmark_cfg != NULL)
			&& FIELD_GET(ADXL345_INT_MAP_WATERMARK_MSK, status1)) {
		fifo_full_irq = true;
	}

	if (!fifo_full_irq) {
		gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	/* Flush completions */
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

	if (fifo_wmark_cfg != NULL) {
		data_opt = fifo_wmark_cfg->opt;
	}

	if (data_opt == SENSOR_STREAM_DATA_NOP || data_opt == SENSOR_STREAM_DATA_DROP) {
		uint8_t *buf;
		uint32_t buf_len;

		/* Clear streaming_sqe since we're done with the call */
		data->sqe = NULL;
		if (rtio_sqe_rx_buf(current_sqe, sizeof(struct adxl345_fifo_data),
				    sizeof(struct adxl345_fifo_data), &buf, &buf_len) != 0) {
			rtio_iodev_sqe_err(current_sqe, -ENOMEM);
			gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_EDGE_TO_ACTIVE);
			return;
		}

		struct adxl345_fifo_data *rx_data = (struct adxl345_fifo_data *)buf;

		memset(buf, 0, buf_len);
		rx_data->is_fifo = 1;
		rx_data->timestamp = data->timestamp;
		rx_data->int_status = status1;
		rx_data->fifo_byte_count = 0;
		rtio_iodev_sqe_ok(current_sqe, 0);

		if (data_opt == SENSOR_STREAM_DATA_DROP) {
			/* Flush the FIFO by disabling it. Save current mode for after the reset. */
			adxl345_fifo_flush_rtio(dev);
		}

		gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	struct rtio_sqe *write_fifo_addr = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *read_fifo_data = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *complete_op = rtio_sqe_acquire(data->rtio_ctx);
	const uint8_t reg_addr = ADXL345_REG_READ(ADXL345_FIFO_STATUS_REG);

	rtio_sqe_prep_tiny_write(write_fifo_addr, data->iodev, RTIO_PRIO_NORM, &reg_addr, 1, NULL);
	write_fifo_addr->flags = RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_fifo_data, data->iodev, RTIO_PRIO_NORM, data->fifo_ent, 1,
						current_sqe);
	read_fifo_data->flags = RTIO_SQE_CHAINED;
	rtio_sqe_prep_callback(complete_op, adxl345_process_fifo_samples_cb, (void *)dev,
							current_sqe);

	rtio_submit(data->rtio_ctx, 0);
}

void adxl345_stream_irq_handler(const struct device *dev)
{
	struct adxl345_dev_data *data = (struct adxl345_dev_data *) dev->data;

	if (data->sqe == NULL) {
		return;
	}
	data->timestamp = k_ticks_to_ns_floor64(k_uptime_ticks());
	struct rtio_sqe *write_status_addr = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *read_status_reg = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *check_status_reg = rtio_sqe_acquire(data->rtio_ctx);
	uint8_t reg = ADXL345_REG_READ(ADXL345_INT_SOURCE);

	rtio_sqe_prep_tiny_write(write_status_addr, data->iodev, RTIO_PRIO_NORM, &reg, 1, NULL);
	write_status_addr->flags = RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_status_reg, data->iodev, RTIO_PRIO_NORM, &data->status1, 1, NULL);
	read_status_reg->flags = RTIO_SQE_CHAINED;

	rtio_sqe_prep_callback(check_status_reg, adxl345_process_status1_cb, (void *)dev, NULL);
	rtio_submit(data->rtio_ctx, 0);
}
