/*
 * Copyright (c) 2025 sevenlab engineering GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sensirion_sen6x

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/drivers/sensor/sen6x.h>
#include <zephyr/net_buf.h>
#include <zephyr/pm/device.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/byteorder.h>

#include "sen6x.h"
#include "sen6x_reg.h"
#include "sen6x_decoder.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(SEN6X, CONFIG_SENSOR_LOG_LEVEL);

static int sen6x_prep_reg_read_rtio_async(const struct device *dev, uint16_t reg, uint8_t *buf,
					  size_t size, k_timeout_t delay, struct rtio_sqe **out)
{
	struct sen6x_data *data = dev->data;
	struct rtio *ctx = data->rtio_ctx;
	struct rtio_iodev *iodev = data->iodev;
	struct rtio_sqe *write_reg_sqe = rtio_sqe_acquire(ctx);
	struct rtio_sqe *delay_sqe = rtio_sqe_acquire(ctx);
	struct rtio_sqe *read_buf_sqe = rtio_sqe_acquire(ctx);
	uint8_t reg_be[2];

	if (!write_reg_sqe || !delay_sqe || !read_buf_sqe) {
		rtio_sqe_drop_all(ctx);
		return -ENOMEM;
	}

	sys_put_be16(reg, reg_be);

	rtio_sqe_prep_tiny_write(write_reg_sqe, iodev, RTIO_PRIO_NORM, reg_be, sizeof(reg_be),
				 NULL);
	write_reg_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP;
	write_reg_sqe->flags |= RTIO_SQE_CHAINED;

	rtio_sqe_prep_delay(delay_sqe, delay, NULL);
	delay_sqe->flags |= RTIO_SQE_CHAINED;

	rtio_sqe_prep_read(read_buf_sqe, iodev, RTIO_PRIO_NORM, buf, size, NULL);
	read_buf_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;

	/** Send back last SQE so it can be concatenated later. */
	if (out) {
		*out = read_buf_sqe;
	}

	return 3;
}

static int sen6x_prep_reg_send_rtio_async(const struct device *dev, uint16_t reg, k_timeout_t delay,
					  struct rtio_sqe **out)
{
	struct sen6x_data *data = dev->data;
	struct rtio *ctx = data->rtio_ctx;
	struct rtio_iodev *iodev = data->iodev;
	struct rtio_sqe *write_reg_sqe = rtio_sqe_acquire(ctx);
	struct rtio_sqe *delay_sqe = rtio_sqe_acquire(ctx);
	uint8_t reg_be[2];

	if (!write_reg_sqe || !delay_sqe) {
		rtio_sqe_drop_all(ctx);
		return -ENOMEM;
	}

	sys_put_be16(reg, reg_be);

	rtio_sqe_prep_tiny_write(write_reg_sqe, iodev, RTIO_PRIO_NORM, reg_be, sizeof(reg_be),
				 NULL);
	write_reg_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP;
	write_reg_sqe->flags |= RTIO_SQE_CHAINED;

	rtio_sqe_prep_delay(delay_sqe, delay, NULL);

	/** Send back last SQE so it can be concatenated later. */
	if (out) {
		*out = delay_sqe;
	}

	return 2;
}

static int sen6x_prep_reg_write_rtio_async(const struct device *dev, uint16_t reg,
					   const uint8_t *buf, size_t size, k_timeout_t delay,
					   struct rtio_sqe **out)
{
	struct sen6x_data *data = dev->data;
	struct rtio *ctx = data->rtio_ctx;
	struct rtio_iodev *iodev = data->iodev;
	struct rtio_sqe *write_reg_sqe = rtio_sqe_acquire(ctx);
	struct rtio_sqe *write_buf_sqe = rtio_sqe_acquire(ctx);
	struct rtio_sqe *delay_sqe = rtio_sqe_acquire(ctx);
	uint8_t reg_be[2];

	if (!write_reg_sqe || !write_buf_sqe || !delay_sqe) {
		rtio_sqe_drop_all(ctx);
		return -ENOMEM;
	}

	sys_put_be16(reg, reg_be);

	rtio_sqe_prep_tiny_write(write_reg_sqe, iodev, RTIO_PRIO_NORM, reg_be, sizeof(reg_be),
				 NULL);
	write_reg_sqe->flags |= RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_write(write_buf_sqe, iodev, RTIO_PRIO_NORM, buf, size, NULL);
	write_buf_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP;
	write_buf_sqe->flags |= RTIO_SQE_CHAINED;

	rtio_sqe_prep_delay(delay_sqe, delay, NULL);

	/** Send back last SQE so it can be concatenated later. */
	if (out) {
		*out = delay_sqe;
	}

	return 3;
}

static int sen6x_reg_read_rtio(const struct device *dev, uint16_t reg, uint8_t *buf, size_t size,
			       k_timeout_t delay)
{
	struct sen6x_data *data = dev->data;
	struct rtio *ctx = data->rtio_ctx;
	int ret;

	ret = sen6x_prep_reg_read_rtio_async(dev, reg, buf, size, delay, NULL);
	if (ret < 0) {
		return ret;
	}

	ret = rtio_submit(ctx, ret);
	if (ret != 0) {
		return ret;
	}

	return rtio_flush_completion_queue(ctx);
}

