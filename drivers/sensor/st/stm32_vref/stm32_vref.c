/*
 * Copyright (c) 2023 Kenneth J. Miller <ken@miller.ec>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_vref

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <zephyr/nvmem.h>
#include <zephyr/pm/device_runtime.h>
#include <stm32_ll_adc.h>

LOG_MODULE_REGISTER(stm32_vref, CONFIG_SENSOR_LOG_LEVEL);

#ifdef CONFIG_STM32_HAL2
#define STM32_ADC_COMMON_INSTANCE	ADC_COMMON_INSTANCE
#else /* CONFIG_STM32_HAL2 */
#define STM32_ADC_COMMON_INSTANCE	__LL_ADC_COMMON_INSTANCE
#endif /* CONFIG_STM32_HAL2 */

/* Resolution used to perform the Vref measurement */
#define MEAS_RES	(12U)

struct stm32_vref_data {
	struct adc_sequence adc_seq;
	struct k_mutex mutex;
	/*
	 * Numerator for VREFINT calculation
	 * (VREFINT = Num / <ADC read data>)
	 */
	int32_t vrefint_cal_numerator;
	int16_t sample_buffer;
	int16_t raw; /* raw adc Sensor value */
};

#if defined(CONFIG_STM32_VREF_READ_CALIB_VIA_NVMEM)
typedef struct nvmem_cell calib_info_t;
#else
typedef const uint16_t *calib_info_t;
#endif /* CONFIG_STM32_VREF_READ_CALIB_VIA_NVMEM */

struct stm32_vref_config {
	const struct device *adc;
	struct adc_channel_cfg adc_cfg;
	ADC_TypeDef *adc_base;

	calib_info_t vrefint_cal;
	int cal_mv;
	uint8_t cal_shift;
};

static void stm32_vref_enable_vrefsensor_channel(ADC_TypeDef *adc)
{
	const uint32_t path = LL_ADC_GetCommonPathInternalCh(STM32_ADC_COMMON_INSTANCE(adc));

	LL_ADC_SetCommonPathInternalCh(STM32_ADC_COMMON_INSTANCE(adc),
					path | LL_ADC_PATH_INTERNAL_VREFINT);

#ifdef LL_ADC_DELAY_VREFINT_STAB_US
	k_usleep(LL_ADC_DELAY_VREFINT_STAB_US);
#endif
}

__maybe_unused static void stm32_vref_disable_vrefsensor_channel(ADC_TypeDef *adc)
{
	const uint32_t path = LL_ADC_GetCommonPathInternalCh(STM32_ADC_COMMON_INSTANCE(adc));

	LL_ADC_SetCommonPathInternalCh(STM32_ADC_COMMON_INSTANCE(adc),
					path & ~LL_ADC_PATH_INTERNAL_VREFINT);
}

static int stm32_vref_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct stm32_vref_config *cfg = dev->config;
	struct stm32_vref_data *data = dev->data;
	struct adc_sequence *sp = &data->adc_seq;
	int rc;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);
	pm_device_runtime_get(cfg->adc);

#ifndef CONFIG_STM32_VREF_INJECTED
	rc = adc_channel_setup(cfg->adc, &cfg->adc_cfg);
	if (rc) {
		LOG_DBG("Setup AIN%u got %d", cfg->adc_cfg.channel_id, rc);
		goto unlock;
	}

	stm32_vref_enable_vrefsensor_channel(cfg->adc_base);
#endif /* CONFIG_STM32_VREF_INJECTED */

	rc = adc_read(cfg->adc, sp);
	if (rc == 0) {
		data->raw = data->sample_buffer;
	}

#ifndef CONFIG_STM32_VREF_INJECTED
	stm32_vref_disable_vrefsensor_channel(cfg->adc_base);

unlock:
#endif /* CONFIG_STM32_VREF_INJECTED */
	pm_device_runtime_put(cfg->adc);
	k_mutex_unlock(&data->mutex);

	return rc;
}

static int stm32_vref_channel_get(const struct device *dev, enum sensor_channel chan,
				  struct sensor_value *val)
{
	struct stm32_vref_data *data = dev->data;
	int32_t vref;

	if (chan != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

	if (data->raw == 0) {
		LOG_ERR("Raw ADC value is zero");
		return -ENODATA;
	}

	/* Calculate VREF+ using VREFINT bandgap voltage and calibration data */
	vref = data->vrefint_cal_numerator / data->raw;

	return sensor_value_from_milli(val, vref);
}

static DEVICE_API(sensor, stm32_vref_driver_api) = {
	.sample_fetch = stm32_vref_sample_fetch,
	.channel_get = stm32_vref_channel_get,
};

static int stm32_vref_init(const struct device *dev)
{
	const struct stm32_vref_config *cfg = dev->config;
	struct stm32_vref_data *data = dev->data;
	struct adc_sequence *asp = &data->adc_seq;

	k_mutex_init(&data->mutex);

	if (!device_is_ready(cfg->adc)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->adc);
		return -ENODEV;
	}

