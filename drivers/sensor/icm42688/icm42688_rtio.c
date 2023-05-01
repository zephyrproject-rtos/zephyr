/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include "icm42688.h"
#include "icm42688_decoder.h"
#include "icm42688_reg.h"
#include "icm42688_spi.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ICM42688_RTIO, CONFIG_SENSOR_LOG_LEVEL);

static inline uint32_t compute_min_buf_len(size_t num_channels, int num_output_samples)
{
	return sizeof(struct icm42688_encoded_data);
}

static int icm42688_rtio_sample_fetch(const struct device *dev, int16_t readings[7])
{
	uint8_t status;
	const struct icm42688_sensor_config *cfg = dev->config;
	uint8_t *buffer = (uint8_t *)readings;

	int res = icm42688_spi_read(&cfg->dev_cfg.spi, REG_INT_STATUS, &status, 1);

	if (res) {
		return res;
	}

	if (!FIELD_GET(BIT_INT_STATUS_DATA_RDY, status)) {
		return -EBUSY;
	}

	res = icm42688_read_all(dev, buffer);

	if (res) {
		return res;
	}

	for (int i = 0; i < 7; i++) {
		readings[i] = sys_le16_to_cpu((buffer[i * 2] << 8) | buffer[i * 2 + 1]);
	}

	return 0;
}

static int icm42688_submit_one_shot(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const enum sensor_channel *const channels = cfg->channels;
	const size_t num_channels = cfg->count;
	int num_output_samples = 7;
	uint32_t min_buf_len = compute_min_buf_len(num_channels, num_output_samples);
	int rc;
	uint8_t *buf;
	uint32_t buf_len;
	struct icm42688_encoded_data *edata;

	/* Get the buffer for the frame, it may be allocated dynamically by the rtio context */
	rc = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	if (rc != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buf_len);
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return rc;
	}

	edata = (struct icm42688_encoded_data *)buf;

	icm42688_encode(dev, channels, num_channels, buf);

	rc = icm42688_rtio_sample_fetch(dev, edata->readings);
	/* Check that the fetch succeeded */
	if (rc != 0) {
		LOG_ERR("Failed to fetch samples");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return rc;
	}

	rtio_iodev_sqe_ok(iodev_sqe, 0);

	return 0;
}

int icm42688_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct icm42688_sensor_data *data = dev->data;
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	int rc;

	if (!cfg->is_streaming) {
		return icm42688_submit_one_shot(dev, iodev_sqe);
	}

	/* Only support */
	struct icm42688_cfg new_config = data->cfg;

	new_config.interrupt1_drdy = false;
	new_config.interrupt1_fifo_ths = false;
	new_config.interrupt1_fifo_full = false;
	for (int i = 0; i < cfg->count; ++i) {
		switch (cfg->triggers[i]) {
		case SENSOR_TRIG_DATA_READY:
			new_config.interrupt1_drdy = true;
			break;
		case SENSOR_TRIG_FIFO_THRESHOLD:
			new_config.interrupt1_fifo_ths = true;
			break;
		case SENSOR_TRIG_FIFO_FULL:
			new_config.interrupt1_fifo_full = true;
			break;
		default:
			LOG_DBG("Trigger (%d) not supported", cfg->triggers[i]);
			break;
		}
	}

	if (new_config.interrupt1_drdy != data->cfg.interrupt1_drdy ||
	    new_config.interrupt1_fifo_ths != data->cfg.interrupt1_fifo_ths ||
	    new_config.interrupt1_fifo_full != data->cfg.interrupt1_fifo_full) {
		rc = icm42688_safely_configure(dev, &new_config);
		if (rc != 0) {
			LOG_ERR("Failed to configure sensor");
			return rc;
		}
	}

	data->streaming_sqe = iodev_sqe;
	return 0;
}

struct fifo_header {
	uint8_t int_status;
	uint16_t gyro_odr: 4;
	uint16_t accel_odr: 4;
	uint16_t gyro_fs: 3;
	uint16_t accel_fs: 3;
	uint16_t packet_format: 2;
} __attribute__((__packed__));

