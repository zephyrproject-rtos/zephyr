/*
 * Copyright (c) 2024 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_clock.h>
#include "adxl345.h"

LOG_MODULE_DECLARE(ADXL345, CONFIG_SENSOR_LOG_LEVEL);

static void adxl345_fifo_flush_rtio(const struct device *dev);

/* auxiliary functions */

int adxl345_rtio_reg_read(const struct device *dev, uint8_t reg,
			  uint8_t *buf, size_t buflen, void *userdata,
			  rtio_callback_t cb)
{
	struct adxl345_dev_data *data = (struct adxl345_dev_data *)dev->data;
	const struct adxl345_dev_config *cfg = dev->config;
	uint8_t r = buflen > 1 ? ADXL345_REG_READ_MULTIBYTE(reg) : ADXL345_REG_READ(reg);
	struct rtio_sqe *write_sqe = rtio_sqe_acquire(data->rtio_ctx);

	if (!write_sqe) {
		LOG_WRN("write_sqe failed");
		goto err;
	}
	rtio_sqe_prep_tiny_write(write_sqe, data->iodev, RTIO_PRIO_NORM, &r,
				 sizeof(r), NULL);
	write_sqe->flags |= RTIO_SQE_TRANSACTION;

	struct rtio_sqe *read_sqe = rtio_sqe_acquire(data->rtio_ctx);

	if (!read_sqe) {
		LOG_WRN("read_sqe failed");
		goto err;
	}

	rtio_sqe_prep_read(read_sqe, data->iodev, RTIO_PRIO_NORM,
			   buf, buflen, userdata);

	if (cfg->bus_type == ADXL345_BUS_I2C) {
		read_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
	}

	if (cb) {
		read_sqe->flags |= RTIO_SQE_CHAINED;
		struct rtio_sqe *check_status_sqe = rtio_sqe_acquire(data->rtio_ctx);

		if (!check_status_sqe) {
			LOG_WRN("check_status_sqe failed");
			goto err;
		}
		rtio_sqe_prep_callback_no_cqe(check_status_sqe, cb, (void *)dev,
					      userdata);
	}

	return rtio_submit(data->rtio_ctx, 0);
err:
	LOG_WRN("low on memory");
	return -ENOMEM;
}

static void adxl345_sqe_done(const struct device *dev,
			     struct rtio_iodev_sqe *iodev_sqe, int res)
{
	if (res < 0) {
		LOG_WRN("res == %d < 0)", res);
		rtio_iodev_sqe_err(iodev_sqe, res);
	} else {
		rtio_iodev_sqe_ok(iodev_sqe, res);
	}
	adxl345_set_gpios_en(dev, true);
}

/* streaming callbacks and calls */

static void adxl345_irq_en_cb(struct rtio *r, const struct rtio_sqe *sqe, int result, void *arg)
{
	ARG_UNUSED(result);

	const struct device *dev = (const struct device *)arg;

	adxl345_set_gpios_en(dev, true);
}

static void adxl345_fifo_read_cb(struct rtio *rtio_ctx, const struct rtio_sqe *sqe,
				 int result, void *arg)
{
	ARG_UNUSED(result);

	const struct device *dev = (const struct device *)arg;
	struct adxl345_dev_data *data = (struct adxl345_dev_data *) dev->data;
	struct rtio_iodev_sqe *iodev_sqe = sqe->userdata;

	adxl345_sqe_done(dev, iodev_sqe, data->fifo_entries);
}