static int sen6x_reg_write_rtio(const struct device *dev, uint16_t reg, const uint8_t *buf,
				int size, k_timeout_t delay)
{
	struct sen6x_data *data = dev->data;
	struct rtio *ctx = data->rtio_ctx;
	int ret;

	if (buf) {
		ret = sen6x_prep_reg_write_rtio_async(dev, reg, buf, size, delay, NULL);
	} else {
		ret = sen6x_prep_reg_send_rtio_async(dev, reg, delay, NULL);
	}

	if (ret < 0) {
		return ret;
	}

	ret = rtio_submit(ctx, ret);
	if (ret != 0) {
		return ret;
	}

	return rtio_flush_completion_queue(ctx);
}

static inline bool sen6x_is_measuring(const struct device *dev)
{
	struct sen6x_data *data = dev->data;

	return atomic_get(&data->measurement_enabled) != 0;
}

static bool sen6x_is_co2_conditioning_running(const struct device *dev)
{
	const struct sen6x_config *cfg = dev->config;
	struct sen6x_data *data = dev->data;

	if (cfg->model != SEN6X_MODEL_SEN63C) {
		return false;
	}

	return data->co2_conditioning_started_time >= 0 &&
	       k_uptime_delta(&data->co2_conditioning_started_time) < 24000;
}

int sen6x_device_reset(const struct device *dev)
{
	const struct sen6x_config *cfg = dev->config;
	struct sen6x_data *data = dev->data;
	int ret;

	if (cfg->model == SEN6X_MODEL_SEN60) {
		ret = sen6x_reg_write_rtio(dev, REG_DEVICE_RESET_SEN60, NULL, 0, K_MSEC(1));
	} else {
		if (sen6x_is_measuring(dev)) {
			return -EPERM;
		}
		ret = sen6x_reg_write_rtio(dev, REG_DEVICE_RESET_SEN6X, NULL, 0, K_MSEC(1200));
	}

	if (ret != 0) {
		return ret;
	}

	atomic_set(&data->measurement_enabled, 0);
	data->measurement_state_changed_time = INT64_MIN;
	data->co2_conditioning_started_time = INT64_MIN;

	return 0;
}

int sen6x_start_continuous_measurement(const struct device *dev)
{
	const struct sen6x_config *cfg = dev->config;
	struct sen6x_data *data = dev->data;
	int ret;

	if (sen6x_is_measuring(dev)) {
		return -EALREADY;
	}
	if (data->measurement_state_changed_time >= 0 &&
	    k_uptime_delta(&data->measurement_state_changed_time) < 1000) {
		return -EAGAIN;
	}
	if (sen6x_is_co2_conditioning_running(dev)) {
		return -EAGAIN;
	}

	if (cfg->model == SEN6X_MODEL_SEN60) {
		ret = sen6x_reg_write_rtio(dev, REG_START_CONTINUOUS_MEASUREMENT_SEN60, NULL, 0,
					   K_MSEC(1));
	} else {
		ret = sen6x_reg_write_rtio(dev, REG_START_CONTINUOUS_MEASUREMENT_SEN6X, NULL, 0,
					   K_MSEC(50));
	}

	if (ret != 0) {
		return ret;
	}

	atomic_set(&data->measurement_enabled, 1);
	data->measurement_state_changed_time = k_uptime_get();
	data->co2_conditioning_started_time = k_uptime_get();

	return 0;
}

int sen6x_stop_continuous_measurement(const struct device *dev)
{
	const struct sen6x_config *cfg = dev->config;
	struct sen6x_data *data = dev->data;
	int ret;

	if (!sen6x_is_measuring(dev)) {
		return -EALREADY;
	}

	if (cfg->model == SEN6X_MODEL_SEN60) {
		ret = sen6x_reg_write_rtio(dev, REG_STOP_MEASUREMENT_SEN60, NULL, 0, K_MSEC(1000));
	} else {
		ret = sen6x_reg_write_rtio(dev, REG_STOP_MEASUREMENT_SEN6X, NULL, 0, K_MSEC(1000));
	}

	if (ret != 0) {
		return ret;
	}

	atomic_set(&data->measurement_enabled, 0);
	data->measurement_state_changed_time = k_uptime_get();

	return 0;
}

static int sen6x_prep_read_device_status(const struct device *dev, struct sen6x_encoded_data *edata,
					 struct rtio_sqe **out)
{

	const struct sen6x_config *cfg = dev->config;

	if (cfg->model == SEN6X_MODEL_SEN60) {
		return sen6x_prep_reg_read_rtio_async(dev, REG_READ_DEVICE_STATUS_SEN60,
						      edata->status, 3, K_MSEC(1), out);
	} else if (cfg->auto_clear_device_status) {
		return sen6x_prep_reg_read_rtio_async(dev, REG_READ_AND_CLEAR_DEVICE_STATUS_SEN6X,
						      edata->status, 6, K_MSEC(20), out);
	} else {
		return sen6x_prep_reg_read_rtio_async(dev, REG_READ_DEVICE_STATUS_SEN6X,
						      edata->status, 6, K_MSEC(20), out);
	}
}

static int sen6x_prep_read_data_ready(const struct device *dev, struct sen6x_encoded_data *edata,
				      struct rtio_sqe **out)
{

	const struct sen6x_config *cfg = dev->config;

