/*
 * Copyright (c) 2025 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/logging/log.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/mpsc_lockfree.h>

#include "bme680.h"

LOG_MODULE_DECLARE(bme680, CONFIG_SENSOR_LOG_LEVEL);

#if defined(CONFIG_BME680_HEATR_DUR_LP)
#define BME680_MEAS_WAIT_MS (197U + 25U)
#elif defined(CONFIG_BME680_HEATR_DUR_ULP)
#define BME680_MEAS_WAIT_MS (1943U + 25U)
#else
#define BME680_MEAS_WAIT_MS (222U)
#endif

static void bme680_start_transfer(struct rtio_iodev_sqe *iodev_sqe,
								  const struct device *dev);

/* Pop next queued request and start its transfer. */
static bool bme680_start_next(const struct device *dev)
{
	struct bme680_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->mpsc_lock);

	if (data->pending_sqe != NULL) {
		k_spin_unlock(&data->mpsc_lock, key);
		return false;
	}

	struct mpsc_node *node = mpsc_pop(&data->io_q);

	if (node == NULL) {
		k_spin_unlock(&data->mpsc_lock, key);
		return false;
	}

	struct rtio_iodev_sqe *next_sqe =
		CONTAINER_OF(node, struct rtio_iodev_sqe, q);

	data->pending_sqe = next_sqe;
	k_spin_unlock(&data->mpsc_lock, key);

	bme680_start_transfer(next_sqe, dev);
	return true;
}

/* Completion callback: compensate raw data, encode, start next */
static void bme680_read_complete_cb(struct rtio *r, const struct rtio_sqe *sqe,
									int res, void *arg)
{
	const struct device *dev = arg;
	struct bme680_data *data = dev->data;
	struct rtio_iodev_sqe *iodev_sqe = sqe->userdata;
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	struct bme680_reading reading;
	struct bme680_encoded_data *edata;
	uint8_t *buf;
	uint32_t buf_len;
	uint64_t cycles;
	k_spinlock_key_t key;
	bool fetch_temp = false;
	bool fetch_press = false;
	bool fetch_humidity = false;
	bool fetch_gas = false;
	int rc;

	ARG_UNUSED(r);

	if (res < 0) {
		LOG_ERR("RTIO read failed: %d", res);
		rtio_iodev_sqe_err(iodev_sqe, res);
		goto check_next;
	}


	bme680_compensate_raw(dev,
		(const struct bme_data_regs *)data->raw_buf,
		&reading);

	rc = rtio_sqe_rx_buf(iodev_sqe, sizeof(*edata), sizeof(*edata),
						 &buf, &buf_len);
	if (rc != 0) {
		LOG_ERR("rx buf alloc failed");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		goto check_next;
	}

	rc = sensor_clock_get_cycles(&cycles);
	if (rc != 0) {
		rtio_iodev_sqe_err(iodev_sqe, rc);
		goto check_next;
	}

	edata = (struct bme680_encoded_data *)buf;
	memset(edata, 0, sizeof(*edata));
	edata->header.timestamp = sensor_clock_cycles_to_ns(cycles);

	for (size_t i = 0; i < cfg->count; i++) {
		switch (cfg->channels[i].chan_type) {
		case SENSOR_CHAN_ALL:
			fetch_temp = fetch_press = fetch_humidity = fetch_gas = true;
			break;

		case SENSOR_CHAN_AMBIENT_TEMP:
				fetch_temp = true;
			break;

		case SENSOR_CHAN_PRESS:
				fetch_press = true;
			break;

		case SENSOR_CHAN_HUMIDITY:
				fetch_humidity = true;
			break;

		case SENSOR_CHAN_GAS_RES:
				fetch_gas = true;
			break;

		default:
			break;
		}
	}

	if (fetch_temp) {
		edata->has_temp = 1;
		edata->reading.comp_temp = reading.comp_temp;
	}
	if (fetch_press) {
		edata->has_press = 1;
		edata->reading.comp_press = reading.comp_press;
	}
	if (fetch_humidity) {
		edata->has_humidity = 1;
		edata->reading.comp_humidity = reading.comp_humidity;
	}
	if (fetch_gas) {
		edata->has_gas = 1;
		edata->reading.comp_gas = reading.comp_gas;
	}
	rtio_iodev_sqe_ok(iodev_sqe, 0);

check_next:
	{
		key = k_spin_lock(&data->mpsc_lock);
		data->pending_sqe = NULL;
		k_spin_unlock(&data->mpsc_lock, key);
	}

	bme680_start_next(dev);
}

