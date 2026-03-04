/*
 * Copyright (c) 2025 Lothar Rubusch <l.rubusch@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include "adxl313.h"

LOG_MODULE_DECLARE(ADXL313, CONFIG_SENSOR_LOG_LEVEL);

/**
 * @brief Read one or more registers from the ADXL313 sensor via RTIO.
 *
 * This function performs a register read operation on the ADXL313 device using
 * the RTIO subsystem. It supports both single-byte and multi-byte reads.
 *
 * The operation is composed of two chained RTIO submission queue entries (SQEs):
 *   1. A write SQE to send the register address to the device.
 *   2. A read SQE to receive the data from the device.
 *
 * For I²C buses, appropriate stop/restart flags are set to ensure correct
 * transaction semantics.
 *
 * If a callback @p cb is provided, it will be invoked after the read completes.
 * In that case, an additional chained callback SQE is appended.
 *
 * @param dev      Pointer to the device structure for the driver instance.
 * @param reg      Register address to read from (must be a valid ADXL313 register).
 * @param buf      Buffer into which the read data will be placed. Must be at
 *                 least @p buflen bytes in size.
 * @param buflen   Number of bytes to read. If > 1, multi-byte read mode is used.
 * @param userdata User-provided context passed to the callback (if any).
 * @param cb       Optional callback function to invoke upon completion of the read.
 *                 If NULL, the function waits synchronously for completion.
 *
 * @retval 0        On success.
 * @retval -ENOMEM  If RTIO submission queue entries could not be acquired
 *                  (e.g., due to resource exhaustion).
 */
int adxl313_rtio_reg_read(const struct device *dev, uint8_t reg, uint8_t *buf, size_t buflen,
			  void *userdata, rtio_callback_t cb)
{
	struct adxl313_dev_data *data = (struct adxl313_dev_data *)dev->data;
	const struct adxl313_dev_config *cfg = dev->config;
	uint8_t r = buflen > 1 ? ADXL313_REG_READ_MULTIBYTE(reg) : ADXL313_REG_READ(reg);
	struct rtio_sqe *write_sqe = rtio_sqe_acquire(data->rtio_ctx);

	if (!write_sqe) {
		LOG_WRN("write_sqe failed");
		goto err;
	}
	rtio_sqe_prep_tiny_write(write_sqe, data->iodev, RTIO_PRIO_NORM, &r, sizeof(r), NULL);
	write_sqe->flags |= RTIO_SQE_TRANSACTION;

	struct rtio_sqe *read_sqe = rtio_sqe_acquire(data->rtio_ctx);

	if (!read_sqe) {
		LOG_WRN("read_sqe failed");
		goto err;
	}

	rtio_sqe_prep_read(read_sqe, data->iodev, RTIO_PRIO_NORM, buf, buflen, userdata);

	if (cfg->bus_type == ADXL313_BUS_I2C) {
		read_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
	}

	if (cb) {
		read_sqe->flags |= RTIO_SQE_CHAINED;

		struct rtio_sqe *check_status_sqe = rtio_sqe_acquire(data->rtio_ctx);

		if (!check_status_sqe) {
			LOG_WRN("check_status_sqe failed");
			goto err;
		}
		rtio_sqe_prep_callback_no_cqe(check_status_sqe, cb, (void *)dev, userdata);
	}

	return rtio_submit(data->rtio_ctx, 0);
err:
	LOG_WRN("low on memory");
	return -ENOMEM;
}

/**
 * @brief Resolve done conditions.
 *
 * @param dev The device structure. NULL is not allowed.
 * @param[in] iodev_sqe In most cases this is data->iodev_sqe; when this is set to NULL, the active
 *        current_iodev pointer must be passed. NULL is allowed.
 * @param res The error state, 0 if fine, else a negative error.
 */
static void adxl313_sqe_done(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe, int res)
{
	if (res < 0) {
		LOG_WRN("res == %d < 0)", res);
		rtio_iodev_sqe_err(iodev_sqe, res);
	} else {
		rtio_iodev_sqe_ok(iodev_sqe, res);
	}
}