	if (!sen6x_is_measuring(dev)) {
		return -EPERM;
	}

	if (cfg->model == SEN6X_MODEL_SEN60) {
		return sen6x_prep_reg_read_rtio_async(dev, REG_GET_DATA_READY_SEN60,
						      edata->data_ready, sizeof(edata->data_ready),
						      K_MSEC(1), out);
	} else {
		return sen6x_prep_reg_read_rtio_async(dev, REG_GET_DATA_READY_SEN6X,
						      edata->data_ready, sizeof(edata->data_ready),
						      K_MSEC(20), out);
	}
}

static int sen6x_prep_read_measured_values(const struct device *dev,
					   struct sen6x_encoded_data *edata, struct rtio_sqe **out)
{

	const struct sen6x_config *cfg = dev->config;

	if (!sen6x_is_measuring(dev)) {
		return -EPERM;
	}

	switch (cfg->model) {
	case SEN6X_MODEL_SEN60:
		return sen6x_prep_reg_read_rtio_async(dev, REG_READ_MEASURED_VALUES_SEN60,
						      edata->channels, 27, K_MSEC(1), out);
	case SEN6X_MODEL_SEN63C:
		return sen6x_prep_reg_read_rtio_async(dev, REG_READ_MEASURED_VALUES_SEN63C,
						      edata->channels, 21, K_MSEC(20), out);
	case SEN6X_MODEL_SEN65:
		return sen6x_prep_reg_read_rtio_async(dev, REG_READ_MEASURED_VALUES_SEN65,
						      edata->channels, 24, K_MSEC(20), out);
	case SEN6X_MODEL_SEN66:
		return sen6x_prep_reg_read_rtio_async(dev, REG_READ_MEASURED_VALUES_SEN66,
						      edata->channels, 27, K_MSEC(20), out);
	case SEN6X_MODEL_SEN68:
		return sen6x_prep_reg_read_rtio_async(dev, REG_READ_MEASURED_VALUES_SEN68,
						      edata->channels, 27, K_MSEC(20), out);
	default:
		return -ENOTSUP;
	}
}

bool sen6x_u16_array_checksum_ok(const void *data_, size_t data_len)
{
	const uint8_t *data = data_;

	if (data_len % 3) {
		return false;
	}

	for (size_t i = 0; i < data_len / 3; i++) {
		const uint8_t *chunk = &data[i * 3];
		uint8_t crc_calculated =
			crc8(chunk, 2, SEN6X_CRC_POLY, SEN6X_CRC_INIT, SEN6X_CRC_REV);

		if (crc_calculated != chunk[2]) {
			return false;
		}
	}

	return true;
}

static void sen6x_remove_checksums_from_rx_data(void *data_, size_t num_checksums)
{
	uint8_t *data = data_;
	size_t position = 0;

	for (size_t i = 0; i < num_checksums * 3; i += 3) {
		data[position++] = data[i + 0];
		data[position++] = data[i + 1];
	}
}

static void status_work_handler(struct k_work *work)
{
	struct sen6x_data *data = CONTAINER_OF(work, struct sen6x_data, status_work);
	const struct sen6x_config *cfg = data->dev->config;
	uint32_t status;

	if (cfg->model == SEN6X_MODEL_SEN60) {
		uint16_t status_sen60;

		if (!sen6x_u16_array_checksum_ok(data->status_buffer, 3)) {
			LOG_WRN("CRC check failed on status data.");
			return;
		}

		status = 0;

		/* Convert it to the new format to simplify the public API. */
		status_sen60 = sys_get_be16(data->status_buffer);
		if (status_sen60 & BIT(1)) {
			status |= SEN6X_STATUS_SPEED_WARNING;
		}
		if (status_sen60 & BIT(4)) {
			status |= SEN6X_STATUS_FAN_ERROR;
		}
	} else {
		if (!sen6x_u16_array_checksum_ok(data->status_buffer, 6)) {
			LOG_WRN("CRC check failed on status data.");
			return;
		}

		status = ((uint32_t)sys_get_be16(&data->status_buffer[0]) << 16) |
			 sys_get_be16(&data->status_buffer[3]);
	}

	if (data->status != status) {
		struct sen6x_callback *callback;
		struct sen6x_callback *tmp;

		data->status = status;

		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&data->callbacks, callback, tmp, node) {
			if (callback->status_changed != NULL) {
				callback->status_changed(data->dev, callback, data->status);
			}
		}
	}
}

static inline void sen6x_save_status_buffer(const struct device *dev,
					    const struct rtio_iodev_sqe *iodev_sqe)
{
	struct sen6x_data *data = dev->data;
	const struct rtio_sqe *sqe = &iodev_sqe->sqe;
	struct sen6x_encoded_data *edata;

	if (sqe->rx.buf == NULL) {
		return;
	}
	if (sqe->rx.buf_len < sizeof(struct sen6x_encoded_data)) {
		return;
	}

	edata = (void *)sqe->rx.buf;
	if (!edata->header.has_status) {
		return;
	}

	memcpy(data->status_buffer, edata->status, sizeof(data->status_buffer));
	k_work_submit(&data->status_work);
}

static inline bool sen6x_status_callbacks_exist(const struct device *dev)
{
	struct sen6x_data *data = dev->data;
	struct sen6x_callback *callback;

	SYS_SLIST_FOR_EACH_CONTAINER(&data->callbacks, callback, node) {
		if (callback->status_changed != NULL) {
			return true;
		}
	}

	return false;
}

