/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/sys/check.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/rtio/work.h>

#include "paa3905.h"
#include "paa3905_reg.h"
#include "paa3905_stream.h"
#include "paa3905_decoder.h"
#include "paa3905_bus.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(PAA3905_STREAM, CONFIG_SENSOR_LOG_LEVEL);

static void paa3905_chip_recovery_handler(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct device *dev = cfg->sensor;
	int err;

	err = paa3905_recover(dev);

	if (err) {
		rtio_iodev_sqe_err(iodev_sqe, err);
	} else {
		rtio_iodev_sqe_ok(iodev_sqe, 0);
	}
}

static void start_drdy_backup_timer(const struct device *dev)
{
	struct paa3905_data *data = dev->data;
	const struct paa3905_config *cfg = dev->config;

	k_timer_start(&data->stream.timer,
		      K_MSEC(cfg->backup_timer_period),
		      K_NO_WAIT);
}

static void paa3905_complete_result(struct rtio *ctx,
				    const struct rtio_sqe *sqe,
				    void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct paa3905_data *data = dev->data;
	struct rtio_iodev_sqe *iodev_sqe = data->stream.iodev_sqe;
	struct paa3905_encoded_data *edata = sqe->userdata;
	struct rtio_cqe *cqe;
	int err;

	edata->header.events.drdy = true &&
				    data->stream.settings.enabled.drdy;
	edata->header.events.motion = REG_MOTION_DETECTED(edata->motion) &&
				      data->stream.settings.enabled.motion;
	edata->header.channels = 0;

	if ((data->stream.settings.enabled.drdy &&
	      data->stream.settings.opt.drdy == SENSOR_STREAM_DATA_INCLUDE) ||
	    (data->stream.settings.enabled.motion &&
	      data->stream.settings.opt.motion == SENSOR_STREAM_DATA_INCLUDE)) {
		edata->header.channels |= paa3905_encode_channel(SENSOR_CHAN_POS_DXYZ);
	}

	if (data->stream.settings.enabled.drdy) {
		start_drdy_backup_timer(dev);
	}

	/** Attempt chip recovery if erratic behavior is detected  */
	if (!REG_OBSERVATION_CHIP_OK(edata->observation)) {

		LOG_WRN("CHIP OK register indicates issues. Attempting chip recovery");
		struct rtio_work_req *req = rtio_work_req_alloc();

		CHECKIF(!req) {
			LOG_ERR("Failed to allocate RTIO work request");
			rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
			return;
		}

		rtio_work_req_submit(req, iodev_sqe, paa3905_chip_recovery_handler);
	} else {
		rtio_iodev_sqe_ok(iodev_sqe, 0);
	}

	/* Flush RTIO bus CQEs */
	do {
		cqe = rtio_cqe_consume(ctx);
		if (cqe != NULL) {
			err = cqe->result;
			rtio_cqe_release(ctx, cqe);
		}
	} while (cqe != NULL);
}

static void paa3905_stream_get_data(const struct device *dev)
{
	struct paa3905_data *data = dev->data;
	uint64_t cycles;
	int err;

	CHECKIF(!data->stream.iodev_sqe) {
		LOG_WRN("No RTIO submission with an INT GPIO event");
		return;
	}

	struct paa3905_encoded_data *buf;
	uint32_t buf_len;
	uint32_t buf_len_required = sizeof(struct paa3905_encoded_data);

	err = rtio_sqe_rx_buf(data->stream.iodev_sqe,
			      buf_len_required,
			      buf_len_required,
			      (uint8_t **)&buf,
			      &buf_len);
	__ASSERT(err == 0, "Failed to acquire buffer (len: %d) for encoded data: %d. "
			   "Please revisit RTIO queue sizing and look for "
			   "bottlenecks during sensor data processing",
			   buf_len_required, err);

	/** Still throw an error even if asserts are off */
	if (err) {
		struct rtio_iodev_sqe *iodev_sqe = data->stream.iodev_sqe;

		LOG_ERR("Failed to acquire buffer for encoded data: %d", err);

		data->stream.iodev_sqe = NULL;
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}

	struct rtio_sqe *write_sqe = rtio_sqe_acquire(data->rtio.ctx);
	struct rtio_sqe *read_sqe = rtio_sqe_acquire(data->rtio.ctx);
	struct rtio_sqe *cb_sqe = rtio_sqe_acquire(data->rtio.ctx);
	uint8_t val;

	err = sensor_clock_get_cycles(&cycles);
	CHECKIF(err) {
		struct rtio_iodev_sqe *iodev_sqe = data->stream.iodev_sqe;

		LOG_ERR("Failed to get timestamp: %d", err);

		data->stream.iodev_sqe = NULL;
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}
	buf->header.timestamp = sensor_clock_cycles_to_ns(cycles);

	CHECKIF(!write_sqe || !read_sqe || !cb_sqe) {
		LOG_ERR("Failed to acquire RTIO SQE's");
		rtio_iodev_sqe_err(data->stream.iodev_sqe, -ENOMEM);
		return;
	}

	val = REG_BURST_READ | REG_SPI_READ_BIT;

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
			   buf->buf,
			   sizeof(buf->buf),
			   NULL);
	read_sqe->flags |= RTIO_SQE_CHAINED;

	rtio_sqe_prep_callback_no_cqe(cb_sqe,
				      paa3905_complete_result,
				      (void *)dev,
				      buf);

	rtio_submit(data->rtio.ctx, 0);
}

