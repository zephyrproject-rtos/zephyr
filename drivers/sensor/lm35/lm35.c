/*
 * Copyright (c) 2024 Renato Soma <renatoys08@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT lm35

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define LM35_GAIN       ADC_GAIN_1
#define LM35_REF        ADC_REF_INTERNAL

LOG_MODULE_REGISTER(LM35, CONFIG_SENSOR_LOG_LEVEL);

struct lm35_data {
	uint16_t raw;
};

struct lm35_config {
	const struct device *adc;
	uint8_t adc_channel;
	struct adc_sequence adc_seq;
	struct adc_channel_cfg ch_cfg;
};

static int lm35_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	const struct lm35_config *cfg = dev->config;

	return adc_read(cfg->adc, &cfg->adc_seq);
}

static int lm35_channel_get(const struct device *dev, enum sensor_channel chan,
			    struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	int err;
	struct lm35_data *data = dev->data;
	const struct lm35_config *cfg = dev->config;
	int32_t mv = data->raw;

	err = adc_raw_to_millivolts(adc_ref_internal(cfg->adc), cfg->ch_cfg.gain,
				    cfg->adc_seq.resolution, &mv);
	if (err) {
		return err;
	}

	/* LM35 computes linearly 10mV per 1 degree C */
	val->val1 = mv / 10;
	val->val2 = (mv % 10) * 100000;

	return 0;
}

static DEVICE_API(sensor, lm35_driver_api) = {
	.sample_fetch = lm35_sample_fetch,
	.channel_get = lm35_channel_get,
};

static int lm35_init(const struct device *dev)
{
	const struct lm35_config *cfg = dev->config;

	if (!device_is_ready(cfg->adc)) {
		LOG_ERR("ADC device is not ready.");
		return -EINVAL;
	}

	adc_channel_setup(cfg->adc, &cfg->ch_cfg);
	return 0;
}

#define LM35_INST(inst)                                                                            \
	static struct lm35_data lm35_data_##inst;                                                  \
                                                                                                   \
	static const struct lm35_config lm35_cfg_##inst = {                                        \
		.adc = DEVICE_DT_GET(DT_INST_IO_CHANNELS_CTLR(inst)),                              \
		.adc_channel = DT_INST_IO_CHANNELS_INPUT(inst),                                    \
		.adc_seq =                                                                         \
			{                                                                          \
				.channels = BIT(DT_INST_IO_CHANNELS_INPUT(inst)),                  \
				.buffer = &lm35_data_##inst.raw,                                   \
				.buffer_size = sizeof(lm35_data_##inst.raw),                       \
				.resolution = DT_INST_PROP(inst, resolution),                      \
			},                                                                         \
		.ch_cfg = {                                                                        \
			.gain = LM35_GAIN,                                                         \
			.reference = LM35_REF,                                                     \
			.acquisition_time = ADC_ACQ_TIME_DEFAULT,                                  \
			.channel_id = DT_INST_IO_CHANNELS_INPUT(inst),                             \
		}};                                                                                \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, lm35_init, NULL, &lm35_data_##inst, &lm35_cfg_##inst,   \
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &lm35_driver_api);

DT_INST_FOREACH_STATUS_OKAY(LM35_INST)
