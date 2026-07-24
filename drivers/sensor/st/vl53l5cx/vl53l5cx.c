/*
 * Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_vl53l5cx

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/byteorder.h>

#include "vl53l5cx.h"
#include "vl53l5cx_decoder.h"
#include "vl53l5cx_rtio.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(vl53l5cx, CONFIG_SENSOR_LOG_LEVEL);

#define DATA_READY_TIMEOUT 1000

#define VL53L5CX_RESOLUTION_16	16
#define VL53L5CX_RESOLUTION_64	64

#define VL53L5CX_MIN_RANGING_FREQUENCY	1
#define VL53L5CX_16_MAX_RANGING_FREQUENCY	60
#define VL53L5CX_64_MAX_RANGING_FREQUENCY	15
static int vl53l5cx_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	struct vl53l5cx_data *data = dev->data;
	uint8_t param;
	uint8_t hal_ret;

	if (chan != SENSOR_CHAN_DISTANCE) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_RESOLUTION:
		if (val->val2 != 0 ||
		    (val->val1 != VL53L5CX_RESOLUTION_16 && val->val1 != VL53L5CX_RESOLUTION_64)) {
			return -ENOTSUP;
		}

		param = val->val1 == VL53L5CX_RESOLUTION_16 ? VL53L5CX_RESOLUTION_4X4 :
							      VL53L5CX_RESOLUTION_8X8;
		hal_ret = vl53l5cx_set_resolution(&data->vl53l5cx, param);
		if (hal_ret != 0) {
			return -EIO;
		}
		break;
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		if (val->val2 != 0 ||
		    val->val1 < VL53L5CX_MIN_RANGING_FREQUENCY ||
		    val->val1 > VL53L5CX_16_MAX_RANGING_FREQUENCY) {
			return -ENOTSUP;
		}

		hal_ret = vl53l5cx_set_ranging_frequency_hz(&data->vl53l5cx, val->val1);
		if (hal_ret != 0) {
			return -EIO;
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int vl53l5cx_attr_get(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, struct sensor_value *val)
{
	struct vl53l5cx_data *data = dev->data;
	uint8_t param;
	uint8_t hal_ret;

	if (chan != SENSOR_CHAN_DISTANCE) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_RESOLUTION:
		hal_ret = vl53l5cx_get_resolution(&data->vl53l5cx, &param);
		if (hal_ret != 0) {
			return -EIO;
		}
		val->val1 = param == VL53L5CX_RESOLUTION_4X4 ? VL53L5CX_RESOLUTION_16 :
							       VL53L5CX_RESOLUTION_64;
		val->val2 = 0;
		break;
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		hal_ret = vl53l5cx_get_ranging_frequency_hz(&data->vl53l5cx, &param);
		if (hal_ret != 0) {
			return -EIO;
		}
		val->val1 = param;
		val->val2 = 0;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int vl53l5cx_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct vl53l5cx_data *data = dev->data;
	uint32_t start_tick;
	uint8_t ready = 0;
	uint8_t hal_ret;
	int ret = 0;

	__ASSERT_NO_MSG((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_DIE_TEMP));

	hal_ret = vl53l5cx_start_ranging(&data->vl53l5cx);
	if (hal_ret != VL53L5CX_STATUS_OK) {
		return -EIO;
	}

	start_tick = k_uptime_get_32();

	do {
		hal_ret = vl53l5cx_check_data_ready(&data->vl53l5cx, &ready);
		if (hal_ret != VL53L5CX_STATUS_OK) {
			ret = -EIO;
			goto out;
		}
	} while (ready != 1 && (k_uptime_get_32() - start_tick) < DATA_READY_TIMEOUT);

	if (ready != 1) {
		ret = -ETIMEDOUT;
		goto out;
	}

	hal_ret = vl53l5cx_get_ranging_data(&data->vl53l5cx, &data->result_data);
	if (hal_ret != VL53L5CX_STATUS_OK) {
		ret = -EIO;
		goto out;
	}

out:
	hal_ret = vl53l5cx_stop_ranging(&data->vl53l5cx);
	if (hal_ret != VL53L5CX_STATUS_OK) {
		return -EIO;
	}

	return ret;
}

static int vl53l5cx_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct vl53l5cx_data *data = dev->data;

	if (chan == SENSOR_CHAN_DIE_TEMP) {
		val->val1 = data->result_data.silicon_temp_degc;
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static DEVICE_API(sensor, vl53l5cx_driver_api) = {
	.attr_set = vl53l5cx_attr_set,
	.attr_get = vl53l5cx_attr_get,
	.sample_fetch = vl53l5cx_sample_fetch,
	.channel_get = vl53l5cx_channel_get,
#ifdef CONFIG_SENSOR_ASYNC_API
	.get_decoder = vl53l5cx_get_decoder,
	.submit = vl53l5cx_submit,
#endif
};

static int vl53l5cx_dev_init(const struct device *dev)
{
	struct vl53l5cx_data *data = dev->data;
	uint8_t hal_ret;

	data->dev = dev;

	if (!i2c_is_ready_dt(&data->vl53l5cx.platform.i2c)) {
		LOG_ERR("I2C bus is not ready");
		return -ENODEV;
	}

	hal_ret = vl53l5cx_init(&data->vl53l5cx);
	if (hal_ret != VL53L5CX_STATUS_OK) {
		return -EIO;
	}

	return 0;
}

#define VL53L5CX_I2C_RTIO_DEFINE(i)					\
	I2C_DT_IODEV_DEFINE(vl53l5cx_iodev_##i, DT_DRV_INST(i));	\
	RTIO_DEFINE(vl53l5cx_rtio_ctx_##i, 4, 4);

#define VL53L5CX_INIT(i)                                                                           \
	IF_ENABLED(CONFIG_I2C_RTIO, (VL53L5CX_I2C_RTIO_DEFINE(i)));                                \
	static struct vl53l5cx_data vl53l5cx_data_##i = {                                          \
		.vl53l5cx.platform.i2c = I2C_DT_SPEC_INST_GET(i),                                  \
		IF_ENABLED(CONFIG_I2C_RTIO,                                                        \
			(.rtio_ctx = &vl53l5cx_rtio_ctx_##i,                                       \
			 .iodev = &vl53l5cx_iodev_##i,))                                           \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(i, vl53l5cx_dev_init, NULL, &vl53l5cx_data_##i, NULL,         \
				     POST_KERNEL,                                                  \
				     CONFIG_SENSOR_INIT_PRIORITY, &vl53l5cx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(VL53L5CX_INIT)
