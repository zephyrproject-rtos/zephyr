/*
 * Copyright (c) 2025 Croxel, Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/check.h>

#define DT_DRV_COMPAT bosch_bmi08x_gyro
#include "bmi08x.h"
#include "bmi08x_bus.h"
#include "bmi08x_gyro_stream.h"
#include "bmi08x_gyro_decoder.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(BMI08X_GYRO_STREAM, CONFIG_SENSOR_LOG_LEVEL);

enum bmi08x_stream_state {
	BMI08X_STREAM_OFF,
	BMI08X_STREAM_ON,
	BMI08X_STREAM_BUSY,
};

static inline void bmi08x_stream_result(const struct device *dev, int result)
{
	struct bmi08x_gyro_data *data = dev->data;
	const struct bmi08x_gyro_config *config = dev->config;
	struct rtio_iodev_sqe *iodev_sqe = data->stream.iodev_sqe;

	data->stream.iodev_sqe = NULL;
	(void)rtio_flush_completion_queue(config->rtio_bus.ctx);
	if (result >= 0) {
		(void)atomic_set(&data->stream.state, BMI08X_STREAM_ON);
		if (iodev_sqe) {
			rtio_iodev_sqe_ok(iodev_sqe, 0);
		}
	} else {
		(void)atomic_set(&data->stream.state, BMI08X_STREAM_OFF);
		if (iodev_sqe) {
			rtio_iodev_sqe_err(iodev_sqe, result);
		}
	}
}

static void bmi08x_stream_complete_handler(struct rtio *ctx, const struct rtio_sqe *sqe, int err,
					   void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct bmi08x_gyro_data *data = dev->data;
	struct rtio_iodev_sqe *iodev_sqe = data->stream.iodev_sqe;
	struct bmi08x_gyro_encoded_data *edata;
	uint32_t buf_len;
	int ret;

	ret = rtio_sqe_rx_buf(iodev_sqe, 0, 0, (uint8_t **)&edata, &buf_len);
	if (ret < 0 || buf_len == 0 || edata->header.int_status == 0 ||
	    edata->header.fifo_status == 0) {
		err = -EIO;
	}

	bmi08x_stream_result(dev, err);
}

static inline void bmi08x_gyro_stream_evt_handler(const struct device *dev)
{
	struct bmi08x_gyro_data *data = dev->data;
	const struct bmi08x_gyro_config *config = dev->config;
	struct rtio_iodev_sqe *iodev_sqe = data->stream.iodev_sqe;
	int ret;
	struct bmi08x_gyro_encoded_data *edata;
	uint32_t buf_len;
	size_t readout_len = sizeof(struct bmi08x_gyro_frame) * data->stream.fifo_wm;
	size_t required_len = sizeof(struct bmi08x_gyro_encoded_data) + readout_len;
	struct rtio_sqe *out_sqe;

	CHECKIF(!iodev_sqe || atomic_get(&data->stream.state) == BMI08X_STREAM_OFF) {
		LOG_WRN("Event with Stream is Off. Disabling stream");
		bmi08x_stream_result(dev, -EIO);
		return;
	}
	CHECKIF(atomic_cas(&data->stream.state, BMI08X_STREAM_ON, BMI08X_STREAM_BUSY) == false) {
		LOG_DBG("Event while Stream is busy. Ignoring");
		return;
	}

	ret = rtio_sqe_rx_buf(iodev_sqe, required_len, required_len, (uint8_t **)&edata, &buf_len);
	CHECKIF(ret < 0 || buf_len < required_len) {
		LOG_ERR("Failed to obtain buffer. Err: %d, Req-len: %d", ret, required_len);
		bmi08x_stream_result(dev, -ENOMEM);
		return;
	}
	bmi08x_gyro_encode_header(dev, edata, true);

	struct reg_val_read {
		uint8_t reg;
		void *buf;
		size_t len;
	} streaming_readout[] = {
		{.reg = BMI08X_REG_GYRO_INT_STAT_1, .buf = &edata->header.int_status, .len = 1,},
		{.reg = BMI08X_REG_FIFO_STATUS, .buf = &edata->header.fifo_status, .len = 1},
		{.reg = BMI08X_REG_GYRO_FIFO_DATA, .buf = edata->fifo, .len = readout_len},
	};

	for (size_t i = 0 ; i < ARRAY_SIZE(streaming_readout) ; i++) {
		ret = bmi08x_prep_reg_read_rtio_async(&config->rtio_bus, streaming_readout[i].reg,
						      (uint8_t *)streaming_readout[i].buf,
						      streaming_readout[i].len, &out_sqe, false);
		CHECKIF(ret < 0 || !out_sqe) {
			bmi08x_stream_result(dev, -EIO);
			return;
		}
		out_sqe->flags |= RTIO_SQE_CHAINED;
	}
	out_sqe = rtio_sqe_acquire(config->rtio_bus.ctx);
	CHECKIF(!out_sqe) {
		rtio_sqe_drop_all(config->rtio_bus.ctx);
		bmi08x_stream_result(dev, -EIO);
		return;
	}
	rtio_sqe_prep_callback_no_cqe(out_sqe, bmi08x_stream_complete_handler, (void *)dev, NULL);
	(void)rtio_submit(config->rtio_bus.ctx, 0);
}

static void bmi08x_gyro_gpio_callback(const struct device *port, struct gpio_callback *cb,
				      uint32_t pin)
{
	struct bmi08x_gyro_data *data = CONTAINER_OF(cb, struct bmi08x_gyro_data, gpio_cb);
	const struct device *dev = data->dev;
	const struct bmi08x_accel_config *cfg = dev->config;

	(void)gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);
	(void)gpio_remove_callback(cfg->int_gpio.port, &data->gpio_cb);
	bmi08x_gyro_stream_evt_handler(dev);
}

static inline int start_stream(const struct device *dev)
{
	struct bmi08x_gyro_data *data = dev->data;
	const struct bmi08x_gyro_config *cfg = dev->config;
	struct rtio_sqe *out_sqe;
	int ret;
	struct reg_vals {
		uint8_t reg;
		uint8_t val;
	} stream_cfg_reg_writes[] = {
		{.reg = BMI08X_REG_GYRO_FIFO_CONFIG_1, .val = 0x80}, /* FIFO Stream mode */
		{.reg = BMI08X_REG_GYRO_FIFO_CONFIG_0, .val = data->stream.fifo_wm}, /* WM Level */
		{.reg = BMI08X_REG_GYRO_FIFO_WM_EN, .val = 0x88}, /* Enable FIFO WM */
		{.reg = BMI08X_REG_GYRO_INT3_INT4_IO_MAP, .val = BIT(2)}, /* FIFO INT to INT3 */
		{.reg = BMI08X_REG_GYRO_INT3_INT4_IO_CONF, .val = BIT(0)}, /* Push-pull act-high */
		{.reg = BMI08X_REG_GYRO_INT_CTRL, .val = BIT(6)}, /* Enable FIFO Interrupts */
	};

	for (size_t i = 0 ; i < ARRAY_SIZE(stream_cfg_reg_writes) ; i++) {
		ret = bmi08x_prep_reg_write_rtio_async(&cfg->rtio_bus, stream_cfg_reg_writes[i].reg,
						       &stream_cfg_reg_writes[i].val, 1, &out_sqe);
		CHECKIF(ret < 0 || !out_sqe) {
			return ret;
		}
		out_sqe->flags |= RTIO_SQE_CHAINED;
	}
	out_sqe->flags &= ~RTIO_SQE_CHAINED;

	/** We Synchronously write the stream since we want to do be done before enabling the
	 * interrupts. In the event that we're recovering from a failure, the interrupt line
	 * will be de-asserted.
	 */
	(void)rtio_submit(cfg->rtio_bus.ctx, ret);
	return 0;
}

