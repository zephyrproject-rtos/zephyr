/*
 * SPDX-FileCopyrightText: Copyright RAKwireless Technology Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT winsen_mq

#include <math.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/mq.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mq, CONFIG_SENSOR_LOG_LEVEL);

struct mq_config {
	struct adc_dt_spec adc;
	uint32_t load_ohms;
	uint32_t r0_ohms;
	int32_t curve_scale_milli;
	int32_t curve_exponent_milli;
};

struct mq_data {
	struct adc_sequence sequence;
	uint16_t sample;
	uint32_t r0_ohms;
};

static int mq_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct mq_config *config = dev->config;
	struct mq_data *data = dev->data;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_GAS_RES &&
	    chan != (enum sensor_channel)SENSOR_CHAN_MQ_PPM) {
		return -ENOTSUP;
	}

	return adc_read_dt(&config->adc, &data->sequence);
}

static int mq_resistance(const struct device *dev, uint32_t *rs_ohms)
{
	const struct mq_config *config = dev->config;
	struct mq_data *data = dev->data;
	uint32_t full_scale = BIT(config->adc.resolution) - 1U;

	if (data->sample == 0U) {
		LOG_WRN("Sensor reads zero, the heater supply may be off");
		return -EIO;
	}

	if (data->sample >= full_scale) {
		return -ERANGE;
	}

	*rs_ohms = (uint32_t)(((uint64_t)config->load_ohms * (full_scale - data->sample)) /
			      data->sample);

	return 0;
}

static int mq_channel_get(const struct device *dev, enum sensor_channel chan,
			  struct sensor_value *val)
{
	const struct mq_config *config = dev->config;
	struct mq_data *data = dev->data;
	uint32_t rs_ohms;
	float ppm;
	int ret;

	ret = mq_resistance(dev, &rs_ohms);
	if (ret < 0) {
		return ret;
	}

	switch ((int)chan) {
	case SENSOR_CHAN_GAS_RES:
		val->val1 = (int32_t)rs_ohms;
		val->val2 = 0;
		return 0;
	case SENSOR_CHAN_MQ_PPM:
		if (config->curve_exponent_milli == 0) {
			return -ENOTSUP;
		}

		if (data->r0_ohms == 0U) {
			LOG_WRN("Clean air resistance unknown, set SENSOR_ATTR_MQ_R0");
			return -ENODATA;
		}

		ppm = (config->curve_scale_milli / 1000.0f) *
		      powf((float)rs_ohms / (float)data->r0_ohms,
			   config->curve_exponent_milli / 1000.0f);

		return sensor_value_from_float(val, ppm);
	default:
		return -ENOTSUP;
	}
}

static int mq_attr_set(const struct device *dev, enum sensor_channel chan,
		       enum sensor_attribute attr, const struct sensor_value *val)
{
	struct mq_data *data = dev->data;

	if (chan != SENSOR_CHAN_GAS_RES && chan != (enum sensor_channel)SENSOR_CHAN_MQ_PPM) {
		return -ENOTSUP;
	}

	if ((int)attr != SENSOR_ATTR_MQ_R0) {
		return -ENOTSUP;
	}

	if (val->val1 <= 0) {
		return -EINVAL;
	}

	data->r0_ohms = (uint32_t)val->val1;

	return 0;
}

static int mq_attr_get(const struct device *dev, enum sensor_channel chan,
		       enum sensor_attribute attr, struct sensor_value *val)
{
	struct mq_data *data = dev->data;

	if (chan != SENSOR_CHAN_GAS_RES && chan != (enum sensor_channel)SENSOR_CHAN_MQ_PPM) {
		return -ENOTSUP;
	}

	if ((int)attr != SENSOR_ATTR_MQ_R0) {
		return -ENOTSUP;
	}

	val->val1 = (int32_t)data->r0_ohms;
	val->val2 = 0;

	return 0;
}

static DEVICE_API(sensor, mq_api) = {
	.sample_fetch = mq_sample_fetch,
	.channel_get = mq_channel_get,
	.attr_set = mq_attr_set,
	.attr_get = mq_attr_get,
};

static int mq_init(const struct device *dev)
{
	const struct mq_config *config = dev->config;
	struct mq_data *data = dev->data;
	int ret;

	if (!adc_is_ready_dt(&config->adc)) {
		LOG_ERR_DEVICE_NOT_READY(config->adc.dev);
		return -ENODEV;
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
	data->r0_ohms = config->r0_ohms;

	return 0;
}

#define MQ_DEFINE(inst)									\
	static struct mq_data mq_data_##inst;						\
											\
	static const struct mq_config mq_config_##inst = {				\
		.adc = ADC_DT_SPEC_INST_GET(inst),					\
		.load_ohms = DT_INST_PROP(inst, load_resistance_ohms),			\
		.r0_ohms = DT_INST_PROP_OR(inst, r0_ohms, 0),				\
		.curve_scale_milli = DT_INST_PROP_OR(inst, curve_scale_milli, 0),	\
		.curve_exponent_milli = DT_INST_PROP_OR(inst, curve_exponent_milli, 0),	\
	};										\
											\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, mq_init, NULL, &mq_data_##inst,		\
				     &mq_config_##inst, POST_KERNEL,			\
				     CONFIG_SENSOR_INIT_PRIORITY, &mq_api);

DT_INST_FOREACH_STATUS_OKAY(MQ_DEFINE)