static void adxl345_process_fifo_samples_cb(struct rtio *r, const struct rtio_sqe *sqe,
					    int result, void *arg)
{
	ARG_UNUSED(result);

	const struct device *dev = (const struct device *)arg;
	struct adxl345_dev_data *data = (struct adxl345_dev_data *) dev->data;
	struct rtio_iodev_sqe *current_sqe = data->sqe;
	data->fifo_entries = FIELD_GET(ADXL345_FIFO_ENTRIES_MSK, data->reg_fifo_status);
	uint8_t fifo_entries = data->fifo_entries;
	size_t sample_set_size = ADXL345_FIFO_SAMPLE_SIZE;
	uint16_t fifo_bytes = fifo_entries * ADXL345_FIFO_SAMPLE_SIZE;

	data->sqe = NULL;

	/* Not inherently an underrun/overrun as we may have a buffer to fill next time */
	if (current_sqe == NULL) {
		LOG_ERR("No pending SQE");
		adxl345_set_gpios_en(dev, true);
		return;
	}

	const size_t min_read_size = sizeof(struct adxl345_fifo_data) + sample_set_size;
	const size_t ideal_read_size = sizeof(struct adxl345_fifo_data) + fifo_bytes;

	uint8_t *buf;
	uint32_t buf_len;

	if (rtio_sqe_rx_buf(current_sqe, min_read_size, ideal_read_size, &buf, &buf_len) != 0) {
		LOG_ERR("Failed to get buffer");
		goto err;
	}
	LOG_DBG("Requesting buffer [%u, %u] got %u", (unsigned int)min_read_size,
		(unsigned int)ideal_read_size, buf_len);

	/* Read FIFO and call back to rtio with rtio_sqe completion */
	struct adxl345_fifo_data *hdr = (struct adxl345_fifo_data *) buf;

	hdr->is_fifo = 1;
	hdr->timestamp = data->timestamp;
	hdr->int_status = data->reg_int_source;
	hdr->is_full_res = data->is_full_res;
	hdr->selected_range = data->selected_range;
	hdr->accel_odr = data->odr;
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
		goto err;
	}


	for (size_t i = 0; i < fifo_entries; i++) {
		data->fifo_entries--;
		if (adxl345_rtio_reg_read(dev, ADXL345_REG_DATA_XYZ_REGS,
					  (read_buf + i * ADXL345_FIFO_SAMPLE_SIZE),
					  ADXL345_FIFO_SAMPLE_SIZE, current_sqe,
					  (i == fifo_entries - 1) ? adxl345_fifo_read_cb : NULL)) {
			LOG_WRN("RTIO reading the XYZ regs failed");
			goto err;
		}
		ARG_UNUSED(rtio_cqe_consume(data->rtio_ctx));
	}

	return;
err:
	LOG_WRN("Failed.");
	adxl345_sqe_done(dev, current_sqe, -ENOMEM);
}

static void adxl345_process_status1_cb(struct rtio *r, const struct rtio_sqe *sqe,
				       int result, void *arg)
{
	ARG_UNUSED(result);

	const struct device *dev = (const struct device *)arg;
	struct adxl345_dev_data *data = (struct adxl345_dev_data *) dev->data;
	struct rtio_iodev_sqe *current_sqe = data->sqe;
	struct sensor_read_config *read_config;
	uint8_t status1 = data->reg_int_source;

	if (data->sqe == NULL) {
		return;
	}

	read_config = (struct sensor_read_config *)data->sqe->sqe.iodev->data;

	if (read_config == NULL) {
		goto err;
	}

	if (read_config->is_streaming == false) {
		goto err;
	}

	adxl345_set_gpios_en(dev, false);

	struct sensor_stream_trigger *fifo_wmark_cfg = NULL;

	for (int i = 0; i < read_config->count; ++i) {
		if (read_config->triggers[i].trigger == SENSOR_TRIG_FIFO_WATERMARK) {
			fifo_wmark_cfg = &read_config->triggers[i];
			continue;
		}
	}

	bool fifo_full_irq = false;

	if (fifo_wmark_cfg && FIELD_GET(ADXL345_INT_WATERMARK, status1)) {
		fifo_full_irq = true;
	}

	if (!fifo_full_irq) {
		goto err;
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
		goto err;
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
			goto err;
		}

		struct adxl345_fifo_data *rx_data = (struct adxl345_fifo_data *)buf;

		memset(buf, 0, buf_len);
		rx_data->is_fifo = 1;
		rx_data->timestamp = data->timestamp;
		rx_data->int_status = status1;
		rx_data->fifo_byte_count = 0;

		if (data_opt == SENSOR_STREAM_DATA_DROP) {
			/* Flush the FIFO by disabling it. Save current mode for after the reset. */
			adxl345_fifo_flush_rtio(dev);
		}

		adxl345_sqe_done(dev, current_sqe, 0);
		return;
	}

	if (adxl345_rtio_reg_read(dev, ADXL345_FIFO_STATUS_REG, &data->reg_fifo_status,
				  sizeof(data->reg_fifo_status), current_sqe,
				  adxl345_process_fifo_samples_cb)) {
		LOG_WRN("Reading the FIFO samples failed");
		goto err;
	}

	return;
err:
	LOG_WRN("Failed.");
	adxl345_sqe_done(dev, current_sqe, -ENOMEM);
}

