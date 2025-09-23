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

static void adxl345_rtio_init_hdr(struct adxl345_fifo_data *hdr,
				  struct adxl345_dev_data *data,
				  uint32_t fifo_byte_count)
{
	hdr->is_fifo = 1;
	hdr->timestamp = data->timestamp;
	hdr->int_status = data->reg_int_source;
	hdr->is_full_res = data->is_full_res;
	hdr->selected_range = data->selected_range;
	hdr->accel_odr = data->odr;
	hdr->sample_set_size = ADXL345_FIFO_SAMPLE_SIZE;

	hdr->fifo_byte_count = fifo_byte_count;
}

static int adxl345_rtio_init_buffer(struct adxl345_dev_data *data,
				    uint8_t **read_buf, uint16_t fifo_bytes,
				    struct rtio_iodev_sqe *current_iodev_sqe)
{
	const size_t min_read_size = sizeof(struct adxl345_fifo_data) + ADXL345_FIFO_SAMPLE_SIZE;
	const size_t ideal_read_size = sizeof(struct adxl345_fifo_data) + fifo_bytes;
	uint8_t *buf;
	uint32_t buf_avail, buf_length;

	if (rtio_sqe_rx_buf(current_iodev_sqe, min_read_size, ideal_read_size, &buf,
			    &buf_length)) {
		LOG_ERR("Failed to get buffer");
		return -EINVAL;
	}
	buf_avail = buf_length - sizeof(struct adxl345_fifo_data);
	uint32_t read_len = MIN(fifo_bytes, buf_avail);

	if (buf_avail < fifo_bytes) {
		uint32_t pkts = read_len / ADXL345_FIFO_SAMPLE_SIZE;

		read_len = pkts * ADXL345_FIFO_SAMPLE_SIZE;
	}

	__ASSERT_NO_MSG(read_len % ADXL345_FIFO_SAMPLE_SIZE == 0);
	adxl345_rtio_init_hdr((struct adxl345_fifo_data *)buf, data, read_len);

	*read_buf = buf + sizeof(struct adxl345_fifo_data);

	return 0;
}

static int adxl345_rtio_cqe_consume(struct adxl345_dev_data *data)
{
	struct rtio_cqe *cqe = rtio_cqe_consume(data->rtio_ctx);
	int res = 0;

	do {
		cqe = rtio_cqe_consume(data->rtio_ctx);
		if (cqe) {
			if ((cqe->result < 0 && res == 0)) {
				LOG_ERR("Bus error: %d", cqe->result);
				res = cqe->result;
			}
			rtio_cqe_release(data->rtio_ctx, cqe);
		}
	} while (cqe);

	return res;
}

static int adxl345_check_streaming(struct adxl345_dev_data *data,
				   struct sensor_read_config **ptr_read_config)
{
	if (!data->sqe) {
		LOG_ERR("data->iodev_sqe was NULL");
		return false;
	}

	*ptr_read_config = (struct sensor_read_config *)data->sqe->sqe.iodev->data;
	struct sensor_read_config *read_config = *ptr_read_config;

	if (!read_config) {
		LOG_WRN("read_config was NULL");
		return false;
	}

	if (!read_config->is_streaming) {
		LOG_WRN("is_streaming of read_config was false/NULL");
		return false;
	}

	return true;
}