	*asp = (struct adc_sequence){
		.channels = BIT(cfg->adc_cfg.channel_id),
		.buffer = &data->sample_buffer,
		.buffer_size = sizeof(data->sample_buffer),
		.resolution = MEAS_RES,
#ifdef CONFIG_STM32_VREF_INJECTED
		.priority = 1,
#endif /* CONFIG_STM32_VREF_INJECTED */
	};

/*
 * Read calibration data and compute numerator once during init.
 * STM32H5X: accesses to flash RO region must be done with caching disabled.
 */
	uint16_t vrefint_cal;
#if defined(CONFIG_STM32_VREF_READ_CALIB_VIA_NVMEM)
	int res;

	res = nvmem_cell_read(&cfg->vrefint_cal, &vrefint_cal, 0, sizeof(vrefint_cal));
	if (res < 0) {
		LOG_ERR("Failed to read VREFINT calibration data: %d", res);
		return res;
	}
#else /* CONFIG_STM32_VREF_READ_CALIB_VIA_NVMEM */
	if (IS_ENABLED(CONFIG_HAS_STM32_UNCACHED_ACCESS_ONLY_OTP)) {
		sys_cache_instr_disable();
	}

	vrefint_cal = *cfg->vrefint_cal;

	if (IS_ENABLED(CONFIG_HAS_STM32_UNCACHED_ACCESS_ONLY_OTP)) {
		sys_cache_instr_enable();
	}
#endif /* CONFIG_STM32_VREF_READ_CALIB_VIA_NVMEM */

	data->vrefint_cal_numerator = cfg->cal_mv * (vrefint_cal >> cfg->cal_shift);

#ifdef CONFIG_STM32_VREF_INJECTED
	int rc = adc_channel_setup(cfg->adc, &cfg->adc_cfg);

	if (rc != 0) {
		LOG_DBG("Setup AIN%u got %d", cfg->adc_cfg.channel_id, rc);
		return rc;
	}

	stm32_vref_enable_vrefsensor_channel(cfg->adc_base);
#endif /* CONFIG_STM32_VREF_INJECTED */

	return 0;
}

#if defined(CONFIG_STM32_VREF_READ_CALIB_VIA_NVMEM)
#define VREFINT_CAL_INIT(inst) NVMEM_CELL_INST_GET_BY_IDX(inst, 0)
#else
/* MMIO address = base of OTP flash area + offset of cell in OTP */
#define VREFINT_CAL_INIT_INNER(nvmc)							\
	((void *)(DT_REG_ADDR(DT_MTD_FROM_NVMEM_CELL(nvmc)) + DT_REG_ADDR(nvmc)))
#define VREFINT_CAL_INIT(inst)							\
	VREFINT_CAL_INIT_INNER(DT_INST_NVMEM_CELL(inst))
#endif /* CONFIG_STM32_VREF_READ_CALIB_VIA_NVMEM */

#define STM32_VREF_DEFINE(inst)									\
	BUILD_ASSERT(DT_INST_PROP(inst, vrefint_cal_resolution) >= MEAS_RES,			\
		"VREFINT calibration resolution is too low");					\
												\
	static struct stm32_vref_data stm32_vref_dev_data_##inst;				\
												\
	static const struct stm32_vref_config stm32_vref_dev_config_##inst = {			\
		.adc = DEVICE_DT_GET(DT_INST_IO_CHANNELS_CTLR(inst)),				\
		.adc_base = (ADC_TypeDef *)DT_REG_ADDR(DT_INST_IO_CHANNELS_CTLR(inst)),		\
		.adc_cfg = {.gain = ADC_GAIN_1,							\
			    .reference = ADC_REF_INTERNAL,					\
			    .acquisition_time = ADC_ACQ_TIME_MAX,				\
			    .channel_id = DT_INST_IO_CHANNELS_INPUT(inst),			\
			    .differential = 0},							\
												\
		.vrefint_cal = VREFINT_CAL_INIT(inst),						\
		.cal_mv = DT_INST_PROP(inst, vrefint_cal_mv),					\
		.cal_shift = (DT_INST_PROP(inst, vrefint_cal_resolution) - MEAS_RES),		\
	};											\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, stm32_vref_init, NULL,				\
				     &stm32_vref_dev_data_##inst,				\
				     &stm32_vref_dev_config_##inst,				\
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,			\
				     &stm32_vref_driver_api);

/*
 * Wrapper which prevents expansion of STM32_VREF_DEFINE() if the corresponding ADC
 * is not enabled to reduce noise in the compiler error, ensuring only a clear error
 * message is shown instead.
 */
#define STM32_VREF_DEFINE_OR_BUILD_ERROR(inst)							\
	COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(DT_INST_IO_CHANNELS_CTLR(inst)),			\
		(STM32_VREF_DEFINE(inst)),							\
		(BUILD_ASSERT(0,								\
			"ADC '" DT_NODE_FULL_NAME(DT_INST_IO_CHANNELS_CTLR(inst)) "' needed by "\
			"Vref sensor '" DT_NODE_FULL_NAME(DT_DRV_INST(inst)) "' is not enabled"	\
		);))

DT_INST_FOREACH_STATUS_OKAY(STM32_VREF_DEFINE_OR_BUILD_ERROR)
