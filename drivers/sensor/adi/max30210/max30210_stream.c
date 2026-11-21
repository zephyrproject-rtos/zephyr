/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "max30210.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_DECLARE(MAX30210, CONFIG_SENSOR_LOG_LEVEL);

static void max30210_stream_error(const struct device *dev, int error)
{
	struct max30210_data *data = dev->data;

	if (data->stream_sqe != NULL) {
		rtio_iodev_sqe_err(data->stream_sqe, error);
		data->stream_sqe = NULL;
	}

	int ret = max30210_reg_update(dev, INTERRUPT_ENABLE, FIFO_FULL_EN_MASK, 0);

	if (ret < 0) {
		LOG_ERR("Failed to disable FIFO interrupt after stream error: %d", ret);
	}
}

void max30210_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *read_config =
		(const struct sensor_read_config *)iodev_sqe->sqe.iodev->data;
	const struct max30210_config *config = dev->config;
	struct max30210_data *data = dev->data;
	bool has_fifo_trigger = false;
	bool has_watermark_trigger = false;
	int ret;

	if (!read_config->is_streaming) {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
		return;
	}

	if (!config->interrupt_gpio.port) {
		rtio_iodev_sqe_err(iodev_sqe, -ENODEV);
		return;
	}

	if (data->stream_sqe != NULL) {
		rtio_iodev_sqe_err(iodev_sqe, -EBUSY);
		return;
	}

	for (size_t i = 0; i < read_config->count; i++) {
		enum sensor_trigger_type trigger = read_config->triggers[i].trigger;
		enum sensor_stream_data_opt opt = read_config->triggers[i].opt;

		if (trigger != SENSOR_TRIG_FIFO_WATERMARK && trigger != SENSOR_TRIG_FIFO_FULL) {
			rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
			return;
		}
		if (trigger == SENSOR_TRIG_FIFO_WATERMARK) {
			has_watermark_trigger = true;
		}
		if (opt != SENSOR_STREAM_DATA_INCLUDE && opt != SENSOR_STREAM_DATA_NOP &&
		    opt != SENSOR_STREAM_DATA_DROP) {
			rtio_iodev_sqe_err(iodev_sqe, -EINVAL);
			return;
		}
		has_fifo_trigger = true;
	}

	if (!has_fifo_trigger) {
		rtio_iodev_sqe_err(iodev_sqe, -EINVAL);
		return;
	}

	uint8_t fifo_watermark = has_watermark_trigger ? config->fifo_watermark :
							 MAX30210_FIFO_DEPTH;

	ret = max30210_reg_update(dev, FIFO_CONFIG_1, FIFO_A_FULL_MASK,
				  MAX30210_FIFO_DEPTH - fifo_watermark);
	if (ret < 0) {
		LOG_ERR("Failed to configure FIFO watermark: %d", ret);
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}

	ret = max30210_reg_update(dev, FIFO_CONFIG_2, FLUSH_FIFO_MASK, 1);
	if (ret < 0) {
		LOG_ERR("Failed to flush FIFO before streaming: %d", ret);
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}

	uint8_t status;

	ret = max30210_reg_read(dev, STATUS, &status, 1);
	if (ret < 0) {
		LOG_ERR("Failed to clear FIFO status before streaming: %d", ret);
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}

	data->stream_sqe = iodev_sqe;

	ret = max30210_reg_update(dev, INTERRUPT_ENABLE, FIFO_FULL_EN_MASK, 1);
	if (ret < 0) {
		LOG_ERR("Failed to enable FIFO interrupt: %d", ret);
		max30210_stream_error(dev, ret);
		return;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to enable FIFO GPIO interrupt: %d", ret);
		max30210_stream_error(dev, ret);
	}
}

