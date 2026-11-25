/*
 * SPDX-FileCopyrightText: Copyright RAKwireless Technology Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_drv505x

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(drv505x, CONFIG_SENSOR_LOG_LEVEL);

#define DRV505X_MICRO_GAUSS_PER_MILLITESLA 10000000

struct drv505x_config {
	struct adc_dt_spec adc;
	int32_t sensitivity_uv_per_mt;
	int32_t quiescent_uv;
};

struct drv505x_data {
	struct adc_sequence sequence;
	uint16_t sample;
};

static int drv505x_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct drv505x_config *config = dev->config;
	struct drv505x_data *data = dev->data;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_MAGN_Z) {
		return -ENOTSUP;
	}

	return adc_read_dt(&config->adc, &data->sequence);
}

static int drv505x_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	const struct drv505x_config *config = dev->config;
	struct drv505x_data *data = dev->data;
	int32_t microvolts;

	if (chan != SENSOR_CHAN_MAGN_Z) {
		return -ENOTSUP;
	}

	microvolts = (int32_t)(((int64_t)data->sample * config->adc.vref_mv * 1000) /
			       BIT(config->adc.resolution));

	return sensor_value_from_micro(val, ((int64_t)(microvolts - config->quiescent_uv) *
					     DRV505X_MICRO_GAUSS_PER_MILLITESLA) /
						    config->sensitivity_uv_per_mt);
}

static DEVICE_API(sensor, drv505x_api) = {
	.sample_fetch = drv505x_sample_fetch,
	.channel_get = drv505x_channel_get,
};

static int drv505x_init(const struct device *dev)
{
	const struct drv505x_config *config = dev->config;
	struct drv505x_data *data = dev->data;
	int ret;

	if (!adc_is_ready_dt(&config->adc)) {
		LOG_ERR_DEVICE_NOT_READY(config->adc.dev);
		return -ENODEV;
	}

	if (config->adc.vref_mv == 0) {
		LOG_ERR("The ADC channel states no reference voltage");
		return -EINVAL;
	}

	ret = adc_channel_setup_dt(&config->adc);
	if (ret < 0) {
		LOG_ERR("Failed to set up the ADC channel (%d)", ret);
		return ret;
	}

	ret = adc_sequence_init_dt(&config->adc, &data->sequence);
	if (ret < 0) {
		LOG_ERR("Failed to initialize the ADC sequence (%d)", ret);
		return ret;
	}

	data->sequence.buffer = &data->sample;
	data->sequence.buffer_size = sizeof(data->sample);

	return 0;
}

#define DRV505X_DEFINE(inst)                                                                       \
	static struct drv505x_data drv505x_data_##inst;                                            \
                                                                                                   \
	static const struct drv505x_config drv505x_config_##inst = {                               \
		.adc = ADC_DT_SPEC_INST_GET(inst),                                                 \
		.sensitivity_uv_per_mt =                                                           \
			DT_INST_PROP(inst, sensitivity_microvolts_per_millitesla),                 \
		.quiescent_uv = DT_INST_PROP(inst, quiescent_microvolts),                          \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, drv505x_init, NULL, &drv505x_data_##inst,               \
				     &drv505x_config_##inst, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &drv505x_api);

DT_INST_FOREACH_STATUS_OKAY(DRV505X_DEFINE)