static void sen6x_complete_result(struct rtio *ctx, const struct rtio_sqe *sqe, int result,
				  void *arg)
{
	ARG_UNUSED(result);
	const struct device *dev = arg;
	struct rtio_iodev_sqe *iodev_sqe = (struct rtio_iodev_sqe *)sqe->userdata;
	int ret = 0;

	ret = rtio_flush_completion_queue(ctx);
	if (ret != 0) {
		rtio_iodev_sqe_err(iodev_sqe, ret);
	} else {
		sen6x_save_status_buffer(dev, iodev_sqe);
		rtio_iodev_sqe_ok(iodev_sqe, 0);
	}

	LOG_DBG("One-shot fetch completed");
}

static inline void sen6x_submit_one_shot(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct sensor_chan_spec *const channels = cfg->channels;
	const size_t num_channels = cfg->count;
	uint32_t min_buf_len = sizeof(struct sen6x_encoded_data);
	int ret;
	uint8_t *buf;
	uint32_t buf_len;
	struct sen6x_encoded_data *edata;
	struct sen6x_data *data = dev->data;
	struct rtio_sqe *read_sqe;
	struct rtio_sqe *complete_sqe;

	ret = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	if (ret != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buf_len);
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}
	edata = (struct sen6x_encoded_data *)buf;

	ret = sen6x_encode(dev, channels, num_channels, buf);
	if (ret != 0) {
		LOG_ERR("Failed to encode sensor data");
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}

	/* The sensor has no interrupt line, so poll the status with every data read. */
	if (sen6x_status_callbacks_exist(dev)) {
		ret = sen6x_prep_read_device_status(dev, edata, &read_sqe);
		if (ret < 0) {
			LOG_ERR("Fail to prepare status read: %d", ret);
			rtio_iodev_sqe_err(iodev_sqe, ret);
			return;
		}
		read_sqe->flags |= RTIO_SQE_CHAINED;

		edata->header.has_status = true;
	}

	/* Reading no channels can be useful to update the status only. */
	if (edata->header.has_data_ready) {
		/* The chip doesn't have an interrupt line, but it might be useful to know if
		 * the data has actually changed. Reading the data resets this flag.
		 */
		ret = sen6x_prep_read_data_ready(dev, edata, &read_sqe);
		if (ret < 0) {
			LOG_ERR("Fail to prepare data-ready read: %d", ret);
			rtio_iodev_sqe_err(iodev_sqe, ret);
			return;
		}
		read_sqe->flags |= RTIO_SQE_CHAINED;
	}

	if (edata->header.has_measured_values) {
		ret = sen6x_prep_read_measured_values(dev, edata, &read_sqe);
		if (ret < 0) {
			LOG_ERR("Fail to prepare data read: %d", ret);
			rtio_iodev_sqe_err(iodev_sqe, ret);
			return;
		}
		read_sqe->flags |= RTIO_SQE_CHAINED;
	}

	if (edata->header.has_number_concentration) {
		if (!sen6x_is_measuring(dev)) {
			rtio_iodev_sqe_err(iodev_sqe, -EPERM);
			return;
		}

		ret = sen6x_prep_reg_read_rtio_async(
			dev, REG_READ_NUMBER_CONCENTRATION_VALUES_SEN6X,
			&edata->channels[MAX_MEASURED_VALUES_SIZE], 15, K_MSEC(20), &read_sqe);
		if (ret < 0) {
			LOG_ERR("Fail to prepare number-concentration read: %d", ret);
			rtio_iodev_sqe_err(iodev_sqe, ret);
			return;
		}
		read_sqe->flags |= RTIO_SQE_CHAINED;
	}

	complete_sqe = rtio_sqe_acquire(data->rtio_ctx);
	if (!complete_sqe) {
		LOG_ERR("Failed to acquire complete read-sqe");
		rtio_sqe_drop_all(data->rtio_ctx);
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	rtio_sqe_prep_callback_no_cqe(complete_sqe, sen6x_complete_result, (void *)dev, iodev_sqe);

	rtio_submit(data->rtio_ctx, 0);
}

static void sen6x_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;

	if (cfg->is_streaming) {
		LOG_ERR("Streaming not supported");
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
	} else {
		sen6x_submit_one_shot(dev, iodev_sqe);
	}
}

DEVICE_API(sensor, sen6x_driver_api) = {
	.get_decoder = sen6x_get_decoder,
	.submit = sen6x_submit,
};

void sen6x_add_callback(const struct device *dev, struct sen6x_callback *callback)
{
	struct sen6x_data *data = dev->data;

	sys_slist_find_and_remove(&data->callbacks, &callback->node);
	sys_slist_prepend(&data->callbacks, &callback->node);

	if (callback->status_changed != NULL) {
		callback->status_changed(dev, callback, data->status);
	}
}

void sen6x_remove_callback(const struct device *dev, struct sen6x_callback *callback)
{
	struct sen6x_data *data = dev->data;

	sys_slist_find_and_remove(&data->callbacks, &callback->node);
}

