/*
 * Copyright (c) 2022 STMicroelectronics
 * Copyright (c) 2026 CodeWrights GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device_runtime.h>
#include <stm32_ll_adc.h>

LOG_MODULE_REGISTER(stm32_vddcore, CONFIG_SENSOR_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_vddcore)
#define DT_DRV_COMPAT st_stm32_vddcore
#else
#error "No compatible devicetree node found"
#endif

struct stm32_vddcore_data {
	const struct device *adc;
	const struct adc_channel_cfg adc_cfg;
	ADC_TypeDef *adc_base;
	struct adc_sequence adc_seq;
	struct k_mutex mutex;
	int16_t sample_buffer;
	int16_t raw; /* raw adc Sensor value */
};

static int stm32_vddcore_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct stm32_vddcore_data *data = dev->data;
	struct adc_sequence *sp = &data->adc_seq;
	int rc;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);
	pm_device_runtime_get(data->adc);

#ifndef CONFIG_STM32_VDDCORE_INJECTED
	rc = adc_channel_setup(data->adc, &data->adc_cfg);

	if (rc != 0) {
		LOG_DBG("Setup AIN%u got %d", data->adc_cfg.channel_id, rc);
		goto unlock;
	}

	LL_ADC_EnableChannelVDDcore(data->adc_base);
#endif /* CONFIG_STM32_VDDCORE_INJECTED */

	rc = adc_read(data->adc, sp);
	if (rc == 0) {
		data->raw = data->sample_buffer;
	}

#ifndef CONFIG_STM32_VDDCORE_INJECTED
	LL_ADC_DisableChannelVDDcore(data->adc_base);

unlock:
#endif /* CONFIG_STM32_VDDCORE_INJECTED */
	pm_device_runtime_put(data->adc);
	k_mutex_unlock(&data->mutex);

	return rc;
}

static int stm32_vddcore_channel_get(const struct device *dev, enum sensor_channel chan,
				  struct sensor_value *val)
{
	struct stm32_vddcore_data *data = dev->data;
	int32_t voltage;

	if (chan != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

	voltage = data->raw * adc_ref_internal(data->adc) / 0x0FFF;

	return sensor_value_from_milli(val, voltage);
}

static DEVICE_API(sensor, stm32_vddcore_driver_api) = {
	.sample_fetch = stm32_vddcore_sample_fetch,
	.channel_get = stm32_vddcore_channel_get,
};

static int stm32_vddcore_init(const struct device *dev)
{
	struct stm32_vddcore_data *data = dev->data;
	struct adc_sequence *asp = &data->adc_seq;

	k_mutex_init(&data->mutex);

	if (data->adc == NULL) {
		LOG_ERR("ADC is not enabled");
		return -ENODEV;
	}

	if (!device_is_ready(data->adc)) {
		LOG_ERR("Device %s is not ready", data->adc->name);
		return -ENODEV;
	}

	*asp = (struct adc_sequence){
		.channels = BIT(data->adc_cfg.channel_id),
		.buffer = &data->sample_buffer,
		.buffer_size = sizeof(data->sample_buffer),
		.resolution = 12U,
#ifdef CONFIG_STM32_VDDCORE_INJECTED
		.priority = 1,
#endif /* CONFIG_STM32_VDDCORE_INJECTED */
	};

#ifdef CONFIG_STM32_VDDCORE_INJECTED
	int rc = adc_channel_setup(data->adc, &data->adc_cfg);

	if (rc != 0) {
		LOG_DBG("Setup AIN%u got %d", data->adc_cfg.channel_id, rc);
		return rc;
	}

	LL_ADC_EnableChannelVDDcore(data->adc_base);
#endif /* CONFIG_STM32_VDDCORE_INJECTED */

	return 0;
}

#define ASSERT_VDDCORE_ADC_ENABLED(inst)                                                           \
	BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_INST_IO_CHANNELS_CTLR(inst)),                      \
		"ADC instance '" DT_NODE_FULL_NAME(DT_INST_IO_CHANNELS_CTLR(inst)) "' needed "     \
		"by Vddcore sensor '" DT_NODE_FULL_NAME(DT_DRV_INST(inst)) "' is not enabled")

#define STM32_VDDCORE_DEFINE(inst)                                                                 \
	ASSERT_VDDCORE_ADC_ENABLED(inst);                                                          \
                                                                                                   \
	static struct stm32_vddcore_data stm32_vddcore_dev_data_##inst = {                         \
		.adc = DEVICE_DT_GET(DT_INST_IO_CHANNELS_CTLR(inst)),                              \
		.adc_base = (ADC_TypeDef *)DT_REG_ADDR(DT_INST_IO_CHANNELS_CTLR(inst)),            \
		.adc_cfg = {                                                                       \
			.gain = ADC_GAIN_1,                                                        \
			.reference = ADC_REF_INTERNAL,                                             \
			.acquisition_time = ADC_ACQ_TIME_MAX,                                      \
			.channel_id = DT_INST_IO_CHANNELS_INPUT(inst),                             \
			.differential = 0,                                                         \
		},                                                                                 \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, stm32_vddcore_init, NULL,                               \
			      &stm32_vddcore_dev_data_##inst, NULL,                                \
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,                            \
			      &stm32_vddcore_driver_api);

DT_INST_FOREACH_STATUS_OKAY(STM32_VDDCORE_DEFINE)