void bmi08x_gyro_stream_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct bmi08x_gyro_data *data = dev->data;
	const struct bmi08x_gyro_config *cfg = dev->config;
	const struct sensor_read_config *stream_cfg = iodev_sqe->sqe.iodev->data;
	int ret = 0;

	if (stream_cfg->count != 1 ||
	    stream_cfg->triggers->trigger != SENSOR_TRIG_FIFO_WATERMARK ||
	    stream_cfg->triggers->opt != SENSOR_STREAM_DATA_INCLUDE) {
		LOG_ERR("Invalid stream configuration");
		rtio_iodev_sqe_err(iodev_sqe, -EINVAL);
		return;
	}
	data->stream.iodev_sqe = iodev_sqe;

	if (atomic_cas(&data->stream.state, BMI08X_STREAM_OFF, BMI08X_STREAM_ON) == true) {
		ret = start_stream(dev);
		if (ret != 0) {
			LOG_ERR("Failed to configure stream");
			bmi08x_stream_result(dev, ret);
			return;
		}
	}
	(void)gpio_add_callback(cfg->int_gpio.port, &data->gpio_cb);
	(void)gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_LEVEL_HIGH);
}

int bmi08x_gyro_stream_init(const struct device *dev)
{
	struct bmi08x_gyro_data *data = dev->data;
	const struct bmi08x_gyro_config *cfg = dev->config;
	int ret;

	if (!gpio_is_ready_dt(&cfg->int_gpio)) {
		LOG_ERR("GPIO device not ready: %p - dev: %p", &cfg->int_gpio, dev);
		return -ENODEV;
	}
	ret = gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure GPIO: %d", ret);
		return ret;
	}
	ret = gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);
	if (ret < 0) {
		LOG_ERR("Failed to configure GPIO: %d", ret);
		return ret;
	}
	gpio_init_callback(&data->gpio_cb, bmi08x_gyro_gpio_callback,  BIT(cfg->int_gpio.pin));
	data->dev = dev;

	return 0;
}