static void sen6x_netbuf_add_checksum(struct net_buf_simple *buf)
{
	__ASSERT_NO_MSG((buf->len % 3) == 2);

	uint8_t *tail = net_buf_simple_tail(buf);
	uint8_t checksum = crc8(&tail[-2], 2, SEN6X_CRC_POLY, SEN6X_CRC_INIT, SEN6X_CRC_REV);

	net_buf_simple_add_u8(buf, checksum);
}

static void sen6x_netbuf_add_u16(struct net_buf_simple *buf, uint16_t val)
{
	net_buf_simple_add_be16(buf, val);
	sen6x_netbuf_add_checksum(buf);
}

static void sen6x_netbuf_add_i16(struct net_buf_simple *buf, int16_t val)
{
	sen6x_netbuf_add_u16(buf, (uint16_t)val);
}

static int sen6x_read_bytes(const struct device *dev, uint16_t reg, k_timeout_t delay, void *value_,
			    size_t max_value_len, size_t read_size)
{
	int ret;
	uint8_t *value = value_;

	if (read_size % 3) {
		return -EINVAL;
	}
	if (max_value_len < read_size) {
		return -ENOBUFS;
	}

	ret = sen6x_reg_read_rtio(dev, reg, value, read_size, delay);
	if (ret != 0) {
		return ret;
	}
	if (!sen6x_u16_array_checksum_ok(value, read_size)) {
		return -EIO;
	}
	sen6x_remove_checksums_from_rx_data(value, read_size / 3);

	/* Just in case values which are supposed to be zero-terminated, arent't. */
	value[(read_size / 3) * 2] = 0;

	return 0;
}

int sen6x_get_product_name(const struct device *dev, char *name, size_t max_name_len)
{
	const struct sen6x_config *cfg = dev->config;

	if (cfg->model == SEN6X_MODEL_SEN60) {
		return -ENOTSUP;
	}

	return sen6x_read_bytes(dev, REG_GET_PRODUCT_NAME, K_MSEC(20), name, max_name_len, 48);
}

int sen6x_get_serial_number(const struct device *dev, char *serial, size_t max_serial_len)
{
	const struct sen6x_config *cfg = dev->config;
	int ret;

	if (cfg->model == SEN6X_MODEL_SEN60) {
		uint8_t *serial_bytes = serial;

		if (sen6x_is_measuring(dev)) {
			return -EPERM;
		}

		ret = sen6x_read_bytes(dev, REG_GET_SERIAL_NUMBER_SEN60, K_MSEC(1), serial,
				       max_serial_len, 6);
		if (ret != 0) {
			return ret;
		}

		ret = snprintf(serial, max_serial_len, "%02X%02X%02X%02X%02X%02X", serial_bytes[0],
			       serial_bytes[1], serial_bytes[2], serial_bytes[3], serial_bytes[4],
			       serial_bytes[5]);
		if (ret < 0 || (size_t)ret >= max_serial_len) {
			return -ENOBUFS;
		}

		return 0;
	} else {
		return sen6x_read_bytes(dev, REG_GET_SERIAL_NUMBER_SEN6X, K_MSEC(20), serial,
					max_serial_len, 48);
	}
}

int sen6x_set_temperature_offset_parameters(
	const struct device *dev, const struct sen6x_temperature_offset_parameters *params)
{
	const struct sen6x_config *cfg = dev->config;

	NET_BUF_SIMPLE_DEFINE(buffer, 12);

	if (cfg->model == SEN6X_MODEL_SEN60) {
		return -ENOTSUP;
	}
	if (!IN_RANGE(params->slot, 0, 4)) {
		return -EINVAL;
	}

	sen6x_netbuf_add_i16(&buffer, params->offset);
	sen6x_netbuf_add_i16(&buffer, params->slope);
	sen6x_netbuf_add_u16(&buffer, params->time_constant);
	sen6x_netbuf_add_u16(&buffer, params->slot);

	return sen6x_reg_write_rtio(dev, REG_SET_TEMPERATURE_OFFSET_PARAMETERS, buffer.data,
				    buffer.len, K_MSEC(20));
}

int sen6x_set_temperature_acceleration_parameters(
	const struct device *dev, const struct sen6x_temperature_acceleration_parameters *params)
{
	const struct sen6x_config *cfg = dev->config;

	NET_BUF_SIMPLE_DEFINE(buffer, 12);

	if (cfg->model == SEN6X_MODEL_SEN60) {
		return -ENOTSUP;
	}
	if (sen6x_is_measuring(dev)) {
		return -EPERM;
	}

	sen6x_netbuf_add_u16(&buffer, params->K);
	sen6x_netbuf_add_u16(&buffer, params->P);
	sen6x_netbuf_add_u16(&buffer, params->T1);
	sen6x_netbuf_add_u16(&buffer, params->T2);

	return sen6x_reg_write_rtio(dev, REG_SET_TEMPERATURE_ACCELERATION_PARAMETERS, buffer.data,
				    buffer.len, K_MSEC(20));
}

static int sen6x_tuning_parameters_valid(const struct sen6x_algorithm_tuning_parameters *params)
{
	if (!IN_RANGE(params->index_offset, 1, 250)) {
		return false;
	}
	if (!IN_RANGE(params->learning_time_offset_hours, 1, 1000)) {
		return false;
	}
	if (!IN_RANGE(params->learning_time_gain_hours, 1, 1000)) {
		return false;
	}
	if (!IN_RANGE(params->gating_max_duration_minutes, 0, 3000)) {
		return false;
	}
	if (!IN_RANGE(params->std_initial, 10, 5000)) {
		return false;
	}
	if (!IN_RANGE(params->gain_factor, 1, 1000)) {
		return false;
	}

	return true;
}

