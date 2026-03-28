/*
 * Copyright (c) 2025 Dipak Shetty <shetty.dipak@gmx.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT everlight_als_pt19

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ALS_PT19, CONFIG_SENSOR_LOG_LEVEL);

struct als_pt19_data {
	struct adc_sequence sequence;
	uint16_t raw;
};

struct als_pt19_config {
	struct adc_dt_spec adc;
	uint32_t load_resistor;
};

static int als_pt19_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct als_pt19_config *config = dev->config;
	struct als_pt19_data *data = dev->data;
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_LIGHT) {
		LOG_ERR("Unsupported sensor channel");
		return -ENOTSUP;
	}

	ret = adc_read_dt(&config->adc, &data->sequence);
	if (ret != 0) {
		LOG_ERR("adc_read: %d", ret);
	}

	return ret;
}

static int als_pt19_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	const struct als_pt19_config *config = dev->config;
	struct als_pt19_data *data = dev->data;
	int32_t raw_val = data->raw;
	int ret;

	__ASSERT_NO_MSG(val != NULL);

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_LIGHT) {
		LOG_ERR("Unsupported sensor channel");
		return -ENOTSUP;
	}

	ret = adc_raw_to_millivolts_dt(&config->adc, &raw_val);
	if (ret != 0) {
		LOG_ERR("to_mv: %d", ret);
		return ret;
	}

	LOG_DBG("Raw voltage: %d mV", raw_val);

	/* Calculate current through the load resistor (in microamps) */
	const int32_t current_ua = (raw_val * 1000) / config->load_resistor;

	/* Convert current to Lux (200µA = 1000 Lux) */
	const int32_t lux = (current_ua * 1000) / 200;

	/* Convert to sensor_value format */
	val->val1 = lux;
	val->val2 = 0;

	LOG_DBG("ADC: %d/%dmV, Current: %duA, Lux: %d", raw_val, config->adc.vref_mv, current_ua,
		val->val1);

	return 0;
}

static DEVICE_API(sensor, als_pt19_driver_api) = {
	.sample_fetch = als_pt19_sample_fetch,
	.channel_get = als_pt19_channel_get,
};

static int als_pt19_init(const struct device *dev)
{
	const struct als_pt19_config *config = dev->config;
	struct als_pt19_data *data = dev->data;
	int ret;

	if (!adc_is_ready_dt(&config->adc)) {
		LOG_ERR("ADC is not ready");
		return -ENODEV;
	}

	ret = adc_channel_setup_dt(&config->adc);
	if (ret != 0) {
		LOG_ERR("ADC channel setup: %d", ret);
		return ret;
	}

	ret = adc_sequence_init_dt(&config->adc, &data->sequence);
	if (ret != 0) {
		LOG_ERR("ADC sequence: %d", ret);
		return ret;
	}

	data->sequence.buffer = &data->raw;
	data->sequence.buffer_size = sizeof(data->raw);

	return 0;
}

#define ALS_PT19_INST(inst)                                                                        \
	static struct als_pt19_data als_pt19_data_##inst;                                          \
                                                                                                   \
	static const struct als_pt19_config als_pt19_cfg_##inst = {                                \
		.adc = ADC_DT_SPEC_INST_GET(inst),                                                 \
		.load_resistor = DT_INST_PROP(inst, load_resistor)};                               \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, &als_pt19_init, NULL, &als_pt19_data_##inst,            \
				     &als_pt19_cfg_##inst, POST_KERNEL,                            \
				     CONFIG_SENSOR_INIT_PRIORITY, &als_pt19_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ALS_PT19_INST)
