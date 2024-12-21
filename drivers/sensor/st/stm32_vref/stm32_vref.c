/*
 * Copyright (c) 2023 Kenneth J. Miller <ken@miller.ec>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_vref

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device_runtime.h>
#include <stm32_ll_adc.h>
#if defined(CONFIG_SOC_SERIES_STM32H5X)
#include <stm32_ll_icache.h>
#endif /* CONFIG_SOC_SERIES_STM32H5X */

LOG_MODULE_REGISTER(stm32_vref, CONFIG_SENSOR_LOG_LEVEL);

struct stm32_vref_data {
	const struct device *adc;
	const struct adc_channel_cfg adc_cfg;
	ADC_TypeDef *adc_base;
	struct adc_sequence adc_seq;
	struct k_mutex mutex;
	int16_t sample_buffer;
	int16_t raw; /* raw adc Sensor value */
};

struct stm32_vref_config {
	uint16_t *cal_addr;
	int cal_mv;
};

static int stm32_vref_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct stm32_vref_data *data = dev->data;
	struct adc_sequence *sp = &data->adc_seq;
	int rc;
	uint32_t path;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);
	pm_device_runtime_get(data->adc);

	rc = adc_channel_setup(data->adc, &data->adc_cfg);
	if (rc) {
		LOG_DBG("Setup AIN%u got %d", data->adc_cfg.channel_id, rc);
		goto unlock;
	}

	path = LL_ADC_GetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(data->adc_base));
	LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(data->adc_base),
				       LL_ADC_PATH_INTERNAL_VREFINT | path);

#ifdef LL_ADC_DELAY_VREFINT_STAB_US
	k_usleep(LL_ADC_DELAY_VREFINT_STAB_US);
#endif

	rc = adc_read(data->adc, sp);
	if (rc == 0) {
		data->raw = data->sample_buffer;
	}

	path = LL_ADC_GetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(data->adc_base));
	LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(data->adc_base),
				       path &= ~LL_ADC_PATH_INTERNAL_VREFINT);


unlock:
	pm_device_runtime_put(data->adc);
	k_mutex_unlock(&data->mutex);

	return rc;
}

static int stm32_vref_channel_get(const struct device *dev, enum sensor_channel chan,
				  struct sensor_value *val)
{
	struct stm32_vref_data *data = dev->data;
	const struct stm32_vref_config *cfg = dev->config;
	int32_t vref;

	if (chan != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

	if (data->raw == 0) {
		LOG_ERR("Raw ADC value is zero");
		return -ENODATA;
	}

/*
 * STM32H5X: accesses to flash RO region must be done with caching disabled.
 */
#if defined(CONFIG_SOC_SERIES_STM32H5X)
	LL_ICACHE_Disable();
#endif /* CONFIG_SOC_SERIES_STM32H5X */

	/* Calculate VREF+ using VREFINT bandgap voltage and calibration data */
#if defined(CONFIG_SOC_SERIES_STM32U5X)
	/*
	 * The VREF CALIBRATION value is acquired on 14 bits
	 * and the data acquired is on 12 bits
	 * since the adc_sequence.resolution is 12
	 */
	vref = (cfg->cal_mv * (*cfg->cal_addr) >> 2) / data->raw;
#else
	vref = cfg->cal_mv * (*cfg->cal_addr) / data->raw;
#endif /* CONFIG_SOC_SERIES_STM32H5X */

#if defined(CONFIG_SOC_SERIES_STM32H5X)
	LL_ICACHE_Enable();
#endif /* CONFIG_SOC_SERIES_STM32H5X */

	return sensor_value_from_milli(val, vref);
}

static DEVICE_API(sensor, stm32_vref_driver_api) = {
	.sample_fetch = stm32_vref_sample_fetch,
	.channel_get = stm32_vref_channel_get,
};

static int stm32_vref_init(const struct device *dev)
{
	struct stm32_vref_data *data = dev->data;
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

/**
 * Verify that the ADC instance which this driver uses to measure internal
 * voltage reference is enabled. On STM32 MCUs with more than one ADC, it is
 * possible to compile this driver even if the ADC used for measurement is
 * disabled. In such cases, fail build with an explicit error message.
 */
#if !DT_NODE_HAS_STATUS_OKAY(DT_INST_IO_CHANNELS_CTLR(0))

/* Use BUILD_ASSERT to get preprocessing on the message */
BUILD_ASSERT(0,	"ADC '" DT_NODE_FULL_NAME(DT_INST_IO_CHANNELS_CTLR(0)) "' needed by "
		"Vref sensor '" DT_NODE_FULL_NAME(DT_DRV_INST(0)) "' is not enabled");

/* To reduce noise in the compiler error log, do not attempt
 * to instantiate device if the sensor's ADC is not enabled.
 */
#else

static struct stm32_vref_data stm32_vref_dev_data = {
	.adc = DEVICE_DT_GET(DT_INST_IO_CHANNELS_CTLR(0)),
	.adc_base = (ADC_TypeDef *)DT_REG_ADDR(DT_INST_IO_CHANNELS_CTLR(0)),
	.adc_cfg = {.gain = ADC_GAIN_1,
		    .reference = ADC_REF_INTERNAL,
		    .acquisition_time = ADC_ACQ_TIME_MAX,
		    .channel_id = DT_INST_IO_CHANNELS_INPUT(0),
		    .differential = 0},
};

static const struct stm32_vref_config stm32_vref_dev_config = {
	.cal_addr = (uint16_t *)DT_INST_PROP(0, vrefint_cal_addr),
	.cal_mv = DT_INST_PROP(0, vrefint_cal_mv),
};

SENSOR_DEVICE_DT_INST_DEFINE(0, stm32_vref_init, NULL, &stm32_vref_dev_data, &stm32_vref_dev_config,
			     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &stm32_vref_driver_api);

#endif /* !DT_NODE_HAS_STATUS_OKAY(DT_INST_IO_CHANNELS_CTLR(0)) */
