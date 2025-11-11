/*
 * Copyright (c) 2025 Croxel, Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/check.h>
#include "rm3100_stream.h"
#include "rm3100_decoder.h"
#include "rm3100.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(RM3100_STREAM, CONFIG_SENSOR_LOG_LEVEL);

static void rm3100_complete_result(struct rtio *ctx, const struct rtio_sqe *sqe, int err,
				   void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct rm3100_data *data = dev->data;
	struct rtio_iodev_sqe *iodev_sqe = data->stream.iodev_sqe;
	struct rtio_cqe *cqe;
	struct rm3100_encoded_data *edata = sqe->userdata;

	edata->header.events.drdy = ((edata->header.status != 0) &&
				     data->stream.settings.enabled.drdy);
	edata->header.channels = 0;

	if (!edata->header.events.drdy) {
		LOG_ERR("Status register does not have DRDY bit set: 0x%02x",
			edata->header.status);
	} else if (data->stream.settings.opt.drdy == SENSOR_STREAM_DATA_INCLUDE) {
		edata->header.channels |= rm3100_encode_channel(SENSOR_CHAN_MAGN_XYZ);
	}

	do {
		cqe = rtio_cqe_consume(ctx);
		if (cqe != NULL) {
			if (err >= 0) {
				err = cqe->result;
			}
			rtio_cqe_release(ctx, cqe);
		}
	} while (cqe != NULL);

	if (err) {
		rtio_iodev_sqe_err(iodev_sqe, err);
	} else {
		rtio_iodev_sqe_ok(iodev_sqe, 0);
	}

	LOG_DBG("Streaming read-out complete");
}

static void rm3100_stream_get_data(const struct device *dev)
{
	struct rm3100_data *data = dev->data;
	uint64_t cycles;
	int err;

	CHECKIF(!data->stream.iodev_sqe) {
		LOG_WRN("No RTIO submission with an INT GPIO event");
		return;
	}

	struct rtio_iodev_sqe *iodev_sqe = data->stream.iodev_sqe;
	uint8_t *buf;
	uint32_t buf_len;
	uint32_t min_buf_len = sizeof(struct rm3100_encoded_data);
	struct rm3100_encoded_data *edata;

	err = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	if (err) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buf_len);

		data->stream.iodev_sqe = NULL;
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}

	edata = (struct rm3100_encoded_data *)buf;

	err = sensor_clock_get_cycles(&cycles);
	CHECKIF(err) {
		LOG_ERR("Failed to get timestamp: %d", err);

		data->stream.iodev_sqe = NULL;
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}
	edata->header.timestamp = sensor_clock_cycles_to_ns(cycles);

	struct rtio_sqe *status_wr_sqe = rtio_sqe_acquire(data->rtio.ctx);
	struct rtio_sqe *status_rd_sqe = rtio_sqe_acquire(data->rtio.ctx);
	struct rtio_sqe *write_sqe = rtio_sqe_acquire(data->rtio.ctx);
	struct rtio_sqe *read_sqe = rtio_sqe_acquire(data->rtio.ctx);
	struct rtio_sqe *complete_sqe = rtio_sqe_acquire(data->rtio.ctx);

	if (!write_sqe || !read_sqe || !complete_sqe) {
		LOG_ERR("Failed to acquire RTIO SQEs");
		rtio_sqe_drop_all(data->rtio.ctx);

		data->stream.iodev_sqe = NULL;
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	uint8_t val;

	val = RM3100_REG_STATUS | REG_READ_BIT;

	rtio_sqe_prep_tiny_write(status_wr_sqe,
				 data->rtio.iodev,
				 RTIO_PRIO_HIGH,
				 &val,
				 1,
				NULL);
	status_wr_sqe->flags |= RTIO_SQE_TRANSACTION;

	rtio_sqe_prep_read(status_rd_sqe,
			   data->rtio.iodev,
			   RTIO_PRIO_HIGH,
			   &edata->header.status,
			   sizeof(edata->header.status),
			   NULL);

	if (rtio_is_i2c(data->rtio.type)) {
		status_rd_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
	}
	status_rd_sqe->flags |= RTIO_SQE_CHAINED;

	val = RM3100_REG_MX | REG_READ_BIT;

	rtio_sqe_prep_tiny_write(write_sqe,
				 data->rtio.iodev,
				 RTIO_PRIO_HIGH,
				 &val,
				 1,
				NULL);
	write_sqe->flags |= RTIO_SQE_TRANSACTION;

	rtio_sqe_prep_read(read_sqe,
			   data->rtio.iodev,
			   RTIO_PRIO_HIGH,
			   edata->payload,
			   sizeof(edata->payload),
			   NULL);
	if (rtio_is_i2c(data->rtio.type)) {
		read_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
	}
	read_sqe->flags |= RTIO_SQE_CHAINED;

	rtio_sqe_prep_callback_no_cqe(complete_sqe,
				      rm3100_complete_result,
				      (void *)dev,
				      buf);

	rtio_submit(data->rtio.ctx, 0);
}

static void rm3100_gpio_callback(const struct device *gpio_dev,
				  struct gpio_callback *cb,
				  uint32_t pins)
{
	struct rm3100_stream *stream = CONTAINER_OF(cb,
						    struct rm3100_stream,
						    cb);
	const struct device *dev = stream->dev;
	const struct rm3100_config *cfg = dev->config;
	int err;

	/* Disable interrupts */
	err = gpio_pin_interrupt_configure_dt(&cfg->int_gpio,
					      GPIO_INT_MODE_DISABLED);
	if (err) {
		LOG_ERR("Failed to disable interrupt");
		return;
	}

	rm3100_stream_get_data(dev);
}