static void
sen6x_netbuf_add_tuning_parameters(struct net_buf_simple *buffer,
				   const struct sen6x_algorithm_tuning_parameters *params)
{
	sen6x_netbuf_add_i16(buffer, params->index_offset);
	sen6x_netbuf_add_i16(buffer, params->learning_time_offset_hours);
	sen6x_netbuf_add_i16(buffer, params->learning_time_gain_hours);
	sen6x_netbuf_add_i16(buffer, params->gating_max_duration_minutes);
	sen6x_netbuf_add_i16(buffer, params->std_initial);
	sen6x_netbuf_add_i16(buffer, params->gain_factor);
}

int sen6x_set_voc_algorithm_tuning_parameters(
	const struct device *dev, const struct sen6x_algorithm_tuning_parameters *params)
{
	const struct sen6x_config *cfg = dev->config;

	NET_BUF_SIMPLE_DEFINE(buffer, 18);

	switch (cfg->model) {
	case SEN6X_MODEL_SEN65:
	case SEN6X_MODEL_SEN66:
	case SEN6X_MODEL_SEN68:
		break;
	default:
		return -ENOTSUP;
	}

	if (!sen6x_tuning_parameters_valid(params)) {
		return -EINVAL;
	}
	if (sen6x_is_measuring(dev)) {
		return -EPERM;
	}

	sen6x_netbuf_add_tuning_parameters(&buffer, params);

	return sen6x_reg_write_rtio(dev, REG_VOC_ALGORITHM_TUNING_PARAMETERS, buffer.data,
				    buffer.len, K_MSEC(20));
}

int sen6x_set_nox_algorithm_tuning_parameters(
	const struct device *dev, const struct sen6x_algorithm_tuning_parameters *params)
{
	const struct sen6x_config *cfg = dev->config;

	NET_BUF_SIMPLE_DEFINE(buffer, 18);

	switch (cfg->model) {
	case SEN6X_MODEL_SEN65:
	case SEN6X_MODEL_SEN66:
	case SEN6X_MODEL_SEN68:
		break;
	default:
		return -ENOTSUP;
	}

	if (!sen6x_tuning_parameters_valid(params)) {
		return -EINVAL;
	}
	if (sen6x_is_measuring(dev)) {
		return -EPERM;
	}

	sen6x_netbuf_add_tuning_parameters(&buffer, params);

	return sen6x_reg_write_rtio(dev, REG_NOX_ALGORITHM_TUNING_PARAMETERS, buffer.data,
				    buffer.len, K_MSEC(20));
}

int sen6x_set_co2_automatic_self_calibration(const struct device *dev, bool enabled)
{
	const struct sen6x_config *cfg = dev->config;

	NET_BUF_SIMPLE_DEFINE(buffer, 3);

	switch (cfg->model) {
	case SEN6X_MODEL_SEN63C:
	case SEN6X_MODEL_SEN66:
		break;
	default:
		return -ENOTSUP;
	}

	if (sen6x_is_measuring(dev)) {
		return -EPERM;
	}
	if (sen6x_is_co2_conditioning_running(dev)) {
		return -EAGAIN;
	}

	net_buf_simple_add_u8(&buffer, 0x00);
	net_buf_simple_add_u8(&buffer, enabled ? 0x01 : 0x00);
	sen6x_netbuf_add_checksum(&buffer);

	return sen6x_reg_write_rtio(dev, REG_CO2_SENSOR_AUTOMATIC_SELF_CALIBRATION, buffer.data,
				    buffer.len, K_MSEC(20));
}

int sen6x_set_ambient_pressure(const struct device *dev, uint16_t value)
{
	const struct sen6x_config *cfg = dev->config;

	NET_BUF_SIMPLE_DEFINE(buffer, 3);

	switch (cfg->model) {
	case SEN6X_MODEL_SEN63C:
	case SEN6X_MODEL_SEN66:
		break;
	default:
		return -ENOTSUP;
	}

	if (!IN_RANGE(value, 700, 1200)) {
		return -EINVAL;
	}

	sen6x_netbuf_add_u16(&buffer, value);

	return sen6x_reg_write_rtio(dev, REG_AMBIENT_PRESSURE, buffer.data, buffer.len, K_MSEC(20));
}

int sen6x_set_sensor_altitude(const struct device *dev, uint16_t value)
{
	const struct sen6x_config *cfg = dev->config;

	NET_BUF_SIMPLE_DEFINE(buffer, 3);

	switch (cfg->model) {
	case SEN6X_MODEL_SEN63C:
	case SEN6X_MODEL_SEN66:
		break;
	default:
		return -ENOTSUP;
	}

	if (!IN_RANGE(value, 0, 3000)) {
		return -EINVAL;
	}
	if (sen6x_is_measuring(dev)) {
		return -EPERM;
	}

	sen6x_netbuf_add_u16(&buffer, value);

	return sen6x_reg_write_rtio(dev, REG_SENSOR_ALTITUDE, buffer.data, buffer.len, K_MSEC(20));
}

