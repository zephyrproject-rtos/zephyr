/*
 * Copyright (c) 2021 Eug Krashtan
 * Copyright (c) 2022 Wouter Cappelle
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
#if defined(CONFIG_SOC_SERIES_STM32H5X)
#include <stm32_ll_icache.h>
#endif /* CONFIG_SOC_SERIES_STM32H5X */

LOG_MODULE_REGISTER(stm32_temp, CONFIG_SENSOR_LOG_LEVEL);
#define CAL_RES 12
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_temp)
#define DT_DRV_COMPAT st_stm32_temp
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_temp_cal)
#define DT_DRV_COMPAT st_stm32_temp_cal
#define HAS_DUAL_CALIBRATION 1
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32c0_temp_cal)
#define DT_DRV_COMPAT st_stm32c0_temp_cal
#define HAS_SINGLE_CALIBRATION 1
#else
#error "No compatible devicetree node found"
#endif

#if defined(HAS_SINGLE_CALIBRATION) || defined(HAS_DUAL_CALIBRATION)
#define HAS_CALIBRATION 1
#endif

struct stm32_temp_data {
	const struct device *adc;
	const struct adc_channel_cfg adc_cfg;
	ADC_TypeDef *adc_base;
	struct adc_sequence adc_seq;
	struct k_mutex mutex;
	int16_t sample_buffer;
	int16_t raw; /* raw adc Sensor value */
};

struct stm32_temp_config {
#if !defined(HAS_CALIBRATION)
	int average_slope;		/** Unit: mV/°C x10 */
	int v25;			/** Unit: mV */
#else /* HAS_CALIBRATION */
	unsigned int calib_vrefanalog;	/** Unit: mV */
	unsigned int calib_data_shift;
	const uint16_t *ts_cal1_addr;
	int ts_cal1_temp;		/** Unit: °C */
#if defined(HAS_SINGLE_CALIBRATION)
	int average_slope;		/** Unit: µV/°C */
#else /* HAS_DUAL_CALIBRATION */
	const uint16_t *ts_cal2_addr;
	int ts_cal2_temp;		/** Unit: °C */
#endif
#endif /* HAS_CALIBRATION */
	bool is_ntc;
};

static inline void adc_enable_tempsensor_channel(ADC_TypeDef *adc)
{
	const uint32_t path = LL_ADC_GetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(adc));

	LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(adc),
					path | LL_ADC_PATH_INTERNAL_TEMPSENSOR);

	k_usleep(LL_ADC_DELAY_TEMPSENSOR_STAB_US);
}

static inline void adc_disable_tempsensor_channel(ADC_TypeDef *adc)
{
	const uint32_t path = LL_ADC_GetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(adc));

	LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(adc),
					path & ~LL_ADC_PATH_INTERNAL_TEMPSENSOR);
}

static float convert_adc_sample_to_temperature(const struct device *dev)
{
	struct stm32_temp_data *data = dev->data;
	const struct stm32_temp_config *cfg = dev->config;
	float temperature;

#if defined(HAS_CALIBRATION)

#if defined(CONFIG_SOC_SERIES_STM32H5X)
	/* Disable the ICACHE to ensure all memory accesses are non-cacheable.
	 * This is required on STM32H5, where the manufacturing flash must be
	 * accessed in non-cacheable mode - otherwise, a bus error occurs.
	 */
	LL_ICACHE_Disable();
#endif /* CONFIG_SOC_SERIES_STM32H5X */

	temperature = ((float)data->raw * adc_ref_internal(data->adc)) / cfg->calib_vrefanalog;
	temperature -= (*cfg->ts_cal1_addr >> cfg->calib_data_shift);
#if defined(HAS_SINGLE_CALIBRATION)
	if (cfg->is_ntc) {
		temperature = -temperature;
	}
	temperature /= (cfg->average_slope * 4096) / (cfg->calib_vrefanalog * 1000);
#else
	temperature *= (cfg->ts_cal2_temp - cfg->ts_cal1_temp);
	temperature /= ((*cfg->ts_cal2_addr - *cfg->ts_cal1_addr) >> cfg->calib_data_shift);
#endif
	temperature += cfg->ts_cal1_temp;

#if defined(CONFIG_SOC_SERIES_STM32H5X)
	/* Re-enable the ICACHE (unconditonally, as it should always be on) */
	LL_ICACHE_Enable();
#endif /* CONFIG_SOC_SERIES_STM32H5X */

#else
	/* Sensor value in millivolts */
	int32_t mv = data->raw * adc_ref_internal(data->adc) / 0x0FFF;

	if (cfg->is_ntc) {
		temperature = (float)(cfg->v25 - mv);
	} else {
		temperature = (float)(mv - cfg->v25);
	}
	temperature = (temperature / cfg->average_slope) * 10;
	temperature += 25;
#endif

	return temperature;
}