void rm3100_stream_submit(const struct device *dev,
			  struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *read_config = iodev_sqe->sqe.iodev->data;
	struct rm3100_data *data = dev->data;
	const struct rm3100_config *cfg = dev->config;
	int err;

	if ((read_config->count != 1) ||
	    (read_config->triggers[0].trigger != SENSOR_TRIG_DATA_READY)) {
		LOG_ERR("Only SENSOR_TRIG_DATA_READY is supported");
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
		return;
	}

	/* Store context for next submission (handled within callbacks) */
	data->stream.iodev_sqe = iodev_sqe;

	data->stream.settings.enabled.drdy = true;
	data->stream.settings.opt.drdy = read_config->triggers[0].opt;

	err = gpio_pin_interrupt_configure_dt(&cfg->int_gpio,
						GPIO_INT_LEVEL_ACTIVE);
	if (err) {
		LOG_ERR("Failed to enable interrupts");

		data->stream.iodev_sqe = NULL;
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}
}

int rm3100_stream_init(const struct device *dev)
{
	const struct rm3100_config *cfg = dev->config;
	struct rm3100_data *data = dev->data;
	int err;

	/** Needed to get back the device handle from the callback context */
	data->stream.dev = dev;

	if (!cfg->int_gpio.port) {
		LOG_ERR("Interrupt GPIO not supplied");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cfg->int_gpio)) {
		LOG_ERR("Interrupt GPIO not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT);
	if (err) {
		LOG_ERR("Failed to configure interrupt GPIO");
		return -EIO;
	}

	gpio_init_callback(&data->stream.cb,
			   rm3100_gpio_callback,
			   BIT(cfg->int_gpio.pin));

	err = gpio_add_callback(cfg->int_gpio.port, &data->stream.cb);
	if (err) {
		LOG_ERR("Failed to add interrupt callback");
		return -EIO;
	}

	err = gpio_pin_interrupt_configure_dt(&cfg->int_gpio,
					      GPIO_INT_MODE_DISABLED);
	if (err) {
		LOG_ERR("Failed to configure interrupt as disabled");
		return -EIO;
	}

	return 0;
}
