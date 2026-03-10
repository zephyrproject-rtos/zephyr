/*
 * Copyright (c) 2022-2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_temp

#include <driver/temperature_sensor.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(esp32_temp, CONFIG_SENSOR_LOG_LEVEL);

#if CONFIG_SOC_SERIES_ESP32
#error "Temperature sensor not supported on ESP32"
#endif /* CONFIG_SOC_SERIES_ESP32 */

struct esp32_temp_data {
	temperature_sensor_config_t temp_sensor_config;
	temperature_sensor_handle_t temp_sensor_handle;
	float temp_out;
};

static int esp32_temp_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct esp32_temp_data *data = dev->data;
	int rc = 0;

	if (temperature_sensor_get_celsius(data->temp_sensor_handle, &data->temp_out) != ESP_OK) {
		LOG_ERR("Temperature read error!");
		rc = -EFAULT;
	}

	return rc;
}

static int esp32_temp_channel_get(const struct device *dev, enum sensor_channel chan,
					struct sensor_value *val)
{
	struct esp32_temp_data *data = dev->data;

	if (chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	return sensor_value_from_double(val, data->temp_out);
}

static DEVICE_API(sensor, esp32_temp_driver_api) = {
	.sample_fetch = esp32_temp_sample_fetch,
	.channel_get = esp32_temp_channel_get,
};

static int esp32_temp_init(const struct device *dev)
{
	struct esp32_temp_data *data = dev->data;

	temperature_sensor_install(&data->temp_sensor_config, &data->temp_sensor_handle);
	temperature_sensor_enable(data->temp_sensor_handle);

	return 0;
}

#define ESP32_TEMP_DEFINE(inst)									\
	static struct esp32_temp_data esp32_temp_dev_data_##inst = {				\
		.temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(			\
			DT_INST_PROP(inst, range_min),						\
			DT_INST_PROP(inst, range_max)						\
		),										\
	};											\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, esp32_temp_init, NULL,				\
			      &esp32_temp_dev_data_##inst, NULL,				\
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,				\
			      &esp32_temp_driver_api);						\

DT_INST_FOREACH_STATUS_OKAY(ESP32_TEMP_DEFINE)