int sen6x_start_fan_cleaning(const struct device *dev)
{
	const struct sen6x_config *cfg = dev->config;

	if (sen6x_is_measuring(dev)) {
		return -EPERM;
	}

	if (cfg->model == SEN6X_MODEL_SEN60) {
		return sen6x_reg_write_rtio(dev, REG_START_FAN_CLEANING_SEN60, NULL, 0, K_MSEC(1));
	} else {
		return sen6x_reg_write_rtio(dev, REG_START_FAN_CLEANING_SEN6X, NULL, 0, K_MSEC(20));
	}
}

static int sen6x_get_heater_measurements(const struct device *dev,
					 struct sensor_q31_data *out_relative_humidity,
					 struct sensor_q31_data *out_temperature)
{
	int ret;
	int16_t relative_humidity = INT16_MAX;
	int16_t temperature = INT16_MAX;
	uint8_t measurements[6];
	uint64_t cycles;
	uint64_t timestamp;
	const int8_t shift = 16;
	/* With the 50ms sleep and the 20ms read this should result in a timeout of
	 * roughly 2s, which I chose as an arbitrary value higher than the 1.3s of
	 * older firmware versions.
	 */
	size_t attempts_left = 28;

	ret = sen6x_reg_write_rtio(dev, REG_ACTIVATE_SHT_HEATER, NULL, 0, K_MSEC(20));
	if (ret != 0) {
		return ret;
	}

	for (; relative_humidity == INT16_MAX || temperature == INT16_MAX; attempts_left--) {
		if (attempts_left == 0) {
			return -ETIMEDOUT;
		}

		k_sleep(K_MSEC(50));

		ret = sensor_clock_get_cycles(&cycles);
		if (ret != 0) {
			return ret;
		}

		ret = sen6x_read_bytes(dev, REG_GET_SHT_HEATER_MEASUREMENTS, K_MSEC(20),
				       measurements, sizeof(measurements), sizeof(measurements));
		if (ret != 0) {
			return ret;
		}

		relative_humidity = sys_get_be16(&measurements[0]);
		temperature = sys_get_be16(&measurements[2]);
	}

	timestamp = sensor_clock_cycles_to_ns(cycles);

	if (out_relative_humidity) {
		*out_relative_humidity = (struct sensor_q31_data){
			.shift = shift,
			.header.base_timestamp_ns = timestamp,
			.header.reading_count = 1,
			.readings[0].value = (((int64_t)relative_humidity) << (31 - shift)) / 100,
		};
	}
	if (out_temperature) {
		*out_temperature = (struct sensor_q31_data){
			.shift = shift,
			.header.base_timestamp_ns = timestamp,
			.header.reading_count = 1,
			.readings[0].value = (((int64_t)temperature) << (31 - shift)) / 200,
		};
	}

	return 0;
}

int sen6x_activate_sht_heater(const struct device *dev,
			      struct sensor_q31_data *out_relative_humidity,
			      struct sensor_q31_data *out_temperature)
{
	const struct sen6x_config *cfg = dev->config;
	struct sen6x_data *data = dev->data;
	bool supports_get_heater_measurements;
	int ret;

	if (sen6x_is_measuring(dev)) {
		return -EPERM;
	}

	switch (cfg->model) {
	case SEN6X_MODEL_SEN63C:
		supports_get_heater_measurements = data->firmware_version.major >= 5;
		break;
	case SEN6X_MODEL_SEN65:
		supports_get_heater_measurements = data->firmware_version.major >= 5;
		break;
	case SEN6X_MODEL_SEN66:
		supports_get_heater_measurements = data->firmware_version.major >= 4;
		break;
	case SEN6X_MODEL_SEN68:
		supports_get_heater_measurements = data->firmware_version.major >= 7;
		break;
	default:
		return -ENOTSUP;
	}

	if (supports_get_heater_measurements) {
		ret = sen6x_get_heater_measurements(dev, out_relative_humidity, out_temperature);
		if (ret != 0) {
			return ret;
		}
	} else {
		ret = sen6x_reg_write_rtio(dev, REG_ACTIVATE_SHT_HEATER, NULL, 0, K_MSEC(1300));
		if (ret != 0) {
			return ret;
		}

		if (out_relative_humidity) {
			out_relative_humidity->header.reading_count = 0;
		}
		if (out_temperature) {
			out_temperature->header.reading_count = 0;
		}
	}

	return 0;
}

int sen6x_get_voc_algorithm_state(const struct device *dev, struct sen6x_voc_algorithm_state *state)
{
	const struct sen6x_config *cfg = dev->config;
	int ret;

	switch (cfg->model) {
	case SEN6X_MODEL_SEN65:
	case SEN6X_MODEL_SEN66:
	case SEN6X_MODEL_SEN68:
		break;
	default:
		return -ENOTSUP;
	}

	ret = sen6x_reg_read_rtio(dev, REG_VOC_ALGORITHM_STATE, state->buffer,
				  sizeof(state->buffer), K_MSEC(20));
	if (ret != 0) {
		return ret;
	}
	if (!sen6x_u16_array_checksum_ok(state->buffer, sizeof(state->buffer))) {
		return -EIO;
	}

	return 0;
}