static void adxl313_rtio_init_hdr(struct adxl313_fifo_data *hdr, struct adxl313_dev_data *data,
				  uint32_t fifo_byte_count)
{
	hdr->is_fifo = 1;
	hdr->timestamp = data->timestamp;
	hdr->int_status = data->reg_int_source;
	hdr->is_full_res = data->is_full_res;
	hdr->selected_range = data->selected_range;
	hdr->accel_odr = data->odr;
	hdr->sample_set_size = ADXL313_FIFO_SAMPLE_SIZE;

	hdr->fifo_byte_count = fifo_byte_count;
}

static int adxl313_rtio_init_buffer(struct adxl313_dev_data *data, uint8_t **read_buf,
				    uint16_t fifo_bytes, struct rtio_iodev_sqe *current_iodev_sqe)
{
	const size_t min_read_size = sizeof(struct adxl313_fifo_data) + ADXL313_FIFO_SAMPLE_SIZE;
	const size_t ideal_read_size = sizeof(struct adxl313_fifo_data) + fifo_bytes;
	uint8_t *buf;
	uint32_t buf_avail, buf_length;

	if (rtio_sqe_rx_buf(current_iodev_sqe, min_read_size, ideal_read_size, &buf, &buf_length)) {
		LOG_ERR("Failed to get buffer");
		return -EINVAL;
	}
	buf_avail = buf_length - sizeof(struct adxl313_fifo_data);

	uint32_t read_len = MIN(fifo_bytes, buf_avail);

	if (buf_avail < fifo_bytes) {
		uint32_t pkts = read_len / ADXL313_FIFO_SAMPLE_SIZE;

		read_len = pkts * ADXL313_FIFO_SAMPLE_SIZE;
	}

	__ASSERT_NO_MSG(read_len % ADXL313_FIFO_SAMPLE_SIZE == 0);
	adxl313_rtio_init_hdr((struct adxl313_fifo_data *)buf, data, read_len);

	*read_buf = buf + sizeof(struct adxl313_fifo_data);

	return 0;
}

static int adxl313_rtio_cqe_consume(struct adxl313_dev_data *data)
{
	struct rtio_cqe *cqe = rtio_cqe_consume(data->rtio_ctx);
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

	return res;
}

static int adxl313_get_sensor_read_config(struct adxl313_dev_data *data,
					  struct sensor_read_config **ptr_read_config)
{
	if (!data->iodev_sqe) {
		LOG_ERR("data->iodev_sqe was NULL");
		return false;
	}

	*ptr_read_config = (struct sensor_read_config *)data->iodev_sqe->sqe.iodev->data;

