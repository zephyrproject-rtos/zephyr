/*
 * Copyright (c) 2022 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(stm32_vbat, CONFIG_SENSOR_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_vbat)
#define DT_DRV_COMPAT st_stm32_vbat
#else
#error "No compatible devicetree node found"
#endif

struct stm32_vbat_data {
	const struct device *adc;
	const struct adc_channel_cfg adc_cfg;
	struct adc_sequence adc_seq;
	struct k_mutex mutex;
	int16_t sample_buffer;
	int16_t raw; /* raw adc Sensor value */
};

struct stm32_vbat_config {
	int ratio;
};

static int stm32_vbat_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct stm32_vbat_data *data = dev->data;
	struct adc_sequence *sp = &data->adc_seq;
	int rc;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	rc = adc_channel_setup(data->adc, &data->adc_cfg);

	if (rc) {
		LOG_DBG("Setup AIN%u got %d", data->adc_cfg.channel_id, rc);
		goto unlock;
	}

	rc = adc_read(data->adc, sp);
	if (rc == 0) {
		data->raw = data->sample_buffer;
	}

unlock:
	k_mutex_unlock(&data->mutex);

	return rc;
}

static int stm32_vbat_channel_get(const struct device *dev, enum sensor_channel chan,
				  struct sensor_value *val)
{
	struct stm32_vbat_data *data = dev->data;
	const struct stm32_vbat_config *cfg = dev->config;
	float voltage;

	if (chan != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

	/* Sensor value in millivolts */
	voltage = data->raw * adc_ref_internal(data->adc) / 0x0FFF;
	/* considering the vbat input through a resistor bridge */
	voltage = voltage * cfg->ratio / 1000; /* value of SENSOR_CHAN_VOLTAGE in Volt */

	return sensor_value_from_double(val, voltage);
}

static const struct sensor_driver_api stm32_vbat_driver_api = {
	.sample_fetch = stm32_vbat_sample_fetch,
	.channel_get = stm32_vbat_channel_get,
};

static int stm32_vbat_init(const struct device *dev)
{
	struct stm32_vbat_data *data = dev->data;
	struct adc_sequence *asp = &data->adc_seq;

	k_mutex_init(&data->mutex);

	if (!device_is_ready(data->adc)) {
		LOG_ERR("Device %s is not ready", data->adc->name);
		return -ENODEV;
	}

	*asp = (struct adc_sequence){
		.channels = BIT(data->adc_cfg.channel_id),
		.buffer = &data->sample_buffer,
		.buffer_size = sizeof(data->sample_buffer),
		.resolution = 12U,
	};

	return 0;
}

#define STM32_VBAT_DEFINE(inst)									\
	static struct stm32_vbat_data stm32_vbat_dev_data_##inst = {				\
		.adc = DEVICE_DT_GET(DT_INST_IO_CHANNELS_CTLR(inst)),				\
		.adc_cfg = {									\
			.gain = ADC_GAIN_1,							\
			.reference = ADC_REF_INTERNAL,						\
			.acquisition_time = ADC_ACQ_TIME_MAX,					\
			.channel_id = DT_INST_IO_CHANNELS_INPUT(inst),				\
			.differential = 0,							\
		},										\
	};											\
												\
	static const struct stm32_vbat_config stm32_vbat_dev_config_##inst = {			\
		.ratio = DT_INST_PROP(inst, ratio),						\
	};											\
												\
	DEVICE_DT_INST_DEFINE(inst, stm32_vbat_init, NULL,					\
			      &stm32_vbat_dev_data_##inst, &stm32_vbat_dev_config_##inst,	\
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,				\
			      &stm32_vbat_driver_api);						\

DT_INST_FOREACH_STATUS_OKAY(STM32_VBAT_DEFINE)
