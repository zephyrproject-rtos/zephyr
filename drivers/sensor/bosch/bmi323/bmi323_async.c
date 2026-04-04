/*
 * Copyright (c) 2025 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/logging/log.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/atomic.h>

#include "bmi323.h"

LOG_MODULE_DECLARE(bosch_bmi323, CONFIG_SENSOR_LOG_LEVEL);

/*
 * Async non-blocking read of all sensor data in a single chained transaction.
 *
 * raw_buffer layout:
 *   [0]              : register address (BMI323_DATA_REG)
 *   [1 .. off]       : bus dummy bytes  (1 for SPI, 2 for I2C / I3C)
 *   [1+off .. 14+off]: 14 bytes of sensor data (accel XYZ, gyro XYZ, temp)
 *
 * Worst-case allocation: 1 (addr) + 2 (max dummy) + 14 (data) = 17 bytes.
 */
#define BMI323_DATA_REG  IMU_BOSCH_BMI323_REG_ACC_DATA_X

#define BMI323_DATA_LEN  (7 * sizeof(uint16_t))

/*
 * bmi323_complete_cb - RTIO completion callback
 *
 * Decodes the raw bus buffer into a bmi323_encoded_data frame and signals
 * completion on the pending iodev_sqe.
 *
 * Runs in RTIO completion context — must not block.
 */
/* Forward declaration */
static void bmi323_complete_cb(struct rtio *r, const struct rtio_sqe *sqe,
							int res, void *arg);

/* Helper to start an RTIO transfer for a given iodev_sqe */
static void bmi323_start_transfer(struct rtio_iodev_sqe *iodev_sqe, struct bosch_bmi323_data *data)
{
	struct rtio_sqe *wr_data;
	struct rtio_sqe *rd_data;
	struct rtio_sqe *cb;
	uint8_t off = data->raw_data_offset;

	data->pending_sqe = iodev_sqe;

	/* Setup register address with read bit for SPI */
	if (data->async_bus_type == BMI323_BUS_SPI) {
		data->raw_buffer[0] = BMI323_DATA_REG | 0x80;
	} else {
		data->raw_buffer[0] = BMI323_DATA_REG;
	}

	wr_data = rtio_sqe_acquire(data->r);
	rd_data = rtio_sqe_acquire(data->r);
	cb = rtio_sqe_acquire(data->r);

	if ((wr_data == NULL) || (rd_data == NULL) || (cb == NULL)) {
		rtio_sqe_drop_all(data->r);
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		data->pending_sqe = NULL;
		return;
	}

	rtio_sqe_prep_tiny_write(wr_data, data->bus_iodev, RTIO_PRIO_NORM,
			 data->raw_buffer, 1, NULL);
	wr_data->flags = RTIO_SQE_TRANSACTION;

	rtio_sqe_prep_read(rd_data, data->bus_iodev, RTIO_PRIO_NORM,
			&data->raw_buffer[1], BMI323_DATA_LEN + off, NULL);
	rd_data->flags = RTIO_SQE_CHAINED;

	if (data->async_bus_type == BMI323_BUS_I2C) {
		rd_data->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
	} else if (data->async_bus_type == BMI323_BUS_I3C) {
		rd_data->iodev_flags |= RTIO_IODEV_I3C_STOP | RTIO_IODEV_I3C_RESTART;
	} else {
		/* SPI: no special flags needed */
	}

	rtio_sqe_prep_callback_no_cqe(cb, bmi323_complete_cb, (void *)(uintptr_t)data->dev,
			iodev_sqe);

	rtio_submit(data->r, 0);
}

/* Helper to try starting next transfer from queue with spinlock protection */
static bool bmi323_start_next(struct bosch_bmi323_data *data)
{
	k_spinlock_key_t key = k_spin_lock(&data->mpsc_lock);

	/* Already processing a transfer */
	if (data->pending_sqe != NULL) {
		k_spin_unlock(&data->mpsc_lock, key);
		return false;
	}

	/* Pop next request from queue */
	struct mpsc_node *node = mpsc_pop(&data->io_q);

	if (node == NULL) {
		/* Queue empty */
		k_spin_unlock(&data->mpsc_lock, key);
		return false;
	}

	struct rtio_iodev_sqe *next_sqe = CONTAINER_OF(node, struct rtio_iodev_sqe, q);

	k_spin_unlock(&data->mpsc_lock, key);

	/* Start the transfer (safe to do outside lock) */
	bmi323_start_transfer(next_sqe, data);
	return true;
}

