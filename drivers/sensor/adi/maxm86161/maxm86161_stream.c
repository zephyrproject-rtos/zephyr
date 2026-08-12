/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_clock.h>

#include "maxm86161.h"

LOG_MODULE_DECLARE(MAXM86161, CONFIG_SENSOR_LOG_LEVEL);

/* Re-arm the INT edge interrupt; log (but do not otherwise act on) failure. */
static void maxm86161_stream_reenable_irq(const struct device *dev)
{
	const struct maxm86161_config *config = dev->config;
	int ret;

	ret = gpio_pin_interrupt_configure_dt(&config->interrupt_gpio, GPIO_INT_EDGE_FALLING);
	if (ret) {
		LOG_ERR("Failed to re-enable INT interrupt: %d", ret);
	}
}

/*
 * Central terminal-error handler for the streaming chain: fail any pending SQE,
 * clear the in-progress flag, and re-arm the interrupt.
 */
static void maxm86161_stream_abort(const struct device *dev, int err)
{
	struct maxm86161_data *data = dev->data;

	if (data->iodev_sqe) {
		rtio_iodev_sqe_err(data->iodev_sqe, err);
		data->iodev_sqe = NULL;
	}
	atomic_clear_bit(&data->stream_mode, 0);
	maxm86161_stream_reenable_irq(dev);
}

void maxm86161_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *sensor_cfg = iodev_sqe->sqe.iodev->data;
	const struct maxm86161_config *config = dev->config;
	struct maxm86161_data *data = dev->data;
	uint8_t fifo_watermark_irq = 0U;
	int ret = 0;

	k_mutex_lock(&data->trigger_mutex, K_FOREVER);

	ret = gpio_pin_interrupt_configure_dt(&config->interrupt_gpio, GPIO_INT_DISABLE);
	if (ret) {
		LOG_ERR("Failed to disable INT GPIO");
		goto err_unlock;
	}

	for (size_t i = 0; i < sensor_cfg->count; i++) {
		if (sensor_cfg->triggers[i].trigger == SENSOR_TRIG_FIFO_WATERMARK) {
			fifo_watermark_irq = 1U;
			break;
		}
	}

	if (fifo_watermark_irq != data->fifo_watermark) {
		data->fifo_watermark = fifo_watermark_irq;

		ret = maxm86161_i2c_update_byte(dev, MAXM86161_REG_INT_ENABLE1,
						MAXM86161_MSK_INT_ENABLE1_A_FULL_EN,
						fifo_watermark_irq);
		if (ret) {
			LOG_ERR("Failed to set A_FULL INT");
			goto err_unlock;
		}

		/* Flush FIFO */
		ret = maxm86161_i2c_update_byte(dev, MAXM86161_REG_FIFO_CONFIG2,
						MAXM86161_MSK_FIFO_FLUSH, 1U);
		if (ret) {
			LOG_ERR("Failed to flush FIFO");
			goto err_unlock;
		}

		/* Clear STATUS flags by reading STATUS1/STATUS2 */
		uint8_t status[2];

		ret = maxm86161_i2c_burst_read(dev, MAXM86161_REG_INT_STATUS1, status,
					       sizeof(status));
		if (ret) {
			LOG_ERR("Failed to flush STATUS registers");
			goto err_unlock;
		}
	}

	atomic_set_bit(&data->stream_mode, 0);
	data->iodev_sqe = iodev_sqe;

	ret = gpio_pin_interrupt_configure_dt(&config->interrupt_gpio, GPIO_INT_EDGE_FALLING);
	if (ret) {
		LOG_ERR("Failed to enable INT GPIO");
		data->iodev_sqe = NULL;
		atomic_clear_bit(&data->stream_mode, 0);
		goto err_unlock;
	}

	k_mutex_unlock(&data->trigger_mutex);
	return;

err_unlock:
	k_mutex_unlock(&data->trigger_mutex);
	rtio_iodev_sqe_err(iodev_sqe, ret);
}

static void maxm86161_int_en_cb(struct rtio *r, const struct rtio_sqe *sqe, int res, void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct maxm86161_data *data = dev->data;

	if (res < 0) {
		LOG_ERR("I2C operation failed: %d", res);
		maxm86161_stream_abort(dev, res);
		return;
	}

	atomic_clear_bit(&data->stream_mode, 0);
	maxm86161_stream_reenable_irq(dev);
	LOG_DBG("Complete FIFO flush callback chain");
}

