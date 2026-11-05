/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_clock.h>
#include "adxl38x.h"

LOG_MODULE_DECLARE(ADXL38X);

/**
 * @brief Select the INT_MAP0 register for the routed interrupt pin
 *
 * @param data Driver data pointer
 * @return uint8_t INT0_MAP0 or INT1_MAP0 register address
 */
static inline uint8_t adxl38x_int_map0_reg(const struct adxl38x_data *data)
{
	return data->use_int1 ? ADXL38X_INT1_MAP0 : ADXL38X_INT0_MAP0;
}

/**
 * @brief Flush the FIFO by toggling standby then restoring the op mode
 *
 * @param dev Device pointer
 */
static void adxl38x_flush_rtio(const struct device *dev)
{
	struct adxl38x_data *data = dev->data;
	struct rtio_sqe *sqe;
	uint8_t addr = ADXL38X_SPI_WRITE(ADXL38X_OP_MODE);
	uint8_t reg_standby;
	uint8_t reg_restore;

	/* Preserve range bits, drop the mode field to standby then back. */
	reg_standby = FIELD_PREP(ADXL38X_OP_MODE_RANGE_MSK, data->range) | ADXL38X_STANDBY_MODE;
	reg_restore = FIELD_PREP(ADXL38X_OP_MODE_RANGE_MSK, data->range) |
		      (data->op_mode & ADXL38X_OP_MODE_MSK);

	sqe = rtio_sqe_acquire(data->rtio_ctx);
	if (sqe == NULL) {
		LOG_ERR("RTIO SQE queue exhausted");
		rtio_sqe_drop_all(data->rtio_ctx);
		return;
	}

	const uint8_t w_standby[2] = {addr, reg_standby};

	rtio_sqe_prep_tiny_write(sqe, data->iodev, RTIO_PRIO_NORM, w_standby, 2, NULL);
	sqe->flags |= RTIO_SQE_CHAINED;

	sqe = rtio_sqe_acquire(data->rtio_ctx);
	if (sqe == NULL) {
		LOG_ERR("RTIO SQE queue exhausted");
		rtio_sqe_drop_all(data->rtio_ctx);
		return;
	}

	const uint8_t w_restore[2] = {addr, reg_restore};

	rtio_sqe_prep_tiny_write(sqe, data->iodev, RTIO_PRIO_NORM, w_restore, 2, NULL);

	rtio_submit(data->rtio_ctx, 0);
}

/**
 * @brief Submit stream read request
 *
 * @param dev Device pointer
 * @param iodev_sqe IODEV SQE pointer
 */
void adxl38x_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg =
		(const struct sensor_read_config *)iodev_sqe->sqe.iodev->data;
	const struct adxl38x_dev_config *dev_cfg = dev->config;
	struct adxl38x_data *data = dev->data;
	uint8_t fifo_watermark_irq = 0;
	int ret;

	ret = gpio_pin_interrupt_configure_dt(&dev_cfg->interrupt_gpio, GPIO_INT_DISABLE);
	if (ret < 0) {
		LOG_ERR("Failed to disable interrupt: %d", ret);
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}

	for (size_t i = 0; i < cfg->count; i++) {
		if (cfg->triggers[i].trigger == SENSOR_TRIG_FIFO_WATERMARK) {
			fifo_watermark_irq = 1;
			break;
		}
	}

	if (fifo_watermark_irq != data->fifo_watermark_irq) {
		data->fifo_watermark_irq = fifo_watermark_irq;
		ret = adxl38x_reg_update(dev, adxl38x_int_map0_reg(data),
					 ADXL38X_INT_MAP0_FIFO_WATERMARK_MSK, fifo_watermark_irq);
		if (ret < 0) {
			LOG_ERR("Failed to update interrupt map: %d", ret);
			rtio_iodev_sqe_err(iodev_sqe, ret);
			return;
		}
		adxl38x_flush_rtio(dev);
	}

	ret = gpio_pin_interrupt_configure_dt(&dev_cfg->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to enable interrupt: %d", ret);
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}

	data->sqe = iodev_sqe;
}

/**
 * @brief FIFO burst-read completion callback
 *
 * @param r RTIO pointer
 * @param sqe RTIO SQE pointer
 * @param result Unused
 * @param arg Device pointer
 */
static void adxl38x_complete_op_cb(struct rtio *r, const struct rtio_sqe *sqe, int result,
				   void *arg)
{
	ARG_UNUSED(result);
	const struct device *dev = (const struct device *)arg;
	const struct adxl38x_dev_config *cfg = dev->config;
	struct rtio_iodev_sqe *current_sqe = (struct rtio_iodev_sqe *)sqe->userdata;

	rtio_iodev_sqe_ok(current_sqe, 0);
	gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}

