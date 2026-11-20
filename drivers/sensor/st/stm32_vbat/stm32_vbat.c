/*
 * Copyright (c) 2022 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/dt-bindings/adc/stm32_adc.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device_runtime.h>
#include <stm32_ll_adc.h>

LOG_MODULE_REGISTER(stm32_vbat, CONFIG_SENSOR_LOG_LEVEL);

#ifdef CONFIG_STM32_HAL2
#define STM32_ADC_COMMON_INSTANCE	ADC_COMMON_INSTANCE
#else /* CONFIG_STM32_HAL2 */
#define STM32_ADC_COMMON_INSTANCE	__LL_ADC_COMMON_INSTANCE
#endif /* CONFIG_STM32_HAL2 */

struct stm32_vbat_data {
	const struct device *adc;
	const struct adc_channel_cfg adc_cfg;
	ADC_TypeDef *adc_base;
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
	pm_device_runtime_get(data->adc);

	if (!IS_ENABLED(CONFIG_STM32_VBAT_INJECTED)) {
		rc = adc_channel_setup(data->adc, &data->adc_cfg);

		if (rc != 0) {
			LOG_DBG("Setup AIN%u got %d", data->adc_cfg.channel_id, rc);
			goto unlock;
		}
	}

	rc = adc_read(data->adc, sp);
	if (rc == 0) {
		data->raw = data->sample_buffer;
	}

unlock:
	pm_device_runtime_put(data->adc);
	k_mutex_unlock(&data->mutex);

	return rc;
}

static int stm32_vbat_channel_get(const struct device *dev, enum sensor_channel chan,
				  struct sensor_value *val)
{
	struct stm32_vbat_data *data = dev->data;
	const struct stm32_vbat_config *cfg = dev->config;
	int32_t voltage;

	if (chan != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

	/* Sensor value in millivolts considering the vbat input through a resistor bridge */
	voltage = data->raw * adc_ref_internal(data->adc) * cfg->ratio / 0x0FFF;

	return sensor_value_from_milli(val, voltage);
}

static DEVICE_API(sensor, stm32_vbat_driver_api) = {
	.sample_fetch = stm32_vbat_sample_fetch,
	.channel_get = stm32_vbat_channel_get,
};

static int stm32_vbat_init(const struct device *dev)
{
	struct stm32_vbat_data *data = dev->data;
	struct adc_sequence *asp = &data->adc_seq;

	k_mutex_init(&data->mutex);

	if (data->adc == NULL) {
		LOG_ERR("ADC is not enabled");
		return -ENODEV;
	}

	if (!device_is_ready(data->adc)) {
		LOG_ERR_DEVICE_NOT_READY(data->adc);
		return -ENODEV;
	}

	*asp = (struct adc_sequence){
		.channels = BIT(data->adc_cfg.channel_id),
		.buffer = &data->sample_buffer,
		.buffer_size = sizeof(data->sample_buffer),
		.resolution = 12U,
#ifdef CONFIG_STM32_VBAT_INJECTED
		.priority = 1,
#endif /* CONFIG_STM32_VBAT_INJECTED */
	};

	if (IS_ENABLED(CONFIG_STM32_VBAT_INJECTED)) {
		int rc = adc_channel_setup(data->adc, &data->adc_cfg);

		if (rc != 0) {
			LOG_DBG("Setup AIN%u got %d", data->adc_cfg.channel_id, rc);
			return rc;
		}
	}

	return 0;
}

#define ASSERT_VBAT_ADC_ENABLED(node_id)							\
	BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_IO_CHANNELS_CTLR(node_id)),			\
		"ADC instance '" DT_NODE_FULL_NAME(DT_IO_CHANNELS_CTLR(node_id)) "' needed "	\
		"by sensor '" DT_NODE_FULL_NAME(node_id) "' is not enabled")

#define STM32_VBAT_DEFINE(node_id, ord)								\
	ASSERT_VBAT_ADC_ENABLED(node_id);							\
												\
	static struct stm32_vbat_data CONCAT(stm32_vbat_dev_data_, ord) = {			\
		.adc = DEVICE_DT_GET(DT_IO_CHANNELS_CTLR(node_id)),				\
		.adc_base = (ADC_TypeDef *)DT_REG_ADDR(DT_IO_CHANNELS_CTLR(node_id)),		\
		.adc_cfg = {									\
			.gain = ADC_GAIN_1,							\
			.reference = ADC_REF_INTERNAL,						\
			.acquisition_time = ADC_ACQ_TIME_MAX,					\
			.channel_id = DT_IO_CHANNELS_INPUT(node_id),				\
			.differential = 0,							\
			COND_CASE_1(								\
				DT_NODE_HAS_COMPAT(node_id, st_stm32_vbat),			\
				(.internal_channel = STM32_ADC_INTERNAL_CHANNEL_VBAT),		\
				DT_NODE_HAS_COMPAT(node_id, st_stm32_vddcore),			\
				(.internal_channel = STM32_ADC_INTERNAL_CHANNEL_VDDCORE),	\
				(.internal_channel = STM32_ADC_INTERNAL_CHANNEL_NONE))		\
		},										\
	};											\
												\
	static const struct stm32_vbat_config CONCAT(stm32_vbat_dev_config_, ord) = {		\
		.ratio = DT_PROP_OR(node_id, ratio, 1),						\
	};											\
												\
	SENSOR_DEVICE_DT_DEFINE(node_id, stm32_vbat_init, NULL,					\
				&CONCAT(stm32_vbat_dev_data_, ord),				\
				&CONCAT(stm32_vbat_dev_config_, ord),				\
				POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,			\
				&stm32_vbat_driver_api);

#define STM32_VBAT_FOREACH_DEFINE(node_id)							\
	STM32_VBAT_DEFINE(node_id, DT_DEP_ORD(node_id))

DT_FOREACH_STATUS_OKAY(st_stm32_vbat, STM32_VBAT_FOREACH_DEFINE)
DT_FOREACH_STATUS_OKAY(st_stm32_vddcore, STM32_VBAT_FOREACH_DEFINE)