static void maxm86161_flush_fifo_write_cb(struct rtio *r, const struct rtio_sqe *sqe, int res,
					  void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct maxm86161_data *data = dev->data;
	uint8_t fifo_config = data->fifo_config_cache | MAXM86161_MSK_FIFO_FLUSH;

	if (res < 0) {
		LOG_ERR("I2C operation failed: %d", res);
		maxm86161_stream_abort(dev, res);
		return;
	}

	struct rtio_sqe *write_sqe = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *complete_sqe = rtio_sqe_acquire(data->rtio_ctx);

	if (!write_sqe || !complete_sqe) {
		LOG_ERR("RTIO SQE pool exhausted");
		rtio_sqe_drop_all(data->rtio_ctx);
		maxm86161_stream_abort(dev, -ENOMEM);
		return;
	}

	uint8_t write_data[] = {MAXM86161_REG_FIFO_CONFIG2, fifo_config};

	rtio_sqe_prep_tiny_write(write_sqe, data->iodev, RTIO_PRIO_NORM, write_data,
				 sizeof(write_data), NULL);
	write_sqe->flags |= RTIO_SQE_CHAINED;

	rtio_sqe_prep_callback_no_cqe(complete_sqe, maxm86161_int_en_cb, (void *)dev, NULL);
	rtio_submit(data->rtio_ctx, 0);
}

static void maxm86161_flush_fifo(const struct device *dev)
{
	struct maxm86161_data *data = dev->data;

	struct rtio_sqe *reg_sqe = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *read_sqe = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *wr_sqe = rtio_sqe_acquire(data->rtio_ctx);

	if (!reg_sqe || !read_sqe || !wr_sqe) {
		LOG_ERR("RTIO SQE pool exhausted");
		rtio_sqe_drop_all(data->rtio_ctx);
		maxm86161_stream_abort(dev, -ENOMEM);
		return;
	}

	uint8_t reg[] = {MAXM86161_REG_FIFO_CONFIG2};

	rtio_sqe_prep_tiny_write(reg_sqe, data->iodev, RTIO_PRIO_NORM, reg, sizeof(reg), NULL);
	reg_sqe->flags |= RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_sqe, data->iodev, RTIO_PRIO_NORM, &data->fifo_config_cache, 1,
			   NULL);
	read_sqe->flags |= RTIO_SQE_CHAINED;
	read_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;

	rtio_sqe_prep_callback_no_cqe(wr_sqe, maxm86161_flush_fifo_write_cb, (void *)dev, NULL);
	rtio_submit(data->rtio_ctx, 0);
}

static void maxm86161_complete_op_cb(struct rtio *r, const struct rtio_sqe *sqe, int res, void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct maxm86161_data *data = dev->data;
	struct rtio_iodev_sqe *current_sqe = (struct rtio_iodev_sqe *)sqe->userdata;

	if (res < 0) {
		LOG_ERR("I2C operation failed: %d", res);
		maxm86161_stream_abort(dev, res);
		return;
	}

	atomic_clear_bit(&data->stream_mode, 0);
	rtio_iodev_sqe_ok(current_sqe, 0);
	maxm86161_stream_reenable_irq(dev);
	LOG_DBG("Complete stream callback chain");
}

