/*
 * Copyright (c) 2024 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_clock.h>
#include "adxl367.h"

LOG_MODULE_DECLARE(ADXL362, CONFIG_SENSOR_LOG_LEVEL);

static void adxl367_sqe_done(const struct adxl367_dev_config *cfg,
	struct rtio_iodev_sqe *iodev_sqe, int res)
{
	if (res < 0) {
		rtio_iodev_sqe_err(iodev_sqe, res);
	} else {
		rtio_iodev_sqe_ok(iodev_sqe, res);
	}

	gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_EDGE_TO_ACTIVE);
}

static void adxl367_irq_en_cb(struct rtio *r, const struct rtio_sqe *sqr, void *arg)
{
	const struct device *dev = (const struct device *)arg;
	const struct adxl367_dev_config *cfg = dev->config;

	gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_EDGE_TO_ACTIVE);
}

static void adxl367_fifo_flush_rtio(const struct device *dev)
{
	struct adxl367_data *data = dev->data;
	uint8_t pow_reg = data->pwr_reg;

	pow_reg &= ~ADXL367_POWER_CTL_MEASURE_MSK;
	pow_reg |= FIELD_PREP(ADXL367_POWER_CTL_MEASURE_MSK, ADXL367_STANDBY);

	struct rtio_sqe *sqe = rtio_sqe_acquire(data->rtio_ctx);
	const uint8_t reg_addr_w[3] = {ADXL367_SPI_WRITE_REG, ADXL367_POWER_CTL, pow_reg};

	rtio_sqe_prep_tiny_write(sqe, data->iodev, RTIO_PRIO_NORM, reg_addr_w, 3, NULL);

	sqe = rtio_sqe_acquire(data->rtio_ctx);
	const uint8_t reg_addr_w2[3] = {ADXL367_SPI_WRITE_REG, ADXL367_FIFO_CONTROL,
		FIELD_PREP(ADXL367_FIFO_CONTROL_FIFO_MODE_MSK, ADXL367_FIFO_DISABLED)};

	rtio_sqe_prep_tiny_write(sqe, data->iodev, RTIO_PRIO_NORM, reg_addr_w2, 3, NULL);

	sqe = rtio_sqe_acquire(data->rtio_ctx);
	const uint8_t reg_addr_w3[3] = {ADXL367_SPI_WRITE_REG, ADXL367_FIFO_CONTROL,
		FIELD_PREP(ADXL367_FIFO_CONTROL_FIFO_MODE_MSK, data->fifo_config.fifo_mode)};

	rtio_sqe_prep_tiny_write(sqe, data->iodev, RTIO_PRIO_NORM, reg_addr_w3, 3, NULL);

	pow_reg = data->pwr_reg;

	pow_reg &= ~ADXL367_POWER_CTL_MEASURE_MSK;
	pow_reg |= FIELD_PREP(ADXL367_POWER_CTL_MEASURE_MSK, ADXL367_MEASURE);

	sqe = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *complete_op = rtio_sqe_acquire(data->rtio_ctx);
	const uint8_t reg_addr_w4[3] = {ADXL367_SPI_WRITE_REG, ADXL367_POWER_CTL, pow_reg};

	rtio_sqe_prep_tiny_write(sqe, data->iodev, RTIO_PRIO_NORM, reg_addr_w4, 3, NULL);
	sqe->flags |= RTIO_SQE_CHAINED;
	rtio_sqe_prep_callback(complete_op, adxl367_irq_en_cb, (void *)dev, NULL);
	rtio_submit(data->rtio_ctx, 0);
}

void adxl367_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg =
			(const struct sensor_read_config *)iodev_sqe->sqe.iodev->data;
	struct adxl367_data *data = (struct adxl367_data *)dev->data;
	const struct adxl367_dev_config *cfg_367 = dev->config;
	uint8_t int_mask = 0;
	uint8_t int_value = 0;
	uint8_t fifo_wmark_irq = 0;
	uint8_t fifo_full_irq = 0;

	int rc = gpio_pin_interrupt_configure_dt(&cfg_367->interrupt,
					      GPIO_INT_DISABLE);
	if (rc < 0) {
		return;
	}

	for (size_t i = 0; i < cfg->count; i++) {
		if (cfg->triggers[i].trigger == SENSOR_TRIG_FIFO_WATERMARK) {
			int_mask |= ADXL367_FIFO_WATERMARK;
			int_value |= ADXL367_FIFO_WATERMARK;
			fifo_wmark_irq = 1;
		}

		if (cfg->triggers[i].trigger == SENSOR_TRIG_FIFO_FULL) {
			int_mask |= ADXL367_FIFO_OVERRUN;
			int_value |= ADXL367_FIFO_OVERRUN;
			fifo_full_irq = 1;
		}
	}

	if (data->fifo_wmark_irq && (fifo_wmark_irq == 0)) {
		int_mask |= ADXL367_FIFO_WATERMARK;
	}

	if (data->fifo_full_irq && (fifo_full_irq == 0)) {
		int_mask |= ADXL367_FIFO_OVERRUN;
	}

	/* Do not flush the FIFO if interrupts are already enabled. */
	if ((fifo_wmark_irq != data->fifo_wmark_irq) || (fifo_full_irq != data->fifo_full_irq)) {
		data->fifo_wmark_irq = fifo_wmark_irq;
		data->fifo_full_irq = fifo_full_irq;

		rc = data->hw_tf->write_reg_mask(dev, ADXL367_INTMAP1_LOWER, int_mask, int_value);
		if (rc < 0) {
			return;
		}

		/* Flush the FIFO by disabling it. Save current mode for after the reset. */
		enum adxl367_fifo_mode current_fifo_mode = data->fifo_config.fifo_mode;

		if (current_fifo_mode == ADXL367_FIFO_DISABLED) {
			LOG_ERR("ERROR: FIFO DISABLED");
			return;
		}

		adxl367_set_op_mode(dev, ADXL367_STANDBY);

		adxl367_fifo_setup(dev, ADXL367_FIFO_DISABLED, data->fifo_config.fifo_format,
			data->fifo_config.fifo_read_mode, data->fifo_config.fifo_samples);

		adxl367_fifo_setup(dev, current_fifo_mode, data->fifo_config.fifo_format,
			data->fifo_config.fifo_read_mode, data->fifo_config.fifo_samples);

		adxl367_set_op_mode(dev, cfg_367->op_mode);
	}

	rc = gpio_pin_interrupt_configure_dt(&cfg_367->interrupt,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (rc < 0) {
		return;
	}

	data->sqe = iodev_sqe;
}

