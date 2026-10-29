/*
 * Copyright 2026 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adc_comparator

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/comparator.h>
#include <zephyr/drivers/comparator/adc_comp.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

LOG_MODULE_REGISTER(adc_comparator, CONFIG_COMPARATOR_LOG_LEVEL);

struct adc_comp_config {
	struct adc_dt_spec port;
};

struct adc_comp_data {
	atomic_t threshold_mv;
};

static int adc_comp_get_output(const struct device *dev)
{
	const struct adc_comp_config *cfg = dev->config;
	struct adc_comp_data *data = dev->data;
	uint16_t raw_buf;
	int32_t val_mv;
	int ret;
	struct adc_sequence seq = {
		.buffer = &raw_buf,
		.buffer_size = sizeof(raw_buf),
	};

	ret = adc_sequence_init_dt(&cfg->port, &seq);
	if (ret < 0) {
		LOG_ERR("Failed to init ADC sequence: %d", ret);
		return ret;
	}

	ret = adc_read_dt(&cfg->port, &seq);
	if (ret < 0) {
		LOG_ERR("Failed to read ADC channel: %d", ret);
		return ret;
	}

	/*
	 * Differential channels return signed samples, whereas single-ended
	 * channels return unsigned samples; convert accordingly.
	 */
	if (cfg->port.channel_cfg.differential != 0U) {
		val_mv = (int16_t)raw_buf;
	} else {
		val_mv = (int32_t)raw_buf;
	}
	ret = adc_raw_to_millivolts_dt(&cfg->port, &val_mv);
	if (ret < 0) {
		LOG_ERR("Failed to convert ADC raw to mV: %d", ret);
		return ret;
	}

	return val_mv > (int32_t)atomic_get(&data->threshold_mv) ? 1 : 0;
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

int comp_adc_comp_set_threshold(const struct device *dev, int32_t ref_mv)
{
	struct adc_comp_data *data = dev->data;

	(void)atomic_set(&data->threshold_mv, (atomic_val_t)ref_mv);

	return 0;
}

static int adc_comp_init(const struct device *dev)
{
	const struct adc_comp_config *cfg = dev->config;
	int ret;

	if (!adc_is_ready_dt(&cfg->port)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->port.dev);
		return -ENODEV;
	}

	ret = adc_channel_setup_dt(&cfg->port);
	if (ret < 0) {
		LOG_ERR("Failed to setup ADC channel: %d", ret);
		return ret;
	}

	return 0;
}

#define ADC_COMP_CHANNEL_NODE(inst)                                                                \
	DT_CHILD_BY_UNIT_ADDR_INT(DT_INST_IO_CHANNELS_CTLR(inst), DT_INST_IO_CHANNELS_INPUT(inst))

#define ADC_COMP_INIT(inst)                                                                        \
	BUILD_ASSERT(DT_PROP_OR(ADC_COMP_CHANNEL_NODE(inst), zephyr_resolution, 0) > 0 &&          \
			     DT_PROP_OR(ADC_COMP_CHANNEL_NODE(inst), zephyr_resolution, 0) <= 16,  \
		     "ADC comparator requires ADC channel resolution between 1 and 16 bits");      \
                                                                                                   \
	static const struct adc_comp_config adc_comp_cfg_##inst = {                                \
		.port = ADC_DT_SPEC_INST_GET(inst),                                                \
	};                                                                                         \
	static struct adc_comp_data adc_comp_data_##inst = {                                       \
		.threshold_mv = ATOMIC_INIT(DT_INST_PROP(inst, threshold_mv)),                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, adc_comp_init, NULL, &adc_comp_data_##inst,                    \
			      &adc_comp_cfg_##inst, POST_KERNEL,                                   \
			      CONFIG_COMPARATOR_ADC_INIT_PRIORITY, &adc_comp_api);

DT_INST_FOREACH_STATUS_OKAY(ADC_COMP_INIT)