/* Build and submit the full single-chain: trig → delay → addr → rd → cb
 * The delay uses RTIO_OP_DELAY (one-shot kernel timer) instead of k_work_delayable.
 */
static void bme680_start_transfer(struct rtio_iodev_sqe *iodev_sqe,
				  const struct device *dev)
{
	struct bme680_data *data = dev->data;
	const struct bme680_config *config = dev->config;
	struct rtio_sqe *trig, *delay, *addr, *rd, *cb;
	k_spinlock_key_t key;
	int rc;

	data->wr_buf[0] = config->is_spi
				  ? (BME680_REG_CTRL_MEAS & BME680_SPI_WRITE_MSK)
				  : BME680_REG_CTRL_MEAS;

	data->wr_buf[1] = BME680_CTRL_MEAS_VAL;

	data->rd_reg_buf[0] = config->is_spi
				  ? (BME680_REG_FIELD0 | BME680_SPI_READ_BIT)
				  : BME680_REG_FIELD0;

	trig = rtio_sqe_acquire(config->r);
	delay = rtio_sqe_acquire(config->r);
	addr = rtio_sqe_acquire(config->r);
	rd = rtio_sqe_acquire(config->r);
	cb = rtio_sqe_acquire(config->r);

	if (!trig || !delay || !addr || !rd || !cb) {
		rtio_sqe_drop_all(config->r);
		key = k_spin_lock(&data->mpsc_lock);
		data->pending_sqe = NULL;
		k_spin_unlock(&data->mpsc_lock, key);
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		bme680_start_next(dev);
		return;
	}

	/* SQE 1: Trigger measurement (write CTRL_MEAS register) */
	rtio_sqe_prep_tiny_write(trig, config->bus_iodev, RTIO_PRIO_NORM,
				 data->wr_buf, 2, NULL);
	if (!config->is_spi) {
		trig->iodev_flags = RTIO_IODEV_I2C_STOP;
	}
	trig->flags = RTIO_SQE_CHAINED;

	/* SQE 2: One-shot timer delay for measurement completion */
	rtio_sqe_prep_delay(delay, K_MSEC(BME680_MEAS_WAIT_MS), NULL);
	delay->flags = RTIO_SQE_CHAINED;

	/* SQE 3: Write FIELD0 register address (transaction start) */
	rtio_sqe_prep_tiny_write(addr, config->bus_iodev, RTIO_PRIO_NORM,
				 data->rd_reg_buf, 1, NULL);
	addr->flags = RTIO_SQE_TRANSACTION;

	/* SQE 4: Read FIELD0 raw data */
	rtio_sqe_prep_read(rd, config->bus_iodev, RTIO_PRIO_NORM,
			   data->raw_buf, BME680_FIELD0_DATA_LEN, NULL);
	if (!config->is_spi) {
		rd->iodev_flags = RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
	}
	rd->flags = RTIO_SQE_CHAINED;

	/* SQE 5: Completion callback */
	rtio_sqe_prep_callback_no_cqe(cb, bme680_read_complete_cb,
				     (void *)(uintptr_t)dev, iodev_sqe);

	rc = rtio_submit(config->r, 0);
	if (rc < 0) {
		key = k_spin_lock(&data->mpsc_lock);
		data->pending_sqe = NULL;
		k_spin_unlock(&data->mpsc_lock, key);
		rtio_iodev_sqe_err(iodev_sqe, rc);
		bme680_start_next(dev);
	}
}

void bme680_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct bme680_data *data = dev->data;
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;

	if (cfg->is_streaming) {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
		return;
	}

#if BME680_BUS_SPI
	const struct bme680_config *config = dev->config;

	/* The async RTIO path bypasses bme680_set_mem_page() in bme680_spi.c.
	 * It only accesses page-1 registers (CTRL_MEAS 0x74, FIELD0 0x1F),
	 * and bme680_power_up() leaves the device on page 1, so this should
	 * always hold unless the sync API switched pages concurrently.
	 */
	if (config->is_spi) {
		__ASSERT(data->mem_page == 1U,
			 "bme680 async: SPI mem_page must be 1");
	}
#endif
	for (size_t i = 0; i < cfg->count; i++) {
		if (cfg->channels[i].chan_idx != 0U) {
			rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
			return;
		}
	}

	/* Always push to queue first (any context can call this) */
	mpsc_push(&data->io_q, &iodev_sqe->q);

	/* Then try to start next with spinlock protection */
	bme680_start_next(dev);
}