static void paa3905_gpio_callback(const struct device *gpio_dev,
				  struct gpio_callback *cb,
				  uint32_t pins)
{
	struct paa3905_stream *stream = CONTAINER_OF(cb,
						     struct paa3905_stream,
						     cb);
	const struct device *dev = stream->dev;
	const struct paa3905_config *cfg = dev->config;
	int err;

	/* Disable interrupts */
	err = gpio_pin_interrupt_configure_dt(&cfg->int_gpio,
					      GPIO_INT_MODE_DISABLED);
	if (err) {
		LOG_ERR("Failed to disable interrupt");
		return;
	}

	paa3905_stream_get_data(dev);
}

static void paa3905_stream_drdy_timeout(struct k_timer *timer)
{
	struct paa3905_stream *stream = CONTAINER_OF(timer,
						     struct paa3905_stream,
						     timer);
	const struct device *dev = stream->dev;
	const struct paa3905_config *cfg = dev->config;
	int err;

	/* Disable interrupts */
	err = gpio_pin_interrupt_configure_dt(&cfg->int_gpio,
					      GPIO_INT_MODE_DISABLED);
	if (err) {
		LOG_ERR("Failed to disable interrupt");
		return;
	}

	paa3905_stream_get_data(dev);
}

static inline bool settings_changed(const struct paa3905_stream *a,
				    const struct paa3905_stream *b)
{
	return (a->settings.enabled.drdy != b->settings.enabled.drdy) ||
	       (a->settings.opt.drdy != b->settings.opt.drdy) ||
	       (a->settings.enabled.motion != b->settings.enabled.motion) ||
	       (a->settings.opt.motion != b->settings.opt.motion);
}

void paa3905_stream_submit(const struct device *dev,
			   struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *read_config = iodev_sqe->sqe.iodev->data;
	struct paa3905_data *data = dev->data;
	const struct paa3905_config *cfg = dev->config;
	int err;

	/** This separate struct is required due to the streaming API using a
	 * multi-shot RTIO submission: meaning, re-submitting itself after
	 * completion; hence, we don't have context to determine if this was
	 * the first submission that kicked things off. We're then, inferring
	 * this by comparing if the read-config has changed, and only restart
	 * in such case.
	 */
	struct paa3905_stream stream = {0};

	for (size_t i = 0 ; i < read_config->count ; i++) {
		switch (read_config->channels[i].chan_type) {
		case SENSOR_TRIG_DATA_READY:
			stream.settings.enabled.drdy = true;
			stream.settings.opt.drdy = read_config->triggers[i].opt;
			break;
		case SENSOR_TRIG_MOTION:
			stream.settings.enabled.motion = true;
			stream.settings.opt.motion = read_config->triggers[i].opt;
			break;
		default:
			LOG_ERR("Unsupported trigger (%d)", read_config->triggers[i].trigger);
			rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
			return;
		}
	}

	/* Store context for next submission (handled within callbacks) */
	data->stream.iodev_sqe = iodev_sqe;

	if (settings_changed(&data->stream, &stream)) {
		uint8_t motion_data[6];

		data->stream.settings = stream.settings;

		/* Disable interrupts */
		err = gpio_pin_interrupt_configure_dt(&cfg->int_gpio,
						      GPIO_INT_MODE_DISABLED);
		if (err) {
			LOG_ERR("Failed to disable interrupt");
			rtio_iodev_sqe_err(iodev_sqe, err);
			return;
		}

		/* Read reg's 0x02-0x06 to clear motion data. */
		err = paa3905_bus_read(dev, REG_MOTION, motion_data, sizeof(motion_data));
		if (err) {
			LOG_ERR("Failed to read motion data");
			rtio_iodev_sqe_err(iodev_sqe, err);
			return;
		}
	}

	/* Re-enable interrupts */
	err = gpio_pin_interrupt_configure_dt(&cfg->int_gpio,
						GPIO_INT_LEVEL_ACTIVE);
	if (err) {
		LOG_ERR("Failed to enable interrupt");
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}

	/** Back-up timer allows us to keep checking in with the sensor in
	 * spite of not having any motion. This results in allowing sensor
	 * recovery if falling in erratic state.
	 */
	if (data->stream.settings.enabled.drdy) {
		start_drdy_backup_timer(dev);
	}
}

int paa3905_stream_init(const struct device *dev)
{
	const struct paa3905_config *cfg = dev->config;
	struct paa3905_data *data = dev->data;
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
			   paa3905_gpio_callback,
			   BIT(cfg->int_gpio.pin));

	err = gpio_add_callback(cfg->int_gpio.port, &data->stream.cb);
	if (err) {
		LOG_ERR("Failed to add interrupt callback");
		return -EIO;
	}

	k_timer_init(&data->stream.timer, paa3905_stream_drdy_timeout, NULL);

	return err;
}
