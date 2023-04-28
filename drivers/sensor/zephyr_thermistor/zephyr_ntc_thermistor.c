/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_ntc_thermistor

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include "zephyr_ntc_thermistor.h"

LOG_MODULE_REGISTER(ZEPHYR_NTC_THERMISTOR, CONFIG_SENSOR_LOG_LEVEL);

struct zephyr_ntc_thermistor_data {
	struct k_mutex mutex;
	int16_t raw;
	int16_t sample_val;
};

struct zephyr_ntc_thermistor_config {
	const struct adc_dt_spec adc_channel;
	const struct ntc_config ntc_cfg;
};

static int zephyr_ntc_thermistor_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct zephyr_ntc_thermistor_data *data = dev->data;
	const struct zephyr_ntc_thermistor_config *cfg = dev->config;
	int32_t val_mv;
	int res;
	struct adc_sequence sequence = {
		.options = NULL,
		.buffer = &data->raw,
		.buffer_size = sizeof(data->raw),
		.calibrate = false,
	};

	k_mutex_lock(&data->mutex, K_FOREVER);

	adc_sequence_init_dt(&cfg->adc_channel, &sequence);
	res = adc_read(cfg->adc_channel.dev, &sequence);
	if (res) {
		val_mv = data->raw;
		res = adc_raw_to_millivolts_dt(&cfg->adc_channel, &val_mv);
		data->sample_val = val_mv;
	}

	k_mutex_unlock(&data->mutex);

	return res;
}

static int zephyr_ntc_thermistor_channel_get(const struct device *dev, enum sensor_channel chan,
					     struct sensor_value *val)
{
	struct zephyr_ntc_thermistor_data *data = dev->data;
	const struct zephyr_ntc_thermistor_config *cfg = dev->config;
	uint32_t ohm, max_adc;
	int32_t temp;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		max_adc = (1 << (cfg->adc_channel.resolution - 1)) - 1;
		ohm = ntc_get_ohm_of_thermistor(&cfg->ntc_cfg, max_adc, data->raw);
		temp = ntc_get_temp_mc(cfg->ntc_cfg.type, ohm);
		val->val1 = temp / 1000;
		val->val2 = (temp % 1000) * 1000;
		LOG_INF("ntc temp says %u", val->val1);
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

static const struct sensor_driver_api zephyr_ntc_thermistor_driver_api = {
	.sample_fetch = zephyr_ntc_thermistor_sample_fetch,
	.channel_get = zephyr_ntc_thermistor_channel_get,
};

static int zephyr_ntc_thermistor_init(const struct device *dev)
{
	const struct zephyr_ntc_thermistor_config *cfg = dev->config;
	int err;

	if (!device_is_ready(cfg->adc_channel.dev)) {
		LOG_ERR("ADC controller device is not ready\n");
		return -ENODEV;
	}

	err = adc_channel_setup_dt(&cfg->adc_channel);
	if (err < 0) {
		LOG_ERR("Could not setup channel err(%d)\n", err);
		return err;
	}

	return 0;
}

#define ZEPHYR_NTC_THERMISTOR_DEV_INIT(inst)                                                       \
	static struct zephyr_ntc_thermistor_data zephyr_ntc_thermistor_driver_##inst;              \
                                                                                                   \
	extern const struct ntc_type NTC_TYPE_NAME(DT_INST_PROP(inst, zephyr_rt_table));           \
                                                                                                   \
	static const struct zephyr_ntc_thermistor_config zephyr_ntc_thermistor_cfg_##inst = {      \
		.adc_channel = ADC_DT_SPEC_INST_GET(inst),                                         \
		.ntc_cfg =                                                                         \
			{                                                                          \
				.r25_ohm = DT_INST_PROP(inst, r25_ohm),                            \
				.pullup_uv = DT_INST_PROP(inst, pullup_uv),                        \
				.pullup_ohm = DT_INST_PROP(inst, pullup_ohm),                      \
				.pulldown_ohm = DT_INST_PROP(inst, pulldown_ohm),                  \
				.connected_positive = DT_INST_PROP(inst, connected_positive),      \
				.type = &NTC_TYPE_NAME(DT_INST_PROP(inst, zephyr_rt_table)),       \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(                                                              \
		inst, zephyr_ntc_thermistor_init, NULL, &zephyr_ntc_thermistor_driver_##inst,      \
		&zephyr_ntc_thermistor_cfg_##inst, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,       \
		&zephyr_ntc_thermistor_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ZEPHYR_NTC_THERMISTOR_DEV_INIT)