/* streaming callbacks and calls */

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
	uint16_t fifo_bytes = fifo_entries * ADXL345_FIFO_SAMPLE_SIZE;

	data->sqe = NULL;

	/* Not inherently an underrun/overrun as we may have a buffer to fill next time */
	if (current_sqe == NULL) {
		LOG_ERR("No pending SQE");
		adxl345_set_gpios_en(dev, true);
		return;
	}

	uint8_t *read_buf;

	if (adxl345_rtio_init_buffer(data, &read_buf, fifo_bytes, current_sqe)) {
		goto err;
	}

	/* Flush completions */
	if (adxl345_rtio_cqe_consume(data)) {
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

	if (!adxl345_check_streaming(data, &read_config)) {
		LOG_WRN("Failed! RTIO not setup for streaming");
		return;
	}

	struct sensor_stream_trigger *fifo_wmark_cfg = NULL;
	struct sensor_stream_trigger *fifo_full_cfg = NULL;
	enum sensor_stream_data_opt data_opt = SENSOR_STREAM_DATA_DROP;

	for (int i = 0; i < read_config->count; ++i) {
		switch (read_config->triggers[i].trigger) {
		case SENSOR_TRIG_FIFO_WATERMARK:
			if (FIELD_GET(ADXL345_INT_WATERMARK, data->reg_int_source)) {
				fifo_wmark_cfg = &read_config->triggers[i];
				data_opt = MIN(data_opt, fifo_wmark_cfg->opt);
			}
			continue;
		case SENSOR_TRIG_FIFO_FULL:
			if (FIELD_GET(ADXL345_INT_OVERRUN, data->reg_int_source)) {
				fifo_full_cfg = &read_config->triggers[i];
				data_opt = MIN(data_opt, fifo_full_cfg->opt);
			}
			continue;
		default:
			LOG_WRN("SENSOR_* case not covered");
			goto err;
		}
	}

	/* Flush completions, in case cancel out */
	if (adxl345_rtio_cqe_consume(data)) {
		LOG_WRN("CQE consume failed");
		goto err;
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

		memset(buf, 0, buf_len);
		adxl345_rtio_init_hdr((struct adxl345_fifo_data *)buf, data, 0);

		if (data_opt == SENSOR_STREAM_DATA_DROP) {
			/*
			 * Flush the FIFO by disabling it. Save current mode
			 * for after the reset.
			 */
			adxl345_rtio_flush_fifo(dev);
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

/**
 * adxl345_rtio_flush_fifo - Reset the FIFO and interrupt status registers.
 * @dev The device node.
 *
 * Consume all FIFO elements, since this not just resets FIFO_STATUS, but also INT_SOURCE entries.
 * When using Analog's STREAM FIFO mode, FIFO elements must be consumed. For Analog's
 * TRIGGER FIFO mode, it would be sufficient to switch FIFO modes to BYPASS FIFO mode and
 * back to Analog's STREAM FIFO mode, since in this mode, the FIFO gets only activated once if the
 * trigger bit of the sensor is set by one of the sensor events, i.e. in terms of Analog Devices a
 * "trigger event". The INT SOURCE does not need to be reset here, but using the sensor in a
 * permanent sensing mode, such as Analog's STREAM FIFO mode requires a reset of the INT_SOURCE
 * register, too. Thus, it is required to consume the FIFO elements.
 *
 * @return 0 for success, or error number.
 */
int adxl345_rtio_flush_fifo(const struct device *dev)
{
	struct adxl345_dev_data *data = dev->data;
	struct rtio_iodev_sqe *current_sqe = data->sqe;
	int8_t fifo_entries = ADXL345_MAX_FIFO_SIZE;
	struct sensor_read_config *read_config = NULL;

	if (!adxl345_check_streaming(data, &read_config)) {
		LOG_WRN("adxl345_check_streaming() failed");
		return 0;
	}

	if (adxl345_rtio_cqe_consume(data)) {
		LOG_WRN("adxl345_rtio_cqe_consume() failed");
		goto err;
	}

	if (adxl345_set_measure_en(dev, false)) {
		LOG_WRN("adxl345_set_measure_en() failed");
		goto err;
	}

	const struct adxl345_dev_config *cfg = dev->config;
	uint8_t reg = ADXL345_REG_DATA_XYZ_REGS;
	uint8_t reg_addr = ADXL345_REG_READ_MULTIBYTE(reg);
	size_t buflen = ADXL345_FIFO_SAMPLE_SIZE;

	for (size_t i = 0; i < fifo_entries; i++) {
		uint8_t dummy[6];
		uint8_t *buf = dummy;
		struct rtio_sqe *write_sqe = rtio_sqe_acquire(data->rtio_ctx);

		if (!write_sqe) {
			LOG_WRN("write_sqe failed");
			goto err;
		}
		rtio_sqe_prep_tiny_write(write_sqe, data->iodev,
					 RTIO_PRIO_NORM, &reg_addr,
					 sizeof(reg_addr), NULL);
		write_sqe->flags |= RTIO_SQE_TRANSACTION;

		struct rtio_sqe *read_sqe = rtio_sqe_acquire(data->rtio_ctx);

		if (!read_sqe) {
			LOG_WRN("read_sqe failed");
			goto err;
		}
		rtio_sqe_prep_read(read_sqe, data->iodev, RTIO_PRIO_NORM,
				   buf, buflen,
				   current_sqe);

		if (cfg->bus_type == ADXL345_BUS_I2C) {
			read_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
		}
		rtio_submit(data->rtio_ctx, 2); /* cautiously keep blocking */
		ARG_UNUSED(rtio_cqe_consume(data->rtio_ctx));
	}

	if (adxl345_set_measure_en(dev, true)) {
		LOG_WRN("adxl345_set_measure_en() failed");
		goto err;
	}

	return 0;
err:
	LOG_WRN("Failed.");
	return -EINVAL;
}

/* Consumer calls */

static bool first = true;
void adxl345_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg =
			(const struct sensor_read_config *)iodev_sqe->sqe.iodev->data;
	struct adxl345_dev_data *data = (struct adxl345_dev_data *)dev->data;
	int rc;

	data->sqe = iodev_sqe;

	if (first) {
		/* Initialize measurement and start with flushed registers */
		adxl345_rtio_flush_fifo(dev);
		adxl345_set_gpios_en(dev, true);
		first = false;
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
	}
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