/**
 * @brief Process the FIFO entry count and issue the burst read
 *
 * @param r RTIO pointer
 * @param sqe RTIO SQE pointer
 * @param result Unused
 * @param arg Device pointer
 */
static void adxl38x_process_fifo_samples_cb(struct rtio *r, const struct rtio_sqe *sqe, int result,
					    void *arg)
{
	ARG_UNUSED(result);
	const struct device *dev = (const struct device *)arg;
	const struct adxl38x_dev_config *cfg = dev->config;
	struct adxl38x_data *data = dev->data;
	struct rtio_iodev_sqe *current_sqe = data->sqe;
	uint16_t entries =
		data->fifo_ent[0] | ((data->fifo_ent[1] & ADXL38X_FIFO_STATUS1_ENTRIES_8_MSK) << 8);
	uint16_t frames = entries / ADXL38X_FIFO_CHANNELS;
	size_t sample_set_size = ADXL38X_FIFO_CHANNELS * ADXL38X_SAMPLE_SIZE;
	uint16_t fifo_bytes = frames * sample_set_size;

	data->sqe = NULL;

	if (current_sqe == NULL) {
		LOG_ERR("No pending SQE");
		gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	const size_t min_read_size = sizeof(struct adxl38x_fifo_data) + sample_set_size;
	const size_t ideal_read_size = sizeof(struct adxl38x_fifo_data) + fifo_bytes;
	uint8_t *buf;
	uint32_t buf_len;

	if (rtio_sqe_rx_buf(current_sqe, min_read_size, ideal_read_size, &buf, &buf_len) != 0) {
		LOG_ERR("Failed to get buffer");
		rtio_iodev_sqe_err(current_sqe, -ENOMEM);
		gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	struct adxl38x_fifo_data *hdr = (struct adxl38x_fifo_data *)buf;
	uint32_t buf_avail = buf_len - sizeof(*hdr);
	uint8_t *read_buf = buf + sizeof(*hdr);
	uint32_t read_len = MIN(fifo_bytes, buf_avail);

	/* Keep whole sample sets. */
	read_len -= read_len % sample_set_size;

	hdr->is_fifo = 1;
	hdr->timestamp = data->timestamp;
	hdr->status0 = data->status0;
	hdr->range = data->range;
	hdr->chip_type = cfg->chip_type;
	hdr->accel_odr = data->accel_odr;
	hdr->fifo_samples = read_len / sample_set_size;
	hdr->fifo_byte_count = read_len;
	hdr->sample_set_size = sample_set_size;

	/* Flush any pending completions before issuing the FIFO burst read. */
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

	if (res != 0) {
		rtio_iodev_sqe_err(current_sqe, res);
		gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	if (read_len == 0) {
		rtio_iodev_sqe_ok(current_sqe, 0);
		gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	struct rtio_sqe *write_fifo_addr = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *read_fifo_data = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *complete_op = rtio_sqe_acquire(data->rtio_ctx);
	uint8_t reg = ADXL38X_SPI_READ(ADXL38X_FIFO_DATA);

	if (write_fifo_addr == NULL || read_fifo_data == NULL || complete_op == NULL) {
		LOG_ERR("RTIO SQE queue exhausted");
		rtio_sqe_drop_all(data->rtio_ctx);
		rtio_iodev_sqe_err(current_sqe, -ENOMEM);
		gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	rtio_sqe_prep_tiny_write(write_fifo_addr, data->iodev, RTIO_PRIO_NORM, &reg, 1, NULL);
	write_fifo_addr->flags = RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_fifo_data, data->iodev, RTIO_PRIO_NORM, read_buf, read_len,
			   current_sqe);
	read_fifo_data->flags = RTIO_SQE_CHAINED;
	rtio_sqe_prep_callback(complete_op, adxl38x_complete_op_cb, (void *)dev,
			       (void *)current_sqe);
	rtio_submit(data->rtio_ctx, 0);
}

/**
 * @brief Handle the STATUS0 read and start FIFO processing
 *
 * @param r RTIO pointer
 * @param sqe RTIO SQE pointer
 * @param result Unused
 * @param arg Device pointer
 */
static void adxl38x_process_status0_cb(struct rtio *r, const struct rtio_sqe *sqe, int result,
				       void *arg)
{
	ARG_UNUSED(result);
	const struct device *dev = (const struct device *)arg;
	struct adxl38x_data *data = dev->data;
	const struct adxl38x_dev_config *cfg = dev->config;
	struct rtio_iodev_sqe *current_sqe = data->sqe;
	struct sensor_read_config *read_config;
	uint8_t status0 = data->status0;

	if (data->sqe == NULL) {
		return;
	}

	read_config = (struct sensor_read_config *)data->sqe->sqe.iodev->data;
	if (read_config == NULL || !read_config->is_streaming) {
		return;
	}

	gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_DISABLE);

	bool have_wmark = false;
	enum sensor_stream_data_opt data_opt = SENSOR_STREAM_DATA_NOP;

	for (int i = 0; i < read_config->count; ++i) {
		if (read_config->triggers[i].trigger == SENSOR_TRIG_FIFO_WATERMARK) {
			have_wmark = true;
			data_opt = read_config->triggers[i].opt;
			break;
		}
	}

	bool fifo_irq =
		have_wmark &&
		(status0 & (ADXL38X_STATUS0_FIFO_WATERMARK_MSK | ADXL38X_STATUS0_FIFO_FULL_MSK));

	if (!fifo_irq) {
		gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	/* Flush completions from the status read chain. */
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

	if (res != 0) {
		rtio_iodev_sqe_err(current_sqe, res);
		gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	if (data_opt == SENSOR_STREAM_DATA_NOP || data_opt == SENSOR_STREAM_DATA_DROP) {
		uint8_t *buf;
		uint32_t buf_len;

		data->sqe = NULL;
		if (rtio_sqe_rx_buf(current_sqe, sizeof(struct adxl38x_fifo_data),
				    sizeof(struct adxl38x_fifo_data), &buf, &buf_len) != 0) {
			rtio_iodev_sqe_err(current_sqe, -ENOMEM);
			gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio,
							GPIO_INT_EDGE_TO_ACTIVE);
			return;
		}

		struct adxl38x_fifo_data *rx = (struct adxl38x_fifo_data *)buf;

		memset(buf, 0, buf_len);
		rx->is_fifo = 1;
		rx->timestamp = data->timestamp;
		rx->status0 = status0;
		rx->fifo_samples = 0;
		rtio_iodev_sqe_ok(current_sqe, 0);

		if (data_opt == SENSOR_STREAM_DATA_DROP) {
			adxl38x_flush_rtio(dev);
		}

		gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	/* Read the 9-bit FIFO entry count (FIFO_STATUS0 + FIFO_STATUS1). */
	struct rtio_sqe *write_addr = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *read_entries = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *complete = rtio_sqe_acquire(data->rtio_ctx);
	uint8_t reg = ADXL38X_SPI_READ(ADXL38X_FIFO_STATUS0);

	if (write_addr == NULL || read_entries == NULL || complete == NULL) {
		LOG_ERR("RTIO SQE queue exhausted");
		rtio_sqe_drop_all(data->rtio_ctx);
		rtio_iodev_sqe_err(current_sqe, -ENOMEM);
		gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	rtio_sqe_prep_tiny_write(write_addr, data->iodev, RTIO_PRIO_NORM, &reg, 1, NULL);
	write_addr->flags = RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_entries, data->iodev, RTIO_PRIO_NORM, data->fifo_ent, 2,
			   current_sqe);
	read_entries->flags = RTIO_SQE_CHAINED;
	rtio_sqe_prep_callback(complete, adxl38x_process_fifo_samples_cb, (void *)dev, NULL);
	rtio_submit(data->rtio_ctx, 0);
}

/**
 * @brief ADXL38X stream IRQ handler
 *
 * @param dev Device pointer
 */
void adxl38x_stream_irq_handler(const struct device *dev)
{
	struct adxl38x_data *data = dev->data;
	uint64_t cycles;
	int ret;

	if (data->sqe == NULL) {
		return;
	}

	ret = sensor_clock_get_cycles(&cycles);
	if (ret != 0) {
		LOG_ERR("Failed to get sensor clock cycles");
		rtio_iodev_sqe_err(data->sqe, ret);
		return;
	}

	data->timestamp = sensor_clock_cycles_to_ns(cycles);

	struct rtio_sqe *write_status_addr = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *read_status_reg = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *check_status_reg = rtio_sqe_acquire(data->rtio_ctx);
	uint8_t reg = ADXL38X_SPI_READ(ADXL38X_STATUS0);

	if (write_status_addr == NULL || read_status_reg == NULL || check_status_reg == NULL) {
		LOG_ERR("RTIO SQE queue exhausted");
		rtio_sqe_drop_all(data->rtio_ctx);
		rtio_iodev_sqe_err(data->sqe, -ENOMEM);
		return;
	}

	rtio_sqe_prep_tiny_write(write_status_addr, data->iodev, RTIO_PRIO_NORM, &reg, 1, NULL);
	write_status_addr->flags |= RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_status_reg, data->iodev, RTIO_PRIO_NORM, &data->status0, 1, NULL);
	read_status_reg->flags |= RTIO_SQE_CHAINED;
	rtio_sqe_prep_callback(check_status_reg, adxl38x_process_status0_cb, (void *)dev, NULL);
	rtio_submit(data->rtio_ctx, 0);
}