static void bmi323_complete_cb(struct rtio *r, const struct rtio_sqe *sqe,
				   int res, void *arg)
{
	const struct device *dev = arg;
	struct bosch_bmi323_data *data = dev->data;
	struct rtio_iodev_sqe *iodev_sqe = sqe->userdata;
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	struct bmi323_encoded_data *edata;
	struct bmi323_reading reading;
	const uint8_t *data_raw;
	uint8_t *buf;
	uint32_t buf_len;
	uint64_t cycles;
	bool fetch_accel = false;
	bool fetch_gyro = false;
	bool fetch_temp = false;
	int rc;

	ARG_UNUSED(r);

	/* Check result from RTIO */
	if (res < 0) {
		rtio_iodev_sqe_err(iodev_sqe, res);
		goto check_next;
	}

	/* Sensor data starts after address byte + bus dummy bytes */
	data_raw = &data->raw_buffer[1U + data->raw_data_offset];

	for (size_t i = 0; i < cfg->count; i++) {
		switch (cfg->channels[i].chan_type) {
		case SENSOR_CHAN_ALL:
			fetch_accel = true;
			fetch_gyro  = true;
			fetch_temp  = true;
			break;
		case SENSOR_CHAN_ACCEL_X:
		case SENSOR_CHAN_ACCEL_Y:
		case SENSOR_CHAN_ACCEL_Z:
		case SENSOR_CHAN_ACCEL_XYZ:
			fetch_accel = true;
			break;
		case SENSOR_CHAN_GYRO_X:
		case SENSOR_CHAN_GYRO_Y:
		case SENSOR_CHAN_GYRO_Z:
		case SENSOR_CHAN_GYRO_XYZ:
			fetch_gyro = true;
			break;
		case SENSOR_CHAN_DIE_TEMP:
			fetch_temp = true;
			break;
		default:
			break;
		}
	}

	reading.accel_x    = (int16_t)sys_get_le16(&data_raw[0]);
	reading.accel_y    = (int16_t)sys_get_le16(&data_raw[2]);
	reading.accel_z    = (int16_t)sys_get_le16(&data_raw[4]);
	reading.gyro_x     = (int16_t)sys_get_le16(&data_raw[6]);
	reading.gyro_y     = (int16_t)sys_get_le16(&data_raw[8]);
	reading.gyro_z     = (int16_t)sys_get_le16(&data_raw[10]);
	reading.temperature = (int16_t)sys_get_le16(&data_raw[12]);

	rc = rtio_sqe_rx_buf(iodev_sqe, sizeof(*edata), sizeof(*edata),
			&buf, &buf_len);
	if (rc != 0) {
		LOG_ERR("Failed to get read buffer of size %u bytes",
			(uint32_t)sizeof(*edata));
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	rc = sensor_clock_get_cycles(&cycles);
	if (rc != 0) {
		LOG_ERR("Failed to get sensor clock cycles");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	edata = (struct bmi323_encoded_data *)buf;
	memset(edata, 0, sizeof(*edata));

	edata->header.timestamp = sensor_clock_cycles_to_ns(cycles);
	edata->has_accel        = fetch_accel;
	edata->has_gyro         = fetch_gyro;
	edata->has_temp         = fetch_temp;
	edata->acce_range		= (data->acc_full_scale / 1000);
	edata->gyro_range		= (data->gyro_full_scale / 1000);
	edata->reading          = reading;

	rtio_iodev_sqe_ok(iodev_sqe, 0);

check_next:
	/* Clear pending and try to start next */
	{
		k_spinlock_key_t key = k_spin_lock(&data->mpsc_lock);

		data->pending_sqe = NULL;

		k_spin_unlock(&data->mpsc_lock, key);
	}

	/* Try to start next transfer */
	bmi323_start_next(data);
}

void bmi323_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct bosch_bmi323_data *data = dev->data;
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;

	if (cfg->is_streaming) {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
		return;
	}

	for (size_t i = 0; i < cfg->count; i++) {
		if (cfg->channels[i].chan_idx != 0U) {
			rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
			return;
		}
	}

	/* Always push to queue first (any context can call this) */
	mpsc_push(&data->io_q, &iodev_sqe->q);

	/* Then try to start next with spinlock protection */
	bmi323_start_next(data);
}