static void maxm86161_process_fifo_samples_cb(struct rtio *r, const struct rtio_sqe *sqr, int res,
					      void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct maxm86161_data *data = dev->data;
	struct rtio_iodev_sqe *current_sqe = data->iodev_sqe;
	uint8_t fifo_ovf_count =
		FIELD_GET(MAXM86161_MSK_FIFO_OVF_COUNTER, data->fifo_count_regs[0]);
	uint8_t fifo_sample_count = data->fifo_count_regs[1];
	uint16_t fifo_bytes = (uint16_t)fifo_sample_count * MAXM86161_FIFO_SAMPLE_SIZE;
	int ret = 0;

	if (res < 0) {
		LOG_ERR("I2C operation failed: %d", res);
		maxm86161_stream_abort(dev, res);
		return;
	}

	data->iodev_sqe = NULL;

	if (current_sqe == NULL) {
		LOG_ERR("No pending SQE");
		rtio_sqe_drop_all(data->rtio_ctx);
		maxm86161_stream_reenable_irq(dev);
		return;
	}

	if (fifo_ovf_count > 0) {
		LOG_WRN("FIFO overflow: %u samples lost", fifo_ovf_count);
	}

	const size_t min_read_size = sizeof(struct maxm86161_fifo_hdr) + MAXM86161_FIFO_SAMPLE_SIZE;
	const size_t ideal_read_size = sizeof(struct maxm86161_fifo_hdr) + fifo_bytes;
	uint8_t *buf;
	uint32_t buf_len;

	if (rtio_sqe_rx_buf(current_sqe, min_read_size, ideal_read_size, &buf, &buf_len) != 0) {
		LOG_ERR("Failed to get SQE RX buffer");
		rtio_iodev_sqe_err(current_sqe, -ENOMEM);
		maxm86161_stream_reenable_irq(dev);
		return;
	}

	struct maxm86161_fifo_hdr *hdr = (struct maxm86161_fifo_hdr *)buf;

	hdr->is_fifo = 1;
	hdr->timestamp = data->timestamp;
	hdr->int_status = data->status1_cache;
	hdr->odr = data->odr;
	hdr->fifo_ovf_count = fifo_ovf_count;

	uint32_t buf_avail = buf_len - sizeof(*hdr);
	uint32_t read_len = ROUND_DOWN(MIN(fifo_bytes, buf_avail), MAXM86161_FIFO_SAMPLE_SIZE);
	uint8_t *read_buf = buf + sizeof(*hdr);

	__ASSERT_NO_MSG(read_len % MAXM86161_FIFO_SAMPLE_SIZE == 0);

	if (read_len < fifo_bytes) {
		LOG_WRN("RX buffer truncates FIFO: %u of %u bytes", read_len, fifo_bytes);
	}

	LOG_INF("Retrieving %d samples in this stream batch.", read_len / 3);

	hdr->fifo_byte_count = read_len;
	hdr->fifo_samples = read_len / MAXM86161_FIFO_SAMPLE_SIZE;

	/* Flush completions from the previous chain */
	struct rtio_cqe *cqe;

	do {
		cqe = rtio_cqe_consume(data->rtio_ctx);
		if (cqe) {
			if ((cqe->result < 0) && (ret == 0)) {
				ret = cqe->result;
			}
			rtio_cqe_release(data->rtio_ctx, cqe);
		}
	} while (cqe);

	if (ret != 0) {
		LOG_ERR("Bus error: %d", ret);
		rtio_iodev_sqe_err(current_sqe, ret);
		maxm86161_stream_reenable_irq(dev);
		return;
	}

	uint8_t reg[] = {MAXM86161_REG_FIFO_DATA_REGISTER};

	struct rtio_sqe *write_reg_addr = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *read_data_addr = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *complete_op = rtio_sqe_acquire(data->rtio_ctx);

	if (!write_reg_addr || !read_data_addr || !complete_op) {
		LOG_ERR("RTIO SQE pool exhausted");
		rtio_sqe_drop_all(data->rtio_ctx);
		rtio_iodev_sqe_err(current_sqe, -ENOMEM);
		maxm86161_stream_reenable_irq(dev);
		return;
	}

	rtio_sqe_prep_tiny_write(write_reg_addr, data->iodev, RTIO_PRIO_NORM, reg,
				 sizeof(reg), NULL);
	write_reg_addr->flags |= RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_data_addr, data->iodev, RTIO_PRIO_NORM, read_buf, read_len, NULL);
	read_data_addr->flags |= RTIO_SQE_CHAINED;
	read_data_addr->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
	rtio_sqe_prep_callback_no_cqe(complete_op, maxm86161_complete_op_cb, (void *)dev,
				      (void *)current_sqe);
	complete_op->flags |= RTIO_SQE_NO_RESPONSE;
	rtio_submit(data->rtio_ctx, 0);
}

