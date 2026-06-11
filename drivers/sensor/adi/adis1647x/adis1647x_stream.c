/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/drivers/gpio.h>

#include "adis1647x.h"

LOG_MODULE_DECLARE(adis1647x, CONFIG_SENSOR_LOG_LEVEL);

void adis1647x_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg =
		(const struct sensor_read_config *)iodev_sqe->sqe.iodev->data;
	struct adis1647x_data *data = (struct adis1647x_data *)dev->data;
	const struct adis1647x_config *cfg_adis1647x = dev->config;

	bool drdy = false;

	for (size_t i = 0; i < cfg->count; i++) {
		if (cfg->triggers[i].trigger == SENSOR_TRIG_DATA_READY) {
			drdy = true;
		}
	}

	if (!drdy) {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
		return;
	}

	int rc = gpio_pin_interrupt_configure_dt(&cfg_adis1647x->interrupt, GPIO_INT_DISABLE);

	if (rc < 0) {
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	data->sqe = iodev_sqe;

	rc = gpio_pin_interrupt_configure_dt(&cfg_adis1647x->interrupt, GPIO_INT_EDGE_TO_ACTIVE);
	if (rc < 0) {
		data->sqe = NULL;
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}
}

void adis1647x_stream_irq_handler(const struct device *dev)
{
	struct adis1647x_data *data = (struct adis1647x_data *)dev->data;
	const struct adis1647x_config *cfg = dev->config;

	uint64_t cycles;
	int rc;

	if (data->sqe == NULL) {
		return;
	}

	rc = sensor_clock_get_cycles(&cycles);
	if (rc != 0) {
		LOG_ERR("Failed to get sensor clock cycles");
		rtio_iodev_sqe_err(data->sqe, rc);
		return;
	}

	data->timestamp = sensor_clock_cycles_to_ns(cycles);

	struct rtio_iodev_sqe *current_sqe = data->sqe;

	uint8_t *buf;
	uint32_t buf_len;

	if (rtio_sqe_rx_buf(current_sqe, sizeof(struct adis1647x_sample_data),
			    sizeof(struct adis1647x_sample_data), &buf, &buf_len) != 0) {
		LOG_ERR("Failed to acquire buffer");
		data->sqe = NULL;
		rtio_iodev_sqe_err(current_sqe, -ENOMEM);
		goto reenable;
	}

	struct adis1647x_sample_data *sample = (struct adis1647x_sample_data *)buf;

	rc = adis1647x_get_data(dev, sample);
	data->sqe = NULL;
	if (rc != 0) {
		rtio_iodev_sqe_err(current_sqe, rc);
	} else {
		sample->timestamp = data->timestamp;
		rtio_iodev_sqe_ok(current_sqe, 0);
	}

reenable:
	gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_EDGE_TO_ACTIVE);
}
