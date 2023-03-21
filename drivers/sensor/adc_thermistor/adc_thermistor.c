/*
 * Copyright (c) 2023 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(adc_thermistor, CONFIG_SENSOR_LOG_LEVEL);

#define DT_DRV_COMPAT zephyr_adc_thermistor

struct adc_thermistor_config {
	const struct adc_dt_spec adc;

	const int32_t *lut;
	size_t lut_size;
};

struct adc_thermistor_data {
	struct k_mutex mutex;

	int32_t val;
};

static int adc_thermistor_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct adc_thermistor_config *config = dev->config;
	struct adc_thermistor_data *data = dev->data;
	int ret;

	uint16_t buf;
	struct adc_sequence sequence = {
		.buffer = &buf,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(buf),
	};

	/* Allow fetching using ambient or die temperature channel */
	switch (chan) {
	case SENSOR_CHAN_ALL:
	case SENSOR_CHAN_AMBIENT_TEMP:
	case SENSOR_CHAN_DIE_TEMP:
		break;
	default:
		return -ENOTSUP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	ret = adc_channel_setup_dt(&config->adc);
	if (ret < 0) {
		LOG_ERR("Failed to setup thermal ADC (%d)", ret);
		goto unlock;
	}

	ret = adc_sequence_init_dt(&config->adc, &sequence);
	if (ret < 0) {
		LOG_ERR("Failed to init thermal ADC sequence (%d)", ret);
		goto unlock;
	}

	ret = adc_read(config->adc.dev, &sequence);
	if (ret < 0) {
		LOG_ERR("Failed to read thermal ADC (%d)", ret);
		goto unlock;
	}

	data->val = buf;

	/* Try to convert to mV if supported, leave as raw otherwise */
	adc_raw_to_millivolts_dt(&config->adc, &data->val);

unlock:
	k_mutex_unlock(&data->mutex);

	return ret;
}

static int adc_thermistor_channel_get(const struct device *dev, enum sensor_channel chan,
				      struct sensor_value *val)
{
	const struct adc_thermistor_config *config = dev->config;
	struct adc_thermistor_data *data = dev->data;
	int temp_val = data->val;

	/* Allow getting using ambient or die temperature channel */
	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
	case SENSOR_CHAN_DIE_TEMP:
		break;
	default:
		return -ENOTSUP;
	}

	if (config->lut_size > 0) {
		size_t i;

		for (i = 0; i < config->lut_size; ++i) {
			if (temp_val >= config->lut[2 * i + 1]) {
				break;
			}
		}

		if (i == 0) {
			temp_val = config->lut[0];
		} else if (i >= config->lut_size) {
			temp_val = config->lut[2 * (config->lut_size - 1)];
		} else {
			/* Linear in between two values */
			int32_t adc_hi, adc_lo, temp_hi, temp_lo;

			adc_hi = config->lut[2 * i - 1];
			adc_lo = config->lut[2 * i + 1];

			if (adc_hi == adc_lo) {
				/* Prevent division by zero */
				LOG_ERR("Duplicate ADC entries for %d", adc_hi);
				return -EINVAL;
			}

			temp_hi = config->lut[2 * i - 2];
			temp_lo = config->lut[2 * i];

			temp_val = (temp_lo - temp_hi) * (temp_val - adc_hi);
			temp_val /= (adc_lo - adc_hi);
			temp_val += temp_hi;
		}
	}

	val->val1 = temp_val / 1000;
	val->val2 = (temp_val % 1000) * 1000;

	return 0;
}

static const struct sensor_driver_api adc_thermistor_driver_api = {
	.sample_fetch = adc_thermistor_sample_fetch,
	.channel_get = adc_thermistor_channel_get,
};

static int adc_thermistor_init(const struct device *dev)
{
	const struct adc_thermistor_config *config = dev->config;
	struct adc_thermistor_data *data = dev->data;

	k_mutex_init(&data->mutex);

	if (!device_is_ready(config->adc.dev)) {
		LOG_ERR("Thermal ADC device not ready");
		return -ENODEV;
	}

	return 0;
}

#define ADC_THERMISTOR_DEFINE(inst)								\
	BUILD_ASSERT(DT_INST_PROP_LEN_OR(inst, temperature_lookup_table, 0) % 2 == 0,		\
		     "Temperature lookup table needs an even size");				\
	static const int32_t adc_thermistor_lut_##inst[]					\
		= DT_INST_PROP_OR(inst, temperature_lookup_table, {});				\
												\
	static const struct adc_thermistor_config adc_thermistor_dev_config_##inst = {		\
		.adc = ADC_DT_SPEC_INST_GET(inst),						\
		.lut = adc_thermistor_lut_##inst,						\
		.lut_size = DT_INST_PROP_LEN_OR(inst, temperature_lookup_table, 0) / 2,		\
	};											\
												\
	static struct adc_thermistor_data adc_thermistor_dev_data_##inst;			\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, adc_thermistor_init, NULL,				\
				     &adc_thermistor_dev_data_##inst,				\
				     &adc_thermistor_dev_config_##inst,				\
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,			\
				     &adc_thermistor_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ADC_THERMISTOR_DEFINE)