static void maxm86161_process_status_cb(struct rtio *r, const struct rtio_sqe *sqr, int res,
					void *arg)
{
	const struct device *dev = (const struct device *)arg;
	const struct maxm86161_config *config = dev->config;
	struct maxm86161_data *data = dev->data;
	struct rtio_iodev_sqe *current_sqe = data->iodev_sqe;
	uint8_t status1 = data->status1_cache;
	int ret = 0;

	if (res < 0) {
		LOG_ERR("I2C operation failed: %d", res);
		maxm86161_stream_abort(dev, res);
		return;
	}

	data->status1_cache_ready = true;

	if (!current_sqe) {
		LOG_ERR("No pending SQE");
		maxm86161_stream_abort(dev, -EINVAL);
		return;
	}

	struct sensor_read_config *read_config =
	    (struct sensor_read_config *)current_sqe->sqe.iodev->data;

	if (read_config == NULL || !read_config->is_streaming) {
		LOG_ERR("Invalid streaming config");
		maxm86161_stream_abort(dev, -EINVAL);
		return;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->interrupt_gpio, GPIO_INT_DISABLE);
	if (ret) {
		maxm86161_stream_abort(dev, ret);
		return;
	}

	struct sensor_stream_trigger *fifo_watermark_cfg = NULL;

	for (int i = 0; i < read_config->count; ++i) {
		if (read_config->triggers[i].trigger == SENSOR_TRIG_FIFO_WATERMARK) {
			fifo_watermark_cfg = &read_config->triggers[i];
			continue;
		}
	}

	/* Track object detection and note the rising (object-present) transition. */
	if (FIELD_GET(MAXM86161_MSK_INT_STATUS1_PROX_INT, status1)) {
		bool was_detected = data->prox_attr.object_detected;

		data->prox_attr.object_detected = !data->prox_attr.object_detected;
		if (!was_detected && data->prox_attr.object_detected) {
			data->prox_attr.prox_transition_time = k_uptime_get();
		}
	}

	bool fifo_watermark_irq =
	    (fifo_watermark_cfg != NULL) && FIELD_GET(MAXM86161_MSK_INT_STATUS1_A_FULL, status1);

	if (!fifo_watermark_irq) {
		LOG_DBG("Not a FIFO watermark interrupt");
		maxm86161_stream_reenable_irq(dev);
		return;
	}

	/* Flush completions from the status read chain */
	struct rtio_cqe *cqe;

	do {
		cqe = rtio_cqe_consume(data->rtio_ctx);
		if (cqe) {
			if ((cqe->result < 0) && (ret == 0)) {
				ret = cqe->result;
			}
			rtio_cqe_release(data->rtio_ctx, cqe);
		}
	} while (cqe);

	if (ret != 0) {
		LOG_ERR("Bus error: %d", ret);
		rtio_iodev_sqe_err(current_sqe, ret);
		data->iodev_sqe = NULL;
		maxm86161_stream_reenable_irq(dev);
		return;
	}

	enum sensor_stream_data_opt data_opt = fifo_watermark_cfg->opt;
	bool settle_suppress = false;

	/*
	 * Picket-fence settling: while proximity mode is active, suppress the
	 * FIFO burst that immediately follows a proximity transition, since those
	 * PROX samples are not yet stable.
	 */
	settle_suppress =
	    data->prox_attr.enabled &&
	    (k_uptime_get() - data->prox_attr.prox_transition_time) < MAXM86161_PROX_SETTLE_MS;

	if (data_opt == SENSOR_STREAM_DATA_NOP || data_opt == SENSOR_STREAM_DATA_DROP ||
	    settle_suppress) {
		uint8_t *buf;
		uint32_t buf_len;

		data->iodev_sqe = NULL;
		if (rtio_sqe_rx_buf(current_sqe, sizeof(struct maxm86161_fifo_hdr),
				    sizeof(struct maxm86161_fifo_hdr), &buf, &buf_len) != 0) {
			LOG_ERR("Failed to get SQE RX buffer");
			rtio_iodev_sqe_err(current_sqe, -ENOMEM);
			maxm86161_stream_reenable_irq(dev);
			return;
		}

		/* empty header data for decoder during drop sequence */
		struct maxm86161_fifo_hdr *hdr = (struct maxm86161_fifo_hdr *)buf;

		memset(buf, 0, buf_len);
		hdr->is_fifo = 1;
		hdr->timestamp = data->timestamp;
		hdr->int_status = status1;
		hdr->odr = data->odr;
		rtio_iodev_sqe_ok(current_sqe, 0);

		/* DROP and settle-suppression both discard the pending FIFO contents. */
		if (data_opt == SENSOR_STREAM_DATA_DROP || settle_suppress) {
			maxm86161_flush_fifo(dev);
			return;
		}

		atomic_clear_bit(&data->stream_mode, 0);
		maxm86161_stream_reenable_irq(dev);
		return;
	}

	struct rtio_sqe *write_reg_addr = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *read_fifo_addr = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *check_fifo = rtio_sqe_acquire(data->rtio_ctx);

	if (!write_reg_addr || !read_fifo_addr || !check_fifo) {
		LOG_ERR("RTIO SQE pool exhausted");
		rtio_sqe_drop_all(data->rtio_ctx);
		rtio_iodev_sqe_err(current_sqe, -ENOMEM);
		data->iodev_sqe = NULL;
		maxm86161_stream_reenable_irq(dev);
		return;
	}

	/*
	 * Read OVF_COUNTER (0x06) and FIFO_DATA_COUNTER (0x07) in one burst so
	 * overflow can be detected before draining the FIFO.
	 */
	uint8_t reg[] = {MAXM86161_REG_FIFO_OVF_COUNTER};

	rtio_sqe_prep_tiny_write(write_reg_addr, data->iodev, RTIO_PRIO_NORM, reg, sizeof(reg),
				 NULL);
	write_reg_addr->flags |= RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_fifo_addr, data->iodev, RTIO_PRIO_NORM, data->fifo_count_regs,
			   sizeof(data->fifo_count_regs), NULL);
	read_fifo_addr->flags |= RTIO_SQE_CHAINED;
	read_fifo_addr->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;

	rtio_sqe_prep_callback_no_cqe(check_fifo, maxm86161_process_fifo_samples_cb, (void *)dev,
				      NULL);
	check_fifo->flags |= RTIO_SQE_NO_RESPONSE;
	rtio_submit(data->rtio_ctx, 0);
	LOG_DBG("RTIO submitted for FIFO count read");
}