	/* Check read config is valid. */
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

static void adxl313_fifo_read_cb(struct rtio *rtio_ctx, const struct rtio_sqe *sqe, int result,
				 void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct adxl313_dev_data *data = dev->data;
	struct rtio_iodev_sqe *iodev_sqe = sqe->userdata;

	ARG_UNUSED(result);

	adxl313_sqe_done(dev, iodev_sqe, data->fifo_entries);
}

static void adxl313_process_fifo_samples_cb(struct rtio *r, const struct rtio_sqe *sqr, int result,
					    void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct adxl313_dev_data *data = (struct adxl313_dev_data *)dev->data;
	struct rtio_iodev_sqe *current_iodev_sqe = data->iodev_sqe;

	data->iodev_sqe = NULL;
	data->fifo_entries = FIELD_GET(ADXL313_FIFO_STATUS_ENTRIES_MSK, data->reg_fifo_status);

	int8_t fifo_entries = data->fifo_entries;
	uint16_t fifo_bytes = fifo_entries * ADXL313_FIFO_SAMPLE_SIZE;
	uint8_t *read_buf;

	ARG_UNUSED(result);

	if (!current_iodev_sqe) {
		LOG_ERR("No pending SQE");
		goto err;
	}

	if (adxl313_rtio_init_buffer(data, &read_buf, fifo_bytes, current_iodev_sqe)) {
		goto err;
	}

	/* Flush completions */
	if (adxl313_rtio_cqe_consume(data)) {
		goto err;
	}

	for (size_t i = 0; i < fifo_entries; i++) {
		data->fifo_entries--;
		if (adxl313_rtio_reg_read(dev, ADXL313_REG_DATA_XYZ_REGS,
					  (read_buf + i * ADXL313_FIFO_SAMPLE_SIZE),
					  ADXL313_FIFO_SAMPLE_SIZE, current_iodev_sqe,
					  (i == fifo_entries - 1) ? adxl313_fifo_read_cb : NULL)) {
			LOG_WRN("RTIO reading the XYZ regs failed");
			goto err;
		}
		ARG_UNUSED(rtio_cqe_consume(data->rtio_ctx));
	}

	return;
err:
	LOG_WRN("Failed.");
	adxl313_sqe_done(dev, current_iodev_sqe, -ENOMEM);
}

/**
 * @brief Flush FIFO contents and clear interrupt sources.
 *
 * This function drains all FIFO samples by issuing read transactions until the
 * FIFO is empty. Draining the FIFO clears both the FIFO status and the
 * INT_SOURCE register.
 *
 * In STREAM FIFO mode, FIFO entries must be consumed to clear interrupt sources.
 * In TRIGGER FIFO mode, the FIFO could alternatively be reset by switching the
 * FIFO mode to BYPASS and back to TRIGGERED; however, this does not clear
 * INT_SOURCE when the device operates in continuous (streaming) measurement
 * modes. Therefore, draining the FIFO is required to fully reset interrupt
 * status in all supported configurations.
 *
 * The sensor is temporarily placed into standby mode while the FIFO is flushed
 * and is returned to measurement mode before the function completes.
 *
 * @param dev Pointer to the device structure. NULL is not allowed.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int adxl313_flush_fifo_async(const struct device *dev)
{
	struct adxl313_dev_data *data = dev->data;
	struct rtio_iodev_sqe *current_iodev_sqe = data->iodev_sqe;
	int8_t fifo_entries = ADXL313_FIFO_MAX_SIZE;
	struct sensor_read_config *read_config = NULL;

	if (!adxl313_get_sensor_read_config(data, &read_config)) {
		LOG_WRN("adxl313_get_sensor_read_config() failed");
		return 0;
	}

	if (adxl313_rtio_cqe_consume(data)) {
		LOG_WRN("adxl313_rtio_cqe_consume() failed");
		goto err;
	}

	if (adxl313_set_measure_en(dev, false)) {
		LOG_WRN("adxl313_set_measure_en() failed");
		goto err;
	}

	const struct adxl313_dev_config *cfg = dev->config;
	uint8_t reg = ADXL313_REG_DATA_XYZ_REGS;
	uint8_t reg_addr = ADXL313_REG_READ_MULTIBYTE(reg);
	size_t buflen = ADXL313_FIFO_SAMPLE_SIZE;

	for (size_t i = 0; i < fifo_entries; i++) {
		uint8_t dummy[6];
		uint8_t *buf = dummy;
		struct rtio_sqe *write_sqe = rtio_sqe_acquire(data->rtio_ctx);

		if (!write_sqe) {
			LOG_WRN("write_sqe failed");
			goto err;
		}
		rtio_sqe_prep_tiny_write(write_sqe, data->iodev, RTIO_PRIO_NORM, &reg_addr,
					 sizeof(reg_addr), NULL);
		write_sqe->flags |= RTIO_SQE_TRANSACTION;

		struct rtio_sqe *read_sqe = rtio_sqe_acquire(data->rtio_ctx);

		if (!read_sqe) {
			LOG_WRN("read_sqe failed");
			goto err;
		}
		rtio_sqe_prep_read(read_sqe, data->iodev, RTIO_PRIO_NORM, buf, buflen,
				   current_iodev_sqe);

		if (cfg->bus_type == ADXL313_BUS_I2C) {
			read_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
		}
		rtio_submit(data->rtio_ctx, 2); /* cautiously keep blocking */
		ARG_UNUSED(rtio_cqe_consume(data->rtio_ctx));
	}

	if (adxl313_set_measure_en(dev, true)) {
		LOG_WRN("adxl313_set_measure_en() failed");
		goto err;
	}

	return 0;