static int stm32_temp_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct stm32_temp_data *data = dev->data;
	struct adc_sequence *sp = &data->adc_seq;
	int rc;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);
	pm_device_runtime_get(data->adc);

	rc = adc_channel_setup(data->adc, &data->adc_cfg);
	if (rc) {
		LOG_DBG("Setup AIN%u got %d", data->adc_cfg.channel_id, rc);
		goto unlock;
	}

	adc_enable_tempsensor_channel(data->adc_base);

	rc = adc_read(data->adc, sp);
	if (rc == 0) {
		data->raw = data->sample_buffer;
	}

	adc_disable_tempsensor_channel(data->adc_base);

unlock:
	pm_device_runtime_put(data->adc);
	k_mutex_unlock(&data->mutex);

	return rc;
}

static int stm32_temp_channel_get(const struct device *dev, enum sensor_channel chan,
				  struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	const float temp = convert_adc_sample_to_temperature(dev);

	return sensor_value_from_float(val, temp);
}

static const struct sensor_driver_api stm32_temp_driver_api = {
	.sample_fetch = stm32_temp_sample_fetch,
	.channel_get = stm32_temp_channel_get,
};

static int stm32_temp_init(const struct device *dev)
{
	struct stm32_temp_data *data = dev->data;
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
 * Verify that the ADC instance which this driver uses to measure temperature
 * is enabled. On STM32 MCUs with more than one ADC, it is possible to compile
 * this driver even if the ADC used for measurement is disabled. In such cases,
 * fail build with an explicit error message.
 */
#if !DT_NODE_HAS_STATUS(DT_INST_IO_CHANNELS_CTLR(0), okay)

/* Use BUILD_ASSERT to get preprocessing on the message */
BUILD_ASSERT(0,	"ADC '" DT_NODE_FULL_NAME(DT_INST_IO_CHANNELS_CTLR(0)) "' needed by "
		"temperature sensor '" DT_NODE_FULL_NAME(DT_DRV_INST(0)) "' is not enabled");

/* To reduce noise in the compiler error log, do not attempt
 * to instantiate device if the sensor's ADC is not enabled.
 */
#else

static struct stm32_temp_data stm32_temp_dev_data = {
	.adc = DEVICE_DT_GET(DT_INST_IO_CHANNELS_CTLR(0)),
	.adc_base = (ADC_TypeDef *)DT_REG_ADDR(DT_INST_IO_CHANNELS_CTLR(0)),
	.adc_cfg = {
		.gain = ADC_GAIN_1,
		.reference = ADC_REF_INTERNAL,
		.acquisition_time = ADC_ACQ_TIME_MAX,
		.channel_id = DT_INST_IO_CHANNELS_INPUT(0),
		.differential = 0
	},
};

static const struct stm32_temp_config stm32_temp_dev_config = {
#if defined(HAS_CALIBRATION)
	.ts_cal1_addr = (uint16_t *)DT_INST_PROP(0, ts_cal1_addr),
	.ts_cal1_temp = DT_INST_PROP(0, ts_cal1_temp),
#if defined(HAS_SINGLE_CALIBRATION)
	.average_slope = DT_INST_PROP(0, avgslope),
#else /* HAS_DUAL_CALIBRATION */
	.ts_cal2_addr = (uint16_t *)DT_INST_PROP(0, ts_cal2_addr),
	.ts_cal2_temp = DT_INST_PROP(0, ts_cal2_temp),
#endif
	.calib_data_shift = (DT_INST_PROP(0, ts_cal_resolution) - CAL_RES),
	.calib_vrefanalog = DT_INST_PROP(0, ts_cal_vrefanalog),
#else
	.average_slope = DT_INST_PROP(0, avgslope),
	.v25 = DT_INST_PROP(0, v25),
#endif
	.is_ntc = DT_INST_PROP_OR(0, ntc, false)
};

SENSOR_DEVICE_DT_INST_DEFINE(0, stm32_temp_init, NULL,
			     &stm32_temp_dev_data, &stm32_temp_dev_config,
			     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
			     &stm32_temp_driver_api);

#endif /* !DT_NODE_HAS_STATUS(DT_INST_IO_CHANNELS_CTLR(0), okay) */
