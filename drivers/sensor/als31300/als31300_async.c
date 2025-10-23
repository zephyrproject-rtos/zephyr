/*
 * Copyright (c) 2025 Croxel
 * SPDX-License-Identifier: Apache-2.0
 */

#include "als31300.h"

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/logging/log.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/dsp/types.h>

LOG_MODULE_DECLARE(als31300, CONFIG_SENSOR_LOG_LEVEL);

/**
 * @brief Encode channel flags for the given sensor channel
 */
static uint8_t als31300_encode_channel(enum sensor_channel chan)
{
	uint8_t encode_bmask = 0;

	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
		encode_bmask |= BIT(0);
		break;
	case SENSOR_CHAN_MAGN_Y:
		encode_bmask |= BIT(1);
		break;
	case SENSOR_CHAN_MAGN_Z:
		encode_bmask |= BIT(2);
		break;
	case SENSOR_CHAN_MAGN_XYZ:
		encode_bmask |= BIT(0) | BIT(1) | BIT(2);
		break;
	case SENSOR_CHAN_AMBIENT_TEMP:
		encode_bmask |= BIT(3);
		break;
	case SENSOR_CHAN_ALL:
		encode_bmask |= BIT(0) | BIT(1) | BIT(2) | BIT(3);
		break;
	default:
		break;
	}

	return encode_bmask;
}

/**
 * @brief Prepare async I2C read operation
 */
int als31300_prep_i2c_read_async(const struct als31300_config *cfg, uint8_t reg, uint8_t *buf,
				 size_t size, struct rtio_sqe **out)
{
	struct rtio *ctx = cfg->bus.ctx;
	struct rtio_iodev *iodev = cfg->bus.iodev;
	struct rtio_sqe *write_reg_sqe = rtio_sqe_acquire(ctx);
	struct rtio_sqe *read_buf_sqe = rtio_sqe_acquire(ctx);

	if (!write_reg_sqe || !read_buf_sqe) {
		rtio_sqe_drop_all(ctx);
		return -ENOMEM;
	}

	rtio_sqe_prep_tiny_write(write_reg_sqe, iodev, RTIO_PRIO_NORM, &reg, 1, NULL);
	write_reg_sqe->flags |= RTIO_SQE_TRANSACTION;

	rtio_sqe_prep_read(read_buf_sqe, iodev, RTIO_PRIO_NORM, buf, size, NULL);
	read_buf_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;

	if (out) {
		*out = read_buf_sqe;
	}

	return 0;
}

/**
 * @brief Completion callback for async operations
 */
static void als31300_complete_result(struct rtio *ctx, const struct rtio_sqe *sqe, int result,
				     void *arg)
{
	ARG_UNUSED(result);
	struct rtio_iodev_sqe *iodev_sqe = (struct rtio_iodev_sqe *)sqe->userdata;
	struct rtio_cqe *cqe;
	int err = 0;

	do {
		cqe = rtio_cqe_consume(ctx);
		if (cqe != NULL) {
			err = cqe->result;
			rtio_cqe_release(ctx, cqe);
		}
	} while (cqe != NULL);

	if (err != 0) {
		rtio_iodev_sqe_err(iodev_sqe, err);
	} else {
		rtio_iodev_sqe_ok(iodev_sqe, 0);
	}
}

/**
 * @brief Encode sensor metadata for async API
 */
int als31300_encode(const struct device *dev, const struct sensor_read_config *read_config,
		    uint8_t trigger_status, uint8_t *buf)
{
	struct als31300_encoded_data *edata = (struct als31300_encoded_data *)buf;
	uint64_t cycles;
	int err;

	ARG_UNUSED(dev);

	edata->header.channels = 0;

	if (trigger_status) {
		/* For triggers, encode all channels */
		edata->header.channels |= als31300_encode_channel(SENSOR_CHAN_ALL);
	} else {
		/* For normal reads, encode requested channels */
		const struct sensor_chan_spec *const channels = read_config->channels;
		size_t num_channels = read_config->count;

		for (size_t i = 0; i < num_channels; i++) {
			edata->header.channels |= als31300_encode_channel(channels[i].chan_type);
		}
	}

	/* Get timestamp */
	err = sensor_clock_get_cycles(&cycles);
	if (err != 0) {
		return err;
	}

	edata->header.timestamp = sensor_clock_cycles_to_ns(cycles);

	return 0;
}

/**
 * @brief RTIO submit function using chained SQE approach
 */
static void als31300_submit_one_shot(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	uint32_t min_buf_len = sizeof(struct als31300_encoded_data);
	int err;
	uint8_t *buf;
	uint32_t buf_len;
	struct als31300_encoded_data *edata;
	const struct als31300_config *conf = dev->config;

	err = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	if (err < 0 || buf_len < min_buf_len || !buf) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buf_len);
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}

	edata = (struct als31300_encoded_data *)buf;

	err = als31300_encode(dev, cfg, 0, buf);
	if (err != 0) {
		LOG_ERR("Failed to encode sensor data");
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}

	struct rtio_sqe *read_sqe;

	err = als31300_prep_i2c_read_async(conf, ALS31300_REG_DATA_28, edata->payload,
					   sizeof(edata->payload), &read_sqe);
	if (err < 0) {
		LOG_ERR("Failed to prepare async read operation");
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}
	read_sqe->flags |= RTIO_SQE_CHAINED;

	struct rtio_sqe *complete_sqe = rtio_sqe_acquire(conf->bus.ctx);

	if (!complete_sqe) {
		LOG_ERR("Failed to acquire completion SQE");
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		rtio_sqe_drop_all(conf->bus.ctx);
		return;
	}

	rtio_sqe_prep_callback_no_cqe(complete_sqe, &als31300_complete_result, (void *)dev,
				      iodev_sqe);

	rtio_submit(conf->bus.ctx, 0);
}

/**
 * @brief RTIO submit function
 */
void als31300_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	als31300_submit_one_shot(dev, iodev_sqe);
}