BUILD_ASSERT(sizeof(struct fifo_header) == 3);

static void icm42688_complete_cb(struct rtio *r, const struct rtio_sqe *sqe, void *arg)
{
	const struct device *dev = arg;
	struct icm42688_sensor_data *drv_data = dev->data;
	const struct icm42688_sensor_config *drv_cfg = dev->config;
	struct rtio_iodev_sqe *iodev_sqe = sqe->userdata;

	/* TODO report number of frames or bytes read here */
	rtio_iodev_sqe_ok(iodev_sqe, drv_data->fifo_count);

	gpio_pin_interrupt_configure_dt(&drv_cfg->dev_cfg.gpio_int1, GPIO_INT_EDGE_TO_ACTIVE);
}

static void icm42688_fifo_count_cb(struct rtio *r, const struct rtio_sqe *sqe, void *arg)
{
	const struct device *dev = arg;
	struct icm42688_sensor_data *drv_data = dev->data;
	const struct icm42688_sensor_config *drv_cfg = dev->config;
	struct rtio_iodev *spi_iodev = drv_data->spi_iodev;
	uint8_t *fifo_count_buf = (uint8_t *)&drv_data->fifo_count;
	uint16_t fifo_count = ((fifo_count_buf[0] << 8) | fifo_count_buf[1]);

	drv_data->fifo_count = fifo_count;

	/* Pull a operation from our device iodev queue, validated to only be reads */
	struct rtio_iodev_sqe *iodev_sqe = drv_data->streaming_sqe;

	drv_data->streaming_sqe = NULL;

	/* Not inherently an underrun/overrun as we may have a buffer to fill next time */
	if (iodev_sqe == NULL) {
		LOG_DBG("No pending SQE");
		gpio_pin_interrupt_configure_dt(&drv_cfg->dev_cfg.gpio_int1,
			 GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	const size_t packet_size = drv_data->cfg.fifo_hires ? 20 : 16;
	const size_t min_read_size = sizeof(struct fifo_header) + packet_size;
	const size_t ideal_read_size = sizeof(struct fifo_header) + fifo_count;
	uint8_t *buf;
	uint32_t buf_len;

	if (rtio_sqe_rx_buf(iodev_sqe, min_read_size, ideal_read_size, &buf, &buf_len) != 0) {
		LOG_ERR("Failed to get buffer");
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	/* Read FIFO and call back to rtio with rtio_sqe completion */
	/* TODO is packet format even needed? the fifo has a header per packet
	 * already
	 */
	struct fifo_header hdr = {
		.int_status = drv_data->int_status,
		.gyro_odr = drv_data->cfg.gyro_odr,
		.gyro_fs = drv_data->cfg.gyro_fs,
		.accel_odr = drv_data->cfg.accel_odr,
		.accel_fs = drv_data->cfg.accel_fs,
		.packet_format = 0,
	};
	uint32_t buf_avail = buf_len;

	memcpy(buf, &hdr, sizeof(hdr));
	buf_avail -= sizeof(hdr);

	uint32_t read_len = MIN(fifo_count, buf_avail);
	uint32_t pkts = read_len / packet_size;

	read_len = pkts * packet_size;

	__ASSERT_NO_MSG(read_len % pkt_size == 0);

	uint8_t *read_buf = buf + sizeof(hdr);

	/* Flush out completions  */
	struct rtio_cqe *cqe;

	do {
		cqe = rtio_cqe_consume(r);
		if (cqe != NULL) {
			rtio_cqe_release(r, cqe);
		}
	} while (cqe != NULL);

	/* Setup new rtio chain to read the fifo data and report then check the
	 * result
	 */
	struct rtio_sqe *write_fifo_addr = rtio_sqe_acquire(r);
	struct rtio_sqe *read_fifo_data = rtio_sqe_acquire(r);
	struct rtio_sqe *complete_op = rtio_sqe_acquire(r);
	const uint8_t reg_addr = REG_SPI_READ_BIT | FIELD_GET(REG_ADDRESS_MASK, REG_FIFO_DATA);

	rtio_sqe_prep_tiny_write(write_fifo_addr, spi_iodev,
				 RTIO_PRIO_NORM, &reg_addr, 1, NULL);
	write_fifo_addr->flags = RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_fifo_data, spi_iodev, RTIO_PRIO_NORM,
			   read_buf, read_len, iodev_sqe);

	rtio_sqe_prep_callback(complete_op, icm42688_complete_cb,
			       (void *)dev, iodev_sqe);

	rtio_submit(r, 0);
}

static void icm42688_int_status_cb(struct rtio *r, const struct rtio_sqe *sqr, void *arg)
{
	const struct device *dev = arg;
	struct icm42688_sensor_data *drv_data = dev->data;
	const struct icm42688_sensor_config *drv_cfg = dev->config;
	struct rtio_iodev *spi_iodev = drv_data->spi_iodev;

	if (FIELD_GET(BIT_INT_STATUS_FIFO_THS, drv_data->int_status) == 0 &&
	    FIELD_GET(BIT_INT_STATUS_FIFO_FULL, drv_data->int_status) == 0) {
		gpio_pin_interrupt_configure_dt(&drv_cfg->dev_cfg.gpio_int1,
						GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	/* Flush completions */
	struct rtio_cqe *cqe;

	do {
		cqe = rtio_cqe_consume(r);
		if (cqe != NULL) {
			rtio_cqe_release(r, cqe);
		}
	} while (cqe != NULL);

	struct rtio_sqe *write_fifo_count_reg = rtio_sqe_acquire(r);
	struct rtio_sqe *read_fifo_count = rtio_sqe_acquire(r);
	struct rtio_sqe *check_fifo_count = rtio_sqe_acquire(r);
	uint8_t reg = REG_SPI_READ_BIT | FIELD_GET(REG_ADDRESS_MASK, REG_FIFO_COUNTH);
	uint8_t *read_buf = (uint8_t *)&drv_data->fifo_count;

	rtio_sqe_prep_tiny_write(write_fifo_count_reg, spi_iodev, RTIO_PRIO_NORM, &reg, 1, NULL);
	write_fifo_count_reg->flags = RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_fifo_count, spi_iodev, RTIO_PRIO_NORM, read_buf, 2, NULL);
	rtio_sqe_prep_callback(check_fifo_count, icm42688_fifo_count_cb, arg, NULL);

	rtio_submit(r, 0);
}

void icm42688_fifo_event(const struct device *dev)
{
	struct icm42688_sensor_data *drv_data = dev->data;
	struct rtio_iodev *spi_iodev = drv_data->spi_iodev;
	struct rtio *r = drv_data->r;

	/*
	 * Setup rtio chain of ops with inline calls to make decisions
	 * 1. read int status
	 * 2. call to check int status and get pending RX operation
	 * 4. read fifo len
	 * 5. call to determine read len
	 * 6. read fifo
	 * 7. call to report completion
	 */
	struct rtio_sqe *write_int_reg = rtio_sqe_acquire(r);
	struct rtio_sqe *read_int_reg = rtio_sqe_acquire(r);
	struct rtio_sqe *check_int_status = rtio_sqe_acquire(r);
	uint8_t reg = REG_SPI_READ_BIT | FIELD_GET(REG_ADDRESS_MASK, REG_INT_STATUS);

	rtio_sqe_prep_tiny_write(write_int_reg, spi_iodev, RTIO_PRIO_NORM, &reg, 1, NULL);
	write_int_reg->flags = RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_int_reg, spi_iodev, RTIO_PRIO_NORM, &drv_data->int_status, 1, NULL);
	rtio_sqe_prep_callback(check_int_status, icm42688_int_status_cb, (void *)dev, NULL);
	rtio_submit(r, 0);
}
