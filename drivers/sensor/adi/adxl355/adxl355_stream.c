/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_clock.h>
#include "adxl355.h"

LOG_MODULE_DECLARE(ADXL355);
/**
 * @brief Flush the FIFO buffer
 *
 * @param dev Device pointer
 */
void adxl355_flush_fifo(const struct device *dev)
{
	struct adxl355_data *data = dev->data;
	uint8_t pwr_reg = data->extra_attr.pwr_reg;

	pwr_reg &= ~ADXL355_POWER_CTL_STANDBY_MSK;
	pwr_reg |= FIELD_PREP(ADXL355_POWER_CTL_STANDBY_MSK, ADXL355_STANDBY);
	adxl355_reg_write(dev, ADXL355_POWER_CTL, &pwr_reg, 1);

	pwr_reg &= ~ADXL355_POWER_CTL_STANDBY_MSK;
	pwr_reg |= FIELD_PREP(ADXL355_POWER_CTL_STANDBY_MSK, ADXL355_MEASURE);
	adxl355_reg_write(dev, ADXL355_POWER_CTL, &pwr_reg, 1);
}

/**
 * @brief Submit stream read request
 *
 * @param dev Device pointer
 * @param iodev_sqe IODEV SQE pointer
 */
void adxl355_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg =
		(const struct sensor_read_config *)iodev_sqe->sqe.iodev->data;

	struct adxl355_data *data = (struct adxl355_data *)dev->data;
	const struct adxl355_dev_config *cfg_355 = dev->config;
	uint8_t fifo_watermark_irq = 0;
	uint8_t status;
	int ret = 0;

	ret = gpio_pin_interrupt_configure_dt(&cfg_355->interrupt_gpio, GPIO_INT_DISABLE);
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

	if (fifo_watermark_irq != data->fifo_watermark) {
		data->fifo_watermark = fifo_watermark_irq;
		ret = adxl355_reg_update(dev, ADXL355_INT_MAP,
					 data->route_to_int2 ? ADXL355_INT_MAP_FIFO_FULL_EN2_MSK
							     : ADXL355_INT_MAP_FIFO_FULL_EN1_MSK,
					 fifo_watermark_irq);
		if (ret < 0) {
			LOG_ERR("Failed to update interrupt map: %d", ret);
			rtio_iodev_sqe_err(iodev_sqe, ret);
			return;
		}
		adxl355_flush_fifo(dev);

		ret = adxl355_reg_read(dev, ADXL355_STATUS, &status, 1);
		if (ret < 0) {
			LOG_ERR("Failed to read status register: %d", ret);
			rtio_iodev_sqe_err(iodev_sqe, ret);
			return;
		}
	}
	ret = gpio_pin_interrupt_configure_dt(&cfg_355->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to enable interrupt: %d", ret);
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}
	data->sqe = iodev_sqe;
}

/**
 * @brief ADXL355 IRQ enable callback
 *
 * @param r RTIO pointer
 * @param sqe RTIO SQE pointer
 * @param result Result
 * @param arg Argument
 */
static void adxl355_irq_en_cb(struct rtio *r, const struct rtio_sqe *sqe, int result, void *arg)
{
	ARG_UNUSED(result);

	const struct device *dev = (const struct device *)arg;
	const struct adxl355_dev_config *cfg = (const struct adxl355_dev_config *)dev->config;

	gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}

/**
 * @brief Flush RTIO
 *
 * @param dev Device pointer
 */
static void adxl355_flush_rtio(const struct device *dev)
{
	struct adxl355_data *data = dev->data;
	uint8_t pow_reg = data->extra_attr.pwr_reg;

	pow_reg &= ~ADXL355_POWER_CTL_STANDBY_MSK;
	pow_reg |= FIELD_PREP(ADXL355_POWER_CTL_STANDBY_MSK, ADXL355_STANDBY);
	struct rtio_sqe *sqe = rtio_sqe_acquire(data->rtio_ctx);
#ifdef CONFIG_SPI_RTIO
	uint8_t reg_addr = ADXL355_SPI_WRITE(ADXL355_POWER_CTL);
#else
	uint8_t reg_addr = ADXL355_POWER_CTL;
#endif
	const uint8_t reg_addr_w1[2] = {reg_addr, pow_reg};

	rtio_sqe_prep_tiny_write(sqe, data->iodev, RTIO_PRIO_NORM, reg_addr_w1, 2, NULL);

	pow_reg &= ~ADXL355_POWER_CTL_STANDBY_MSK;
	pow_reg |= FIELD_PREP(ADXL355_POWER_CTL_STANDBY_MSK, ADXL355_MEASURE);
	sqe = rtio_sqe_acquire(data->rtio_ctx);
	const uint8_t reg_addr_w2[2] = {reg_addr, pow_reg};

	rtio_sqe_prep_tiny_write(sqe, data->iodev, RTIO_PRIO_NORM, reg_addr_w2, 2, NULL);
	sqe->flags |= RTIO_SQE_CHAINED;
	struct rtio_sqe *complete_op = rtio_sqe_acquire(data->rtio_ctx);

	rtio_sqe_prep_callback(complete_op, adxl355_irq_en_cb, (void *)dev, NULL);
	rtio_submit(data->rtio_ctx, 0);
}