static void adxl345_fifo_flush_rtio(const struct device *dev)
{
	struct adxl345_dev_data *data = dev->data;
	uint8_t fifo_config;

	fifo_config = ADXL345_FIFO_CTL_TRIGGER_UNSET |
			adxl345_fifo_ctl_mode_init[ADXL345_FIFO_BYPASSED] |
			data->fifo_config.fifo_samples;

	struct rtio_sqe *write_fifo_addr = rtio_sqe_acquire(data->rtio_ctx);
	const uint8_t reg_addr_w2[2] = {ADXL345_FIFO_CTL_REG, fifo_config};

	rtio_sqe_prep_tiny_write(write_fifo_addr, data->iodev, RTIO_PRIO_NORM,
				 reg_addr_w2, 2, NULL);

	fifo_config = ADXL345_FIFO_CTL_TRIGGER_UNSET |
			adxl345_fifo_ctl_mode_init[data->fifo_config.fifo_mode] |
			data->fifo_config.fifo_samples;

	write_fifo_addr = rtio_sqe_acquire(data->rtio_ctx);
	const uint8_t reg_addr_w3[2] = {ADXL345_FIFO_CTL_REG, fifo_config};

	rtio_sqe_prep_tiny_write(write_fifo_addr, data->iodev, RTIO_PRIO_NORM,
				 reg_addr_w3, 2, NULL);
	write_fifo_addr->flags |= RTIO_SQE_CHAINED;

	struct rtio_sqe *complete_op = rtio_sqe_acquire(data->rtio_ctx);

	rtio_sqe_prep_callback(complete_op, adxl345_irq_en_cb, (void *)dev, NULL);
	rtio_submit(data->rtio_ctx, 0);
}

/* Consumer calls */

void adxl345_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg =
			(const struct sensor_read_config *)iodev_sqe->sqe.iodev->data;
	struct adxl345_dev_data *data = (struct adxl345_dev_data *)dev->data;
	uint8_t status;
	int rc;

	if (adxl345_set_gpios_en(dev, false)) {
		return;
	}

	for (size_t i = 0; i < cfg->count; i++) {
		if (cfg->triggers[i].trigger == SENSOR_TRIG_FIFO_WATERMARK) {
			rc = adxl345_reg_assign_bits(dev, ADXL345_INT_ENABLE_REG,
						     ADXL345_INT_WATERMARK, true);
			if (rc) {
				LOG_WRN("adxl345_reg_assign_bits() failed");
				return;
			}
		}

		/* Flush the FIFO by disabling it. Save current mode for after the reset. */
		enum adxl345_fifo_mode current_fifo_mode = data->fifo_config.fifo_mode;

		if (current_fifo_mode == ADXL345_FIFO_BYPASSED) {
			current_fifo_mode = ADXL345_FIFO_STREAMED;
		}
		adxl345_configure_fifo(dev, ADXL345_FIFO_BYPASSED,
				       data->fifo_config.fifo_trigger,
				       data->fifo_config.fifo_samples);
		adxl345_configure_fifo(dev, current_fifo_mode,
				       data->fifo_config.fifo_trigger,
				       data->fifo_config.fifo_samples);
		rc = adxl345_reg_read_byte(dev, ADXL345_FIFO_STATUS_REG, &status);
	}

	if (adxl345_set_gpios_en(dev, true)) {
		return;
	}

	data->sqe = iodev_sqe;
}

void adxl345_stream_irq_handler(const struct device *dev)
{
	struct adxl345_dev_data *data = (struct adxl345_dev_data *) dev->data;
	struct rtio_iodev_sqe *current_sqe = data->sqe;
	uint64_t cycles;
	int rc;

	if (data->sqe == NULL) {
		return;
	}

	rc = sensor_clock_get_cycles(&cycles);
	if (rc != 0) {
		LOG_ERR("Failed to get sensor clock cycles");
		goto err;
	}

	data->timestamp = sensor_clock_cycles_to_ns(cycles);

	if (adxl345_rtio_reg_read(dev, ADXL345_INT_SOURCE_REG,
				  &data->reg_int_source,
				  sizeof(data->reg_int_source),
				  NULL, adxl345_process_status1_cb)) {
		LOG_ERR("Processing the FIFO status failed");
		goto err;
	}

	adxl345_sqe_done(dev, current_sqe, 0);

	return;
err:
	LOG_WRN("Failed.");
	adxl345_sqe_done(dev, current_sqe, -ENOMEM);
}
