/*
 * Copyright (c) 2022 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_temp

#include <driver/temp_sensor.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(esp32_temp, CONFIG_SENSOR_LOG_LEVEL);

#if CONFIG_SOC_SERIES_ESP32
#error "Temperature sensor not supported on ESP32"
#endif /* CONFIG_IDF_TARGET_ESP32 */

struct esp32_temp_data {
	struct k_mutex mutex;
	temp_sensor_config_t temp_sensor;
	float temp_out;
};

struct esp32_temp_config {
	temp_sensor_dac_offset_t range;
};

static int esp32_temp_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct esp32_temp_data *data = dev->data;
	int rc = 0;

	k_mutex_lock(&data->mutex, K_FOREVER);

	if (temp_sensor_read_celsius(&data->temp_out) != ESP_OK) {
		LOG_ERR("Temperature read error!");
		rc = -EFAULT;
		goto unlock;
	}

unlock:
	k_mutex_unlock(&data->mutex);

	return rc;
}

static int esp32_temp_channel_get(const struct device *dev, enum sensor_channel chan,
					struct sensor_value *val)
{
	struct esp32_temp_data *data = dev->data;
	const struct esp32_temp_config *cfg = dev->config;

	if (chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	return sensor_value_from_double(val, data->temp_out);
}

static const struct sensor_driver_api esp32_temp_driver_api = {
	.sample_fetch = esp32_temp_sample_fetch,
	.channel_get = esp32_temp_channel_get,
};

static int esp32_temp_init(const struct device *dev)
{
	struct esp32_temp_data *data = dev->data;
	const struct esp32_temp_config *conf = dev->config;

	k_mutex_init(&data->mutex);
	temp_sensor_get_config(&data->temp_sensor);
	data->temp_sensor.dac_offset = conf->range;
	temp_sensor_set_config(data->temp_sensor);
	temp_sensor_start();
	LOG_DBG("Temperature sensor started. Offset %d, clk_div %d",
		data->temp_sensor.dac_offset, data->temp_sensor.clk_div);

	return 0;
}

#define ESP32_TEMP_DEFINE(inst)									\
	static struct esp32_temp_data esp32_temp_dev_data_##inst = {				\
		.temp_sensor = TSENS_CONFIG_DEFAULT(),						\
	};											\
												\
	static const struct esp32_temp_config esp32_temp_dev_config_##inst = {			\
		.range = (temp_sensor_dac_offset_t) DT_INST_PROP(inst, range),			\
	};											\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, esp32_temp_init, NULL,				\
			      &esp32_temp_dev_data_##inst, &esp32_temp_dev_config_##inst,	\
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,				\
			      &esp32_temp_driver_api);						\

DT_INST_FOREACH_STATUS_OKAY(ESP32_TEMP_DEFINE)