static void adxl367_fifo_read_cb(struct rtio *rtio_ctx, const struct rtio_sqe *sqe, void *arg)
{
	const struct device *dev = (const struct device *)arg;
	const struct adxl367_dev_config *cfg = (const struct adxl367_dev_config *)dev->config;
	struct rtio_iodev_sqe *iodev_sqe = sqe->userdata;

	adxl367_sqe_done(cfg, iodev_sqe, 0);
}

size_t adxl367_get_numb_of_samp_in_pkt(const struct adxl367_data *data)
{
	size_t sample_numb;

	switch (data->fifo_config.fifo_format) {
	case ADXL367_FIFO_FORMAT_X:
	case ADXL367_FIFO_FORMAT_Y:
	case ADXL367_FIFO_FORMAT_Z:
		sample_numb = 1;
		break;

	case ADXL367_FIFO_FORMAT_XT:
	case ADXL367_FIFO_FORMAT_YT:
	case ADXL367_FIFO_FORMAT_ZT:
	case ADXL367_FIFO_FORMAT_XA:
	case ADXL367_FIFO_FORMAT_YA:
	case ADXL367_FIFO_FORMAT_ZA:
		sample_numb = 2;
		break;

	case ADXL367_FIFO_FORMAT_XYZT:
	case ADXL367_FIFO_FORMAT_XYZA:
		sample_numb = 4;
		break;

	default:
		sample_numb = 3;
		break;
	}

	return sample_numb;
}