void maxm86161_stream_irq_handler(const struct device *dev)
{
	struct maxm86161_data *data = dev->data;
	struct rtio_iodev_sqe *current_sqe = data->iodev_sqe;
	int ret = 0;
	uint64_t cycles = 0U;

	if (!current_sqe) {
		LOG_ERR("No pending SQE");
		maxm86161_stream_reenable_irq(dev);
		return;
	}

	ret = sensor_clock_get_cycles(&cycles);
	if (ret) {
		LOG_ERR("Failed to get sensor clock cycles");
		maxm86161_stream_abort(dev, ret);
		return;
	}

	data->timestamp = sensor_clock_cycles_to_ns(cycles);

	struct rtio_sqe *write_reg_addr = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *read_status_addr = rtio_sqe_acquire(data->rtio_ctx);
	struct rtio_sqe *check_status = rtio_sqe_acquire(data->rtio_ctx);

	if (!write_reg_addr || !read_status_addr || !check_status) {
		LOG_ERR("RTIO SQE pool exhausted");
		rtio_sqe_drop_all(data->rtio_ctx);
		maxm86161_stream_abort(dev, -ENOMEM);
		return;
	}

	uint8_t reg[] = {MAXM86161_REG_INT_STATUS1};

	rtio_sqe_prep_tiny_write(write_reg_addr, data->iodev, RTIO_PRIO_NORM, reg, sizeof(reg),
				 NULL);
	write_reg_addr->flags |= RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_status_addr, data->iodev, RTIO_PRIO_NORM,
			   &data->status1_cache, 1, NULL);
	read_status_addr->flags |= RTIO_SQE_CHAINED;
	read_status_addr->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;

	rtio_sqe_prep_callback_no_cqe(check_status, maxm86161_process_status_cb, (void *)dev, NULL);
	check_status->flags |= RTIO_SQE_NO_RESPONSE;
	rtio_submit(data->rtio_ctx, 0);
	LOG_DBG("RTIO submitted for STATUS1 read");
}