void max30210_stream_process(const struct device *dev, uint8_t status)
{
	struct max30210_data *data = dev->data;
	struct rtio_iodev_sqe *iodev_sqe = data->stream_sqe;
	const struct sensor_read_config *read_config;
	const struct sensor_stream_trigger *fifo_trigger = NULL;
	struct max30210_fifo_data *fifo_data;
	uint8_t fifo_count;
	uint8_t overflow_count;
	uint8_t *buf;
	uint32_t buf_len;
	size_t fifo_bytes = 0;
	int ret;

	if (iodev_sqe == NULL || (status & FIFO_FULL_MASK) == 0) {
		return;
	}

	if (rtio_iodev_sqe_is_canceled(iodev_sqe)) {
		data->stream_sqe = NULL;
		ret = max30210_reg_update(dev, INTERRUPT_ENABLE, FIFO_FULL_EN_MASK, 0);
		if (ret < 0) {
			LOG_ERR("Failed to disable FIFO interrupt after stream cancellation: %d", ret);
		}
		return;
	}

	ret = max30210_reg_read(dev, FIFO_COUNTER_1, &overflow_count, 1);
	if (ret < 0) {
		LOG_ERR("Failed to read FIFO overflow counter: %d", ret);
		max30210_stream_error(dev, ret);
		return;
	}

	ret = max30210_reg_read(dev, FIFO_COUNTER_2, &fifo_count, 1);
	if (ret < 0) {
		LOG_ERR("Failed to read FIFO data count: %d", ret);
		max30210_stream_error(dev, ret);
		return;
	}
	fifo_count = FIELD_GET(FIFO_DATA_COUNT_MASK, fifo_count);
	if (fifo_count == 0 || fifo_count > MAX30210_FIFO_DEPTH) {
		LOG_ERR("Invalid FIFO data count: %u", fifo_count);
		max30210_stream_error(dev, -EIO);
		return;
	}

	read_config = (const struct sensor_read_config *)iodev_sqe->sqe.iodev->data;
	for (size_t i = 0; i < read_config->count; i++) {
		if (read_config->triggers[i].trigger == SENSOR_TRIG_FIFO_WATERMARK) {
			fifo_trigger = &read_config->triggers[i];
			break;
		}
		if (fifo_trigger == NULL &&
		    read_config->triggers[i].trigger == SENSOR_TRIG_FIFO_FULL) {
			fifo_trigger = &read_config->triggers[i];
		}
	}

	if (fifo_trigger == NULL) {
		max30210_stream_error(dev, -EINVAL);
		return;
	}

	if (fifo_trigger->opt == SENSOR_STREAM_DATA_INCLUDE) {
		fifo_bytes = fifo_count * MAX30210_BYTES_PER_SAMPLE;
		ret = rtio_sqe_rx_buf(iodev_sqe, sizeof(*fifo_data) + fifo_bytes,
				      sizeof(*fifo_data) + fifo_bytes, &buf, &buf_len);
		if (ret != 0) {
			LOG_ERR("Failed to acquire FIFO stream buffer: %d", ret);
			max30210_stream_error(dev, ret);
			return;
		}
	} else {
		ret = rtio_sqe_rx_buf(iodev_sqe, sizeof(*fifo_data), sizeof(*fifo_data), &buf,
				      &buf_len);
		if (ret != 0) {
			LOG_ERR("Failed to acquire FIFO event buffer: %d", ret);
			max30210_stream_error(dev, ret);
			return;
		}

		if (fifo_trigger->opt == SENSOR_STREAM_DATA_DROP) {
			ret = max30210_reg_update(dev, FIFO_CONFIG_2, FLUSH_FIFO_MASK, 1);
			if (ret < 0) {
				LOG_ERR("Failed to discard FIFO data: %d", ret);
				max30210_stream_error(dev, ret);
				return;
			}
		}
	}

	fifo_data = (struct max30210_fifo_data *)buf;
	fifo_data->timestamp_ns = k_ticks_to_ns_floor64(k_uptime_ticks());
	fifo_data->sample_period_ns = UINT64_C(64000000000) >> data->sampling_rate;
	fifo_data->sample_count =
		(fifo_trigger->opt == SENSOR_STREAM_DATA_INCLUDE) ? fifo_count : 0;
	fifo_data->fifo_count = fifo_count;
	fifo_data->status = status;

	if (fifo_trigger->opt == SENSOR_STREAM_DATA_INCLUDE) {
		ret = max30210_reg_read(dev, FIFO_DATA, (uint8_t *)(fifo_data + 1),
					(uint8_t)fifo_bytes);
		if (ret < 0) {
			LOG_ERR("Failed to read FIFO data: %d", ret);
			max30210_stream_error(dev, ret);
			return;
		}
	}

	if (FIELD_GET(FIFO_OVERFLOW_COUNTER_MASK, overflow_count) != 0) {
		LOG_WRN("FIFO overflowed %u times",
			(unsigned int)
			FIELD_GET(FIFO_OVERFLOW_COUNTER_MASK, overflow_count));
	}

	data->stream_sqe = NULL;
	rtio_iodev_sqe_ok(iodev_sqe, 0);
}