/**
 * @brief ADXL355 complete operation callback
 *
 * @param r RTIO pointer
 * @param sqe RTIO SQE pointer
 * @param result Result
 * @param arg Argument
 */
static void adxl355_complete_op_cb(struct rtio *r, const struct rtio_sqe *sqe, int result,
				   void *arg)
{
	ARG_UNUSED(result);
	const struct device *dev = (const struct device *)arg;
	const struct adxl355_dev_config *cfg = (const struct adxl355_dev_config *)dev->config;
	struct rtio_iodev_sqe *current_sqe = (struct rtio_iodev_sqe *)sqe->userdata;

	rtio_iodev_sqe_ok(current_sqe, 0);
	gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}

/**
 * @brief ADXL355 process FIFO samples callback
 *
 * @param r RTIO pointer
 * @param sqe RTIO SQE pointer
 * @param result Result
 * @param arg Argument
 */
static void adxl355_process_fifo_samples_cb(struct rtio *r, const struct rtio_sqe *sqe, int result,
					    void *arg)
{
	ARG_UNUSED(result);
	const struct device *dev = (const struct device *)arg;
	const struct adxl355_dev_config *cfg = (const struct adxl355_dev_config *)dev->config;
	struct adxl355_data *data = (struct adxl355_data *)dev->data;
	struct rtio_iodev_sqe *current_sqe = data->sqe;
	uint16_t fifo_samples = ((data->fifo_samples) & ADXL355_FIFO_ENTRIES_MSK);
	size_t sample_set_size = 3;
	uint16_t fifo_bytes = fifo_samples * sample_set_size;

	data->sqe = NULL;

	if (current_sqe == NULL) {
		LOG_ERR("No pending SQE");
		gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	const size_t min_read_size = sizeof(struct adxl355_fifo_data) + sample_set_size;
	const size_t ideal_read_size = sizeof(struct adxl355_fifo_data) + fifo_bytes;

	uint8_t *buf;
	uint32_t buf_len;

	if (rtio_sqe_rx_buf(current_sqe, min_read_size, ideal_read_size, &buf, &buf_len) != 0) {
		LOG_ERR("Failed to get buffer");
		rtio_iodev_sqe_err(current_sqe, -ENOMEM);
		gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	LOG_DBG("Requesting buffer [%u, %u] got %u", (unsigned int)min_read_size,
		(unsigned int)ideal_read_size, buf_len);

	struct adxl355_fifo_data *hdr = (struct adxl355_fifo_data *)buf;

	hdr->is_fifo = 1;
	hdr->timestamp = data->timestamp;
	hdr->status1 = data->status1;
	hdr->fifo_byte_count = fifo_bytes;
	hdr->fifo_samples = fifo_samples;
	hdr->range = data->range;
	hdr->accel_odr = data->odr;
	hdr->sample_set_size = sample_set_size;
	uint32_t buf_avail = buf_len - sizeof(*hdr);

	memcpy(buf, hdr, sizeof(*hdr));

	uint8_t *read_buf = buf + sizeof(*hdr);
	uint32_t read_len = MIN(fifo_bytes, buf_avail);

	((struct adxl355_fifo_data *)buf)->fifo_byte_count = read_len;
	__ASSERT_NO_MSG(read_len % sample_set_size == 0);

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

	struct rtio_sqe *write_fifo_addr = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *read_fifo_data = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *complete_op = rtio_sqe_acquire(data->rtio_ctx);

#ifdef CONFIG_SPI_RTIO
	uint8_t reg = ADXL355_SPI_READ(ADXL355_FIFO_DATA);
#elif defined(CONFIG_I2C_RTIO)
	uint8_t reg = ADXL355_FIFO_DATA;
#endif
	rtio_sqe_prep_tiny_write(write_fifo_addr, data->iodev, RTIO_PRIO_NORM, &reg, 1, NULL);
	write_fifo_addr->flags = RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_fifo_data, data->iodev, RTIO_PRIO_NORM, read_buf, read_len,
			   current_sqe);
	read_fifo_data->flags = RTIO_SQE_CHAINED;
#ifdef CONFIG_I2C_RTIO
	read_fifo_data->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
#endif
	rtio_sqe_prep_callback(complete_op, adxl355_complete_op_cb, (void *)dev,
			       (void *)current_sqe);
	rtio_submit(data->rtio_ctx, 0);
}

/**
 * @brief ADXL355 process STATUS1 callback
 *
 * @param r RTIO pointer
 * @param sqe RTIO SQE pointer
 * @param result Result
 * @param arg Argument
 */
static void adxl355_process_status1_cb(struct rtio *r, const struct rtio_sqe *sqe, int result,
				       void *arg)
{
	ARG_UNUSED(result);
	const struct device *dev = (const struct device *)arg;
	struct adxl355_data *data = (struct adxl355_data *)dev->data;
	const struct adxl355_dev_config *cfg = (const struct adxl355_dev_config *)dev->config;
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

	gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_DISABLE);

	struct sensor_stream_trigger *fifo_wmark_cfg = NULL;

	for (int i = 0; i < read_config->count; ++i) {
		if (read_config->triggers[i].trigger == SENSOR_TRIG_FIFO_WATERMARK) {
			fifo_wmark_cfg = &read_config->triggers[i];
			continue;
		}
	}

	bool fifo_full_irq = false;

	if ((fifo_wmark_cfg != NULL) && FIELD_GET(ADXL355_INT_MAP_FIFO_FULL_EN1_MSK, status1)) {
		fifo_full_irq = true;
	}

	if (!fifo_full_irq) {
		gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);
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
	__ASSERT_NO_MSG(fifo_wmark_cfg != NULL);

	enum sensor_stream_data_opt data_opt = fifo_wmark_cfg->opt;

	if (data_opt == SENSOR_STREAM_DATA_NOP || data_opt == SENSOR_STREAM_DATA_DROP) {
		uint8_t *buf;
		uint32_t buf_len;

		/* Clear streaming_sqe since we're done with the call */
		data->sqe = NULL;
		if (rtio_sqe_rx_buf(current_sqe, sizeof(struct adxl355_fifo_data),
				    sizeof(struct adxl355_fifo_data), &buf, &buf_len) != 0) {
			rtio_iodev_sqe_err(current_sqe, -ENOMEM);
			gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio,
							GPIO_INT_EDGE_TO_ACTIVE);
			return;
		}

		struct adxl355_fifo_data *rx_data = (struct adxl355_fifo_data *)buf;

		memset(buf, 0, buf_len);
		rx_data->is_fifo = 1;
		rx_data->timestamp = data->timestamp;
		rx_data->status1 = status1;
		rx_data->fifo_samples = 0;
		rtio_iodev_sqe_ok(current_sqe, 0);

		if (data_opt == SENSOR_STREAM_DATA_DROP) {
			/* Flush the FIFO by disabling it. Save current mode for after the reset. */
			adxl355_flush_rtio(dev);
		}

		gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	struct rtio_sqe *write_fifo_addr = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *read_fifo_data = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *complete_op = rtio_sqe_acquire(data->rtio_ctx);
#ifdef CONFIG_SPI_RTIO
	uint8_t reg = ADXL355_SPI_READ(ADXL355_FIFO_ENTRIES);
#elif defined(CONFIG_I2C_RTIO)
	uint8_t reg = ADXL355_FIFO_ENTRIES;
#endif
	rtio_sqe_prep_tiny_write(write_fifo_addr, data->iodev, RTIO_PRIO_NORM, &reg, 1, NULL);
	write_fifo_addr->flags = RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_fifo_data, data->iodev, RTIO_PRIO_NORM, &data->fifo_samples, 1,
			   current_sqe);
	read_fifo_data->flags = RTIO_SQE_CHAINED;
