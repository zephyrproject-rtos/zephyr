/*
 * Copyright (c) 2026 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_fxas21002

#include <string.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/logging/log.h>
#include <zephyr/rtio/rtio.h>

#include "fxas21002.h"

LOG_MODULE_DECLARE(FXAS21002, CONFIG_SENSOR_LOG_LEVEL);

static void fxas21002_stream_complete_cb(struct rtio *r, const struct rtio_sqe *sqe,
					int res, void *arg)
{
	const struct device *dev = arg;
	struct fxas21002_data *data = dev->data;
	const struct fxas21002_config *config = dev->config;
	struct rtio_iodev_sqe *iodev_sqe = data->streaming_sqe;
	struct fxas21002_encoded_data *edata;
	uint8_t *buf;
	uint32_t buf_len;
	int rc;

	ARG_UNUSED(r);
	ARG_UNUSED(sqe);

	data->streaming_sqe = NULL;

	if (iodev_sqe == NULL) {
		goto rearm;
	}

	if (res != 0) {
		rtio_iodev_sqe_err(iodev_sqe, res);
		goto rearm;
	}

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	const int status_off = (config->inst_on_bus == FXAS21002_BUS_SPI) ? 1 : 0;
#else
	const int status_off = 0;
#endif

	rc = rtio_sqe_rx_buf(iodev_sqe, sizeof(*edata), sizeof(*edata), &buf, &buf_len);
	if (rc != 0) {
		rtio_iodev_sqe_err(iodev_sqe, rc);
		goto rearm;
	}

	edata = (struct fxas21002_encoded_data *)buf;
	edata->header.base_timestamp_ns = data->stream_timestamp;
	edata->header.reading_count = 1U;
	edata->status = data->stream_buffer[status_off];
	edata->range = config->range;
	edata->sample_count = 1;

	for (int i = 0; i < FXAS21002_MAX_NUM_CHANNELS; i++) {
		edata->raw[i] = ((int16_t)data->stream_buffer[status_off + 1 + i * 2] << 8) |
				data->stream_buffer[status_off + 2 + i * 2];
	}

	rtio_iodev_sqe_ok(iodev_sqe, 0);

rearm:
	gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}

void fxas21002_stream_irq_handler(const struct device *dev)
{
	struct fxas21002_data *data = dev->data;
	const struct fxas21002_config *config = dev->config;
	struct rtio_sqe *cb;
	uint64_t cycles;
	int rc;

	if (data->streaming_sqe == NULL) {
		gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}

	rc = sensor_clock_get_cycles(&cycles);
	if (rc != 0) {
		rtio_iodev_sqe_err(data->streaming_sqe, rc);
		data->streaming_sqe = NULL;
		gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		return;
	}
	data->stream_timestamp = sensor_clock_cycles_to_ns(cycles);

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	if (config->inst_on_bus == FXAS21002_BUS_I2C) {
		static const uint8_t reg_addr = FXAS21002_REG_STATUS;
		struct rtio_sqe *wr_sqe = rtio_sqe_acquire(config->r);
		struct rtio_sqe *rd_sqe = rtio_sqe_acquire(config->r);

		cb = rtio_sqe_acquire(config->r);
		if (!wr_sqe || !rd_sqe || !cb) {
			rtio_sqe_drop_all(config->r);
			goto enomem;
		}

		rtio_sqe_prep_tiny_write(wr_sqe, config->bus_iodev, RTIO_PRIO_NORM,
					 &reg_addr, 1, NULL);
		wr_sqe->flags = RTIO_SQE_TRANSACTION;

		rtio_sqe_prep_read(rd_sqe, config->bus_iodev, RTIO_PRIO_NORM,
				   data->stream_buffer, 7, NULL);
		rd_sqe->iodev_flags = RTIO_IODEV_I2C_RESTART | RTIO_IODEV_I2C_STOP;
		rd_sqe->flags = RTIO_SQE_CHAINED;

		rtio_sqe_prep_callback_no_cqe(cb, fxas21002_stream_complete_cb,
					      (void *)dev, NULL);
		rtio_submit(config->r, 0);
		return;
	}
#endif

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	if (config->inst_on_bus == FXAS21002_BUS_SPI) {
		struct rtio_sqe *io_sqe = rtio_sqe_acquire(config->r);

		cb = rtio_sqe_acquire(config->r);
		if (!io_sqe || !cb) {
			rtio_sqe_drop_all(config->r);
			goto enomem;
		}

		data->spi_tx_buf[0] = FXAS21002_REG_STATUS | BIT(7);
		memset(&data->spi_tx_buf[1], 0, 7);

		rtio_sqe_prep_transceive(io_sqe, config->bus_iodev, RTIO_PRIO_NORM,
					 data->spi_tx_buf, data->stream_buffer, 8, NULL);
		io_sqe->flags = RTIO_SQE_CHAINED;

		rtio_sqe_prep_callback_no_cqe(cb, fxas21002_stream_complete_cb,
					      (void *)dev, NULL);
		rtio_submit(config->r, 0);
		return;
	}
#endif

enomem:
	rtio_iodev_sqe_err(data->streaming_sqe, -ENOMEM);
	data->streaming_sqe = NULL;
	gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}

static int fxas21002_enable_drdy(const struct device *dev)
{
	const struct fxas21002_config *config = dev->config;
	struct fxas21002_data *data = dev->data;
	enum fxas21002_power power = FXAS21002_POWER_STANDBY;
	uint32_t transition_time;
	int ret;

	k_sem_take(&data->sem, K_FOREVER);

	/* CTRL_REG2 is writable in standby or ready, not in active. */
	ret = fxas21002_get_power(dev, &power);
	if (ret != 0) {
		goto out;
	}
	ret = fxas21002_set_power(dev, FXAS21002_POWER_READY);
	if (ret != 0) {
		goto out;
	}
	/* Set INT_EN_DRDY and leave the INT1/INT2 route bit unchanged. */
	ret = config->ops->reg_field_update(dev, FXAS21002_REG_CTRLREG2,
					    FXAS21002_CTRLREG2_CFG_EN_MASK,
					    FXAS21002_CTRLREG2_CFG_EN_MASK);
	if (ret != 0) {
		goto out;
	}
	ret = fxas21002_set_power(dev, power);
	if (ret != 0) {
		goto out;
	}
	transition_time = fxas21002_get_transition_time(FXAS21002_POWER_READY,
							power, config->dr);
	k_busy_wait(transition_time);
out:
	k_sem_give(&data->sem);
	return ret;
}

void fxas21002_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct fxas21002_config *config = dev->config;
	struct fxas21002_data *data = dev->data;
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	bool drdy_enabled = false;

	for (size_t i = 0; i < cfg->count; i++) {
		if (cfg->triggers[i].trigger == SENSOR_TRIG_DATA_READY) {
			drdy_enabled = true;
		}
	}

	if (data->streaming_sqe != NULL) {
		rtio_iodev_sqe_err(iodev_sqe, -EBUSY);
		return;
	}

	if (!drdy_enabled || !config->int_gpio.port) {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
		return;
	}

	if (!data->drdy_on) {
		if (fxas21002_enable_drdy(dev) != 0) {
			rtio_iodev_sqe_err(iodev_sqe, -EIO);
			return;
		}
		data->drdy_on = true;
	}

	gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_DISABLE);
	data->streaming_sqe = iodev_sqe;
	gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}