static void adxl367_process_fifo_samples_cb(struct rtio *r, const struct rtio_sqe *sqr, void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct adxl367_data *data = (struct adxl367_data *)dev->data;
	const struct adxl367_dev_config *cfg = (const struct adxl367_dev_config *)dev->config;
	struct rtio_iodev_sqe *current_sqe = data->sqe;
	uint16_t fifo_samples = ((data->fifo_ent[0]) | ((data->fifo_ent[1] & 0x3) << 8));
	size_t sample_numb = adxl367_get_numb_of_samp_in_pkt(data);
	size_t packet_size = sample_numb;
	uint16_t fifo_packet_cnt = fifo_samples / sample_numb;
	uint16_t fifo_bytes = 0;

	switch (data->fifo_config.fifo_read_mode) {
	case ADXL367_8B:
		fifo_bytes = fifo_packet_cnt;
		break;
	case ADXL367_12B: {
		unsigned int fifo_bits = fifo_packet_cnt * sample_numb * 12;

		if (fifo_bits % 8 == 0) {
			fifo_bytes = fifo_bits / 8;
		} else {
			while (fifo_bits % 8) {
				if (fifo_bits >= sample_numb * 12) {
					fifo_bits -= sample_numb * 12;
				} else {
					fifo_bits = 0;
					break;
				}
			}

			if (fifo_bits) {
				fifo_bytes = fifo_bits / 8;
			} else {
				LOG_ERR("fifo_bytes error: %d", fifo_bytes);
				adxl367_sqe_done(cfg, current_sqe, -1);
				return;
			}
		}

		packet_size = packet_size * 12 / 8;
		if ((sample_numb * 12) % 8) {
			packet_size++;
		}
		break;
	}
	default:
		fifo_bytes = fifo_packet_cnt * 2;
		packet_size *= 2;
		break;
	}

	data->sqe = NULL;

	/* Not inherently an underrun/overrun as we may have a buffer to fill next time */
	if (current_sqe == NULL) {
		LOG_ERR("No pending SQE");
		gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	const size_t min_read_size = sizeof(struct adxl367_fifo_data) + packet_size;
	const size_t ideal_read_size = sizeof(struct adxl367_fifo_data) + fifo_bytes;

	uint8_t *buf;
	uint32_t buf_len;

	if (rtio_sqe_rx_buf(current_sqe, min_read_size, ideal_read_size, &buf, &buf_len) != 0) {
		LOG_ERR("Failed to get buffer");
		adxl367_sqe_done(cfg, current_sqe, -ENOMEM);
		return;
	}

	LOG_DBG("Requesting buffer [%u, %u] got %u", (unsigned int)min_read_size,
		(unsigned int)ideal_read_size, buf_len);

	/* Read FIFO and call back to rtio with rtio_sqe completion */
	struct adxl367_fifo_data *hdr = (struct adxl367_fifo_data *)buf;

	hdr->is_fifo = 1;
	hdr->timestamp = data->timestamp;
	hdr->int_status = data->status;
	hdr->accel_odr = data->odr;
	hdr->range = data->range;
	hdr->fifo_read_mode = data->fifo_config.fifo_read_mode;

	if (data->fifo_config.fifo_read_mode == ADXL367_12B) {
		hdr->packet_size = sample_numb;
	} else {
		hdr->packet_size = packet_size;
	}

	if ((data->fifo_config.fifo_format == ADXL367_FIFO_FORMAT_X) ||
		(data->fifo_config.fifo_format == ADXL367_FIFO_FORMAT_XT) ||
		(data->fifo_config.fifo_format == ADXL367_FIFO_FORMAT_XA) ||
		(data->fifo_config.fifo_format == ADXL367_FIFO_FORMAT_XYZ) ||
		(data->fifo_config.fifo_format == ADXL367_FIFO_FORMAT_XYZA) ||
		(data->fifo_config.fifo_format == ADXL367_FIFO_FORMAT_XYZT)) {
		hdr->has_x = 1;
	}

	if ((data->fifo_config.fifo_format == ADXL367_FIFO_FORMAT_Y) ||
		(data->fifo_config.fifo_format == ADXL367_FIFO_FORMAT_YT) ||
		(data->fifo_config.fifo_format == ADXL367_FIFO_FORMAT_YA) ||
		(data->fifo_config.fifo_format == ADXL367_FIFO_FORMAT_XYZ) ||
		(data->fifo_config.fifo_format == ADXL367_FIFO_FORMAT_XYZA) ||
		(data->fifo_config.fifo_format == ADXL367_FIFO_FORMAT_XYZT)) {
		hdr->has_y = 1;
	}

	if ((data->fifo_config.fifo_format == ADXL367_FIFO_FORMAT_Z) ||
		(data->fifo_config.fifo_format == ADXL367_FIFO_FORMAT_ZT) ||
		(data->fifo_config.fifo_format == ADXL367_FIFO_FORMAT_ZA) ||
		(data->fifo_config.fifo_format == ADXL367_FIFO_FORMAT_XYZ) ||
		(data->fifo_config.fifo_format == ADXL367_FIFO_FORMAT_XYZA) ||
		(data->fifo_config.fifo_format == ADXL367_FIFO_FORMAT_XYZT)) {
		hdr->has_z = 1;
	}

	if ((data->fifo_config.fifo_format == ADXL367_FIFO_FORMAT_XT) ||
		(data->fifo_config.fifo_format == ADXL367_FIFO_FORMAT_YT) ||
		(data->fifo_config.fifo_format == ADXL367_FIFO_FORMAT_ZT) ||
		(data->fifo_config.fifo_format == ADXL367_FIFO_FORMAT_XYZT)) {
		hdr->has_tmp = 1;
	}

	if ((data->fifo_config.fifo_format == ADXL367_FIFO_FORMAT_XA) ||
		(data->fifo_config.fifo_format == ADXL367_FIFO_FORMAT_YA) ||
		(data->fifo_config.fifo_format == ADXL367_FIFO_FORMAT_ZA) ||
		(data->fifo_config.fifo_format == ADXL367_FIFO_FORMAT_XYZA)) {
		hdr->has_adc = 1;
	}

	uint32_t buf_avail = buf_len;

	buf_avail -= sizeof(*hdr);

	uint32_t read_len = MIN(fifo_bytes, buf_avail);

	if (data->fifo_config.fifo_read_mode == ADXL367_12B) {
		unsigned int read_bits = read_len * 8;
		unsigned int packet_size_bits = sample_numb * 12;
		unsigned int read_packet_num = read_bits / packet_size_bits;
		unsigned int read_len_bits = read_packet_num * sample_numb * 12;

		if (read_len_bits % 8 == 0) {
			read_len = read_len_bits / 8;
		} else {
			while (read_len_bits % 8) {
				if (read_len_bits >= sample_numb * 12) {
					read_len_bits -= sample_numb * 12;
				} else {
					read_len_bits = 0;
					break;
				}
			}

			if (read_len_bits) {
				read_len = read_len_bits / 8;
			} else {
				LOG_ERR("read_len error");
				adxl367_sqe_done(cfg, current_sqe, -ENOMEM);
				return;
			}
		}
	} else {
		uint32_t pkts = read_len / packet_size;

		read_len = pkts * packet_size;
	}

	((struct adxl367_fifo_data *)buf)->fifo_byte_count = read_len;

	__ASSERT_NO_MSG(read_len % packet_size == 0);

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
		adxl367_sqe_done(cfg, current_sqe, res);
		return;
	}

	/* Setup new rtio chain to read the fifo data and report then check the
	 * result
	 */
	struct rtio_sqe *write_fifo_addr = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *read_fifo_data = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *complete_op = rtio_sqe_acquire(data->rtio_ctx);
	const uint8_t reg_addr = ADXL367_SPI_READ_FIFO;

	rtio_sqe_prep_tiny_write(write_fifo_addr, data->iodev, RTIO_PRIO_NORM, &reg_addr, 1, NULL);
	write_fifo_addr->flags = RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_fifo_data, data->iodev, RTIO_PRIO_NORM, read_buf, read_len,
			   current_sqe);
	read_fifo_data->flags = RTIO_SQE_CHAINED;
	rtio_sqe_prep_callback(complete_op, adxl367_fifo_read_cb, (void *)dev, current_sqe);

	rtio_submit(data->rtio_ctx, 0);
}