#ifdef CONFIG_I2C_RTIO
	read_fifo_data->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
#endif
	rtio_sqe_prep_callback(complete_op, adxl355_process_fifo_samples_cb, (void *)dev, NULL);
	rtio_submit(data->rtio_ctx, 0);
}

/**
 * @brief ADXL355 stream IRQ handler
 *
 * @param dev Device pointer
 */
void adxl355_stream_irq_handler(const struct device *dev)
{
	int ret;
	struct adxl355_data *data = (struct adxl355_data *)dev->data;
	uint64_t cycles;

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
#if defined(CONFIG_SPI_RTIO)
	uint8_t reg = ADXL355_SPI_READ(ADXL355_STATUS);
#elif defined(CONFIG_I2C_RTIO)
	uint8_t reg = ADXL355_STATUS;
#endif
	rtio_sqe_prep_tiny_write(write_status_addr, data->iodev, RTIO_PRIO_NORM, &reg, 1, NULL);
	write_status_addr->flags |= RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_status_reg, data->iodev, RTIO_PRIO_NORM, &data->status1, 1, NULL);
	read_status_reg->flags |= RTIO_SQE_CHAINED;

#ifdef CONFIG_I2C_RTIO
	read_status_reg->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
#endif
	rtio_sqe_prep_callback(check_status_reg, adxl355_process_status1_cb, (void *)dev, NULL);
	rtio_submit(data->rtio_ctx, 0);
}