err:
	LOG_WRN("Failed.");
	return -EINVAL;
}

static bool first = true;
/**
 * @brief Submit a stream request to ADXL313 accelerometer
 *
 * This function initializes and configures the ADXL313 for streaming data capture.
 * It sets up the necessary GPIOs, flushes the FIFO buffer (if it's the first submission),
 * and enables the watermark interrupt trigger.
 *
 * @param dev Pointer to device structure containing configuration and I2C interface. NULL is not
 *        allowed.
 * @param[in] iodev_sqe Pointer to RTIO stream queue entry for data collection. NULL is not allowed.
 *
 * @note The function assumes that SENSOR_TRIG_FIFO_WATERMARK is the only available
 *       sensor trigger. Other interrupts (overrun, data ready) are implicitly enabled
 *       according to the datasheet.
 */
void adxl313_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct adxl313_dev_data *data = (struct adxl313_dev_data *)dev->data;
	int rc;

	data->iodev_sqe = iodev_sqe;

	if (first) {
		/* Initialize measurement and start with flushed registers */
		adxl313_flush_fifo_async(dev);
		adxl313_set_gpios_en(dev, true);
		first = false;
	}

	/*
	 * The only available sensor trigger is SENSOR_TRIG_FIFO_WATERMARK.
	 *
	 * The following interrupt events don't need to be explicitly enabled
	 * at the ADXL313 (implicitely enabled according to the datasheet):
	 * - overrun / SENSOR_TRIG_FIFO_FULL
	 * - data ready / SENSOR_TRIG_DATA_READY
	 */
	rc = adxl313_reg_assign_bits(dev, ADXL313_REG_INT_ENABLE, ADXL313_INT_WATERMARK, true);
	if (rc) {
		LOG_WRN("adxl313_reg_assign_bits() failed");
		return;
	}
}

/**
 * @brief ADXL313 FIFO interrupt handler for streaming data collection
 *
 * This function handles interrupts generated by the ADXL313 accelerometer when new
 * samples are available in the FIFO buffer. It processes the FIFO status and reads
 * sample data using RTIO (Real-Time IO) operations.
 *
 * The function performs the following tasks:
 * - Verifies that a valid stream queue entry exists
 * - Checks if RTIO streaming is properly configured
 * - Consumes any completed CQEs (Completion Queue Entries)
 * - Reads FIFO status and samples from the ADXL313 using RTIO operations
 *
 * @param dev Pointer to device structure containing configuration and I2C interface. NULL is not
 *        allowed.
 */
void adxl313_stream_fifo_irq_handler(const struct device *dev)
{
	struct adxl313_dev_data *data = (struct adxl313_dev_data *)dev->data;
	struct rtio_iodev_sqe *current_iodev_sqe = data->iodev_sqe;

	if (!current_iodev_sqe) {
		return;
	}

	/* Interrupt and FIFO status processing */
	struct sensor_read_config *read_config = NULL;

	if (!adxl313_get_sensor_read_config(data, &read_config)) {
		LOG_WRN("Failed! RTIO not setup for streaming");
		return;
	}

	/* Flush completions, in case cancel out */
	if (adxl313_rtio_cqe_consume(data)) {
		LOG_WRN("CQE consume failed");
		goto err;
	}

	if (adxl313_rtio_reg_read(dev, ADXL313_REG_FIFO_STATUS, &data->reg_fifo_status,
				  sizeof(data->reg_fifo_status), current_iodev_sqe,
				  adxl313_process_fifo_samples_cb)) {
		LOG_WRN("Reading the FIFO samples failed");
		goto err;
	}

	return;
err:
	LOG_WRN("Failed.");
	adxl313_sqe_done(dev, current_iodev_sqe, -ENOMEM);
}