static void adxl367_process_status_cb(struct rtio *r, const struct rtio_sqe *sqr, void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct adxl367_data *data = (struct adxl367_data *) dev->data;
	const struct adxl367_dev_config *cfg = (const struct adxl367_dev_config *) dev->config;
	struct rtio_iodev_sqe *current_sqe = data->sqe;
	struct sensor_read_config *read_config;
	uint8_t status = data->status;

	__ASSERT(data->sqe != NULL, "%s data->sqe = NULL", __func__);

	read_config = (struct sensor_read_config *)data->sqe->sqe.iodev->data;

	__ASSERT(read_config != NULL, "%s read_config = NULL", __func__);

	__ASSERT(read_config->is_streaming != false,
		"%s read_config->is_streaming = false", __func__);

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

	if ((fifo_wmark_cfg != NULL) && FIELD_GET(ADXL367_STATUS_FIFO_WATERMARK, status)) {
		fifo_wmark_irq = true;
	}

	if ((fifo_full_cfg != NULL) && FIELD_GET(ADXL367_STATUS_FIFO_OVERRUN, status)) {
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
		adxl367_sqe_done(cfg, current_sqe, res);
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
		if (rtio_sqe_rx_buf(current_sqe, sizeof(struct adxl367_fifo_data),
				    sizeof(struct adxl367_fifo_data), &buf, &buf_len) != 0) {
			adxl367_sqe_done(cfg, current_sqe, -ENOMEM);
			return;
		}

		struct adxl367_fifo_data *rx_data = (struct adxl367_fifo_data *)buf;

		memset(buf, 0, buf_len);
		rx_data->is_fifo = 1;
		rx_data->timestamp = data->timestamp;
		rx_data->int_status = status;
		rx_data->fifo_byte_count = 0;

		if (data_opt == SENSOR_STREAM_DATA_DROP) {
			/* Flush the FIFO by disabling it. Save current mode for after the reset. */
			adxl367_fifo_flush_rtio(dev);
			return;
		}

		adxl367_sqe_done(cfg, current_sqe, 0);
		return;
	}

	struct rtio_sqe *write_fifo_addr = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *read_fifo_data = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *complete_op = rtio_sqe_acquire(data->rtio_ctx);
	const uint8_t reg[2] = {ADXL367_SPI_READ_REG, ADXL367_FIFO_ENTRIES_L};

	rtio_sqe_prep_tiny_write(write_fifo_addr, data->iodev, RTIO_PRIO_NORM, reg, 2, NULL);
	write_fifo_addr->flags = RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_fifo_data, data->iodev, RTIO_PRIO_NORM, data->fifo_ent, 2,
						current_sqe);
	read_fifo_data->flags = RTIO_SQE_CHAINED;
	rtio_sqe_prep_callback(complete_op, adxl367_process_fifo_samples_cb, (void *)dev,
							current_sqe);

	rtio_submit(data->rtio_ctx, 0);
}

void adxl367_stream_irq_handler(const struct device *dev)
{
	struct adxl367_data *data = (struct adxl367_data *) dev->data;
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
	const uint8_t reg[2] = {ADXL367_SPI_READ_REG, ADXL367_STATUS};

	rtio_sqe_prep_tiny_write(write_status_addr, data->iodev, RTIO_PRIO_NORM, reg, 2, NULL);
	write_status_addr->flags = RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_status_reg, data->iodev, RTIO_PRIO_NORM, &data->status, 1, NULL);
	read_status_reg->flags = RTIO_SQE_CHAINED;
	rtio_sqe_prep_callback(check_status_reg, adxl367_process_status_cb, (void *)dev, NULL);
	rtio_submit(data->rtio_ctx, 0);
}
