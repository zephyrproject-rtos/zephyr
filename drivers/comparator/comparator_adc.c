/*
 * Copyright 2026 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adc_comparator

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/adc/voltage_divider.h>
#include <zephyr/drivers/comparator.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(adc_comparator, CONFIG_COMPARATOR_LOG_LEVEL);

struct adc_comp_config {
	struct voltage_divider_dt_spec voltage;
	int32_t threshold_mv;
};

static int adc_comp_get_output(const struct device *dev)
{
	const struct adc_comp_config *cfg = dev->config;
	int16_t raw_buf;
	int32_t val_mv;
	int ret;
	struct adc_sequence seq = {
		.buffer = &raw_buf,
		.buffer_size = sizeof(raw_buf),
	};

	ret = adc_sequence_init_dt(&cfg->voltage.port, &seq);
	if (ret < 0) {
		LOG_ERR("Failed to init ADC sequence: %d", ret);
		return ret;
	}

	ret = adc_read_dt(&cfg->voltage.port, &seq);
	if (ret < 0) {
		LOG_ERR("Failed to read ADC channel: %d", ret);
		return ret;
	}

	val_mv = raw_buf;
	ret = adc_raw_to_millivolts_dt(&cfg->voltage.port, &val_mv);
	if (ret < 0) {
		LOG_ERR("Failed to convert ADC raw to mV: %d", ret);
		return ret;
	}

	(void)voltage_divider_scale_dt(&cfg->voltage, &val_mv);

	return val_mv > cfg->threshold_mv ? 1 : 0;
}

static int adc_comp_set_trigger(const struct device *dev, enum comparator_trigger trigger)
{
	ARG_UNUSED(dev);

	if (trigger != COMPARATOR_TRIGGER_NONE) {
		return -ENOTSUP;
	}

	return 0;
}

static int adc_comp_set_trigger_callback(const struct device *dev, comparator_callback_t callback,
					 void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(callback);
	ARG_UNUSED(user_data);

	return -ENOTSUP;
}

static int adc_comp_trigger_is_pending(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static DEVICE_API(comparator, adc_comp_api) = {
	.get_output = adc_comp_get_output,
	.set_trigger = adc_comp_set_trigger,
	.set_trigger_callback = adc_comp_set_trigger_callback,
	.trigger_is_pending = adc_comp_trigger_is_pending,
};

static int adc_comp_init(const struct device *dev)
{
	const struct adc_comp_config *cfg = dev->config;
	int ret;

	if (!adc_is_ready_dt(&cfg->voltage.port)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->voltage.port.dev);
		return -ENODEV;
	}

	ret = adc_channel_setup_dt(&cfg->voltage.port);
	if (ret < 0) {
		LOG_ERR("Failed to setup ADC channel: %d", ret);
		return ret;
	}

	return 0;
}

#define ADC_COMP_INIT(inst)                                                                        \
	static const struct adc_comp_config adc_comp_cfg_##inst = {                                \
		.voltage = VOLTAGE_DIVIDER_DT_SPEC_GET(DT_DRV_INST(inst)),                         \
		.threshold_mv = DT_INST_PROP(inst, threshold_mv),                                  \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, adc_comp_init, NULL, NULL, &adc_comp_cfg_##inst, POST_KERNEL,  \
			      CONFIG_COMPARATOR_INIT_PRIORITY, &adc_comp_api);

DT_INST_FOREACH_STATUS_OKAY(ADC_COMP_INIT)