int sen6x_set_voc_algorithm_state(const struct device *dev,
				  const struct sen6x_voc_algorithm_state *state)
{
	const struct sen6x_config *cfg = dev->config;
	int ret;

	switch (cfg->model) {
	case SEN6X_MODEL_SEN65:
	case SEN6X_MODEL_SEN66:
	case SEN6X_MODEL_SEN68:
		break;
	default:
		return -ENOTSUP;
	}

	if (!sen6x_u16_array_checksum_ok(state->buffer, sizeof(state->buffer))) {
		return -EINVAL;
	}
	if (sen6x_is_measuring(dev)) {
		return -EPERM;
	}

	ret = sen6x_reg_write_rtio(dev, REG_VOC_ALGORITHM_STATE, state->buffer,
				   sizeof(state->buffer), K_MSEC(20));
	if (ret != 0) {
		return ret;
	}

	return 0;
}

const struct sen6x_firmware_version *sen6x_get_firmware_version(const struct device *dev)
{
	struct sen6x_data *data = dev->data;

	return &data->firmware_version;
}

#ifdef CONFIG_PM_DEVICE
static int sen6x_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct sen6x_data *data = dev->data;
	int ret = -EIO;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		if (data->was_measuring_before_suspend) {
			ret = sen6x_start_continuous_measurement(dev);
			if (ret != 0) {
				LOG_ERR("Failed to start continuous measurement %d", ret);
				return ret;
			}
		}
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		data->was_measuring_before_suspend = sen6x_is_measuring(dev);
		if (data->was_measuring_before_suspend) {
			ret = sen6x_stop_continuous_measurement(dev);
			if (ret != 0) {
				LOG_ERR("Failed to stop continuous measurement %d", ret);
				return ret;
			}
		}
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}
#endif

static int sen6x_init(const struct device *dev)
{
	struct sen6x_data *data = dev->data;
	const struct sen6x_config *cfg = dev->config;
	int ret;

	if (!i2c_is_ready_iodev(data->iodev)) {
		LOG_ERR("Bus is not ready");
		return -ENODEV;
	}

	/* Sensor startup time (Time after power-on until I2C communication can be started). */
	k_sleep(K_MSEC(100));

	ret = sen6x_device_reset(dev);
	if (ret != 0) {
		LOG_ERR("Failed to reset %d", ret);
		return ret;
	}

	if (cfg->model != SEN6X_MODEL_SEN60) {
		uint8_t version[3];

		ret = sen6x_reg_read_rtio(dev, REG_GET_VERSION_SEN6X, version, sizeof(version),
					  K_MSEC(20));
		if (ret != 0) {
			LOG_ERR("Failed to read version %d", ret);
			return ret;
		}

		if (!sen6x_u16_array_checksum_ok(version, 3)) {
			LOG_WRN("CRC check failed on version data.");
			return -EIO;
		}

		data->firmware_version = (struct sen6x_firmware_version){
			.major = version[0],
			.minor = version[1],
		};
		LOG_DBG("version: %u.%u", data->firmware_version.major,
			data->firmware_version.minor);
	}

	if (cfg->start_measurement_on_init) {
		ret = sen6x_start_continuous_measurement(dev);
		if (ret != 0) {
			LOG_ERR("Failed to start continuous measurement %d", ret);
			return ret;
		}
	}

	LOG_DBG("Init OK");

	return 0;
}

#define SEN6X_INIT(inst)                                                                           \
	BUILD_ASSERT(DT_INST_ENUM_IDX(inst, model) != SEN6X_MODEL_SEN60 ||                         \
			     DT_INST_PROP(inst, auto_clear_device_status) == false,                \
		     "SEN60 doesn't support auto-clearing the device status");                     \
                                                                                                   \
	RTIO_DEFINE(sen6x_rtio_ctx_##inst, 32, 32);                                                \
	I2C_DT_IODEV_DEFINE(sen6x_bus_##inst, DT_DRV_INST(inst));                                  \
	PM_DEVICE_DT_INST_DEFINE(inst, sen6x_pm_action);                                           \
                                                                                                   \
	static const struct sen6x_config sen6x_cfg_##inst = {                                      \
		.model = DT_INST_ENUM_IDX(inst, model),                                            \
		.auto_clear_device_status = DT_INST_PROP(inst, auto_clear_device_status),          \
		.start_measurement_on_init = DT_INST_PROP(inst, start_measurement_on_init),        \
	};                                                                                         \
	static struct sen6x_data sen6x_data_##inst = {                                             \
		.rtio_ctx = &sen6x_rtio_ctx_##inst,                                                \
		.iodev = &sen6x_bus_##inst,                                                        \
		.callbacks = SYS_SLIST_STATIC_INIT(&sen6x_data_##inst.callbacks),                  \
		.status_work = Z_WORK_INITIALIZER(status_work_handler),                            \
		.dev = DEVICE_DT_INST_GET(inst),                                                   \
		.measurement_enabled = ATOMIC_INIT(0),                                             \
		.measurement_state_changed_time = INT64_MIN,                                       \
		.co2_conditioning_started_time = INT64_MIN,                                        \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, sen6x_init, PM_DEVICE_DT_INST_GET(inst),                \
				     &sen6x_data_##inst, &sen6x_cfg_##inst, POST_KERNEL,           \
				     CONFIG_SENSOR_INIT_PRIORITY, &sen6x_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SEN6X_INIT)
