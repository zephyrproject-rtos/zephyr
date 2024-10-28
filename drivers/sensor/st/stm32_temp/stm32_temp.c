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

#define CAL_RES			12
#define MAX_CALIB_POINTS	2

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
	float average_slope;		/** Unit: mV/°C */
	int v25;			/** Unit: mV */
#else /* HAS_CALIBRATION */
	unsigned int calib_vrefanalog;	/** Unit: mV */
	unsigned int calib_data_shift;
	const void *ts_cal1_addr;
	int ts_cal1_temp;		/** Unit: °C */
#if defined(HAS_SINGLE_CALIBRATION)
	float average_slope;		/** Unit: mV/°C */
#else /* HAS_DUAL_CALIBRATION */
	const void *ts_cal2_addr;
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

#if defined(HAS_CALIBRATION)
static uint32_t fetch_mfg_data(const void *addr)
{
	/* On all STM32 series, the calibration data is stored
	 * as 16-bit data in the manufacturing flash region
	 */
	return sys_read16((mem_addr_t)addr);
}

/**
 * @returns TS_CAL1 in calib_data[0]
 *          TS_CAL2 in calib_data[1] if applicable
 */
static void read_calibration_data(const struct stm32_temp_config *cfg,
				uint32_t calib_data[MAX_CALIB_POINTS])
{
#if defined(CONFIG_SOC_SERIES_STM32H5X)
	/* Disable the ICACHE to ensure all memory accesses are non-cacheable.
	 * This is required on STM32H5, where the manufacturing flash must be
	 * accessed in non-cacheable mode - otherwise, a bus error occurs.
	 */
	LL_ICACHE_Disable();
#endif /* CONFIG_SOC_SERIES_STM32H5X */

	calib_data[0] = fetch_mfg_data(cfg->ts_cal1_addr);
#if defined(HAS_DUAL_CALIBRATION)
	calib_data[1] = fetch_mfg_data(cfg->ts_cal2_addr);
#endif


#if defined(CONFIG_SOC_SERIES_STM32H5X)
	/* Re-enable the ICACHE (unconditonally - it should always be turned on) */
	LL_ICACHE_Enable();
#endif /* CONFIG_SOC_SERIES_STM32H5X */
}
#endif /* HAS_CALIBRATION */

static float convert_adc_sample_to_temperature(const struct device *dev)
{
	struct stm32_temp_data *data = dev->data;
	const struct stm32_temp_config *cfg = dev->config;
	const uint16_t vdda_mv = adc_ref_internal(data->adc);
	float temperature;

#if !defined(HAS_CALIBRATION)
	/**
	 * Series without calibration (STM32F1/F2):
	 *   Tjunction = ((Dividend) / Avg_Slope) + 25
	 *
	 *  where Dividend is:
	 *   - (V25 - Vsense) on STM32F1 series ("ntc")
	 *   - (Vsense - V25) on STM32F2 series
	 *  and Vsense = (ADC raw data) / ADC_MAX_VALUE * Vdda
	 *  and ADC_MAX_VALUE = 4095 (12-bit ADC resolution)
	 *
	 * References:
	 *  - RM0008 §11.10 "Temperature sensor" (STM32F100)
	 *  - RM0041 §10.9  "Temperature sensor" (STM32F101/F102/F103/F105/F107)
	 *  - RM0033 §10.10 "Temperature sensor" (STM32F2)
	 */
	/* Perform multiplication first for higher accuracy */
	const int vsense = ((int)data->raw * vdda_mv) / 4095;

	if (cfg->is_ntc) {
		temperature = (float)(cfg->v25 - vsense);
	} else {
		temperature = (float)(vsense - cfg->v25);
	}
	temperature /= cfg->average_slope;
	temperature += 25.0f;
#else /* HAS_CALIBRATION */
	uint32_t calib[MAX_CALIB_POINTS];

	read_calibration_data(cfg, calib);

	const float sense_data = ((float)vdda_mv / cfg->calib_vrefanalog) * data->raw;

#if defined(HAS_SINGLE_CALIBRATION)
	/**
	 * Series with one calibration point (STM32C0,STM32F030/F070):
	 *  Tjunction = ((Dividend) / Avg_Slope_Code) + TS_CAL1_TEMP
	 *
	 *  where Dividend is:
	 *   - (TS_CAL1 - Sense_Data) on STM32F030/STM32F070 ("ntc")
	 *   - (Sense_Data - TS_CAL1) on STM32C0 series
	 *
	 *  and Avg_SlopeCode = (Avg_Slope * 4096 / calibration Vdda)
	 *
	 * References:
	 *  - RM0360 §12.8  "Temperature sensor" (STM32F030/STM32F070)
	 *  - RM0490 §14.10 "Temperature sensor and internal reference voltage" (STM32C0)
	 */
	const float avg_slope_code =
		(cfg->average_slope / cfg->calib_vrefanalog) * 4096.f;
	float dividend;

	if (cfg->is_ntc) {
		dividend = ((float)(calib[0] >> cfg->calib_data_shift) - sense_data);
	} else {
		dividend = (sense_data - (calib[0] >> cfg->calib_data_shift));
	}

	temperature = (dividend / avg_slope_code) + cfg->ts_cal1_temp;
#else /* HAS_DUAL_CALIBRATION */
	/**
	 * Series with two calibration points:
	 *  Tjunction = (Slope * (Sense_Data - TS_CAL1)) + TS_CAL1_TEMP
	 *
	 *                 (TS_CAL2_TEMP - TS_CAL1_TEMP)
	 *  where Slope =  -----------------------------
	 *                      (TS_CAL2 - TS_CAL1)
	 */
	const float slope = ((float)(cfg->ts_cal2_temp - cfg->ts_cal1_temp))
					/ ((calib[1] - calib[0]) >> cfg->calib_data_shift);

	temperature = (slope * (sense_data - (calib[0] >> cfg->calib_data_shift)))
			+ cfg->ts_cal1_temp;
#endif /* HAS_SINGLE_CALIBRATION */
#endif /* HAS_CALIBRATION */

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
#if !DT_NODE_HAS_STATUS_OKAY(DT_INST_IO_CHANNELS_CTLR(0))

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
	.ts_cal1_addr = (const void *)DT_INST_PROP(0, ts_cal1_addr),
	.ts_cal1_temp = DT_INST_PROP(0, ts_cal1_temp),
#if defined(HAS_SINGLE_CALIBRATION)
	.average_slope = ((float)DT_INST_STRING_UNQUOTED(0, avgslope)),
#else /* HAS_DUAL_CALIBRATION */
	.ts_cal2_addr = (const void *)DT_INST_PROP(0, ts_cal2_addr),
	.ts_cal2_temp = DT_INST_PROP(0, ts_cal2_temp),
#endif
	.calib_data_shift = (DT_INST_PROP(0, ts_cal_resolution) - CAL_RES),
	.calib_vrefanalog = DT_INST_PROP(0, ts_cal_vrefanalog),
#else
	.average_slope = ((float)DT_INST_STRING_UNQUOTED(0, avgslope)),
	.v25 = DT_INST_PROP(0, v25),
#endif
	.is_ntc = DT_INST_PROP_OR(0, ntc, false)
};

SENSOR_DEVICE_DT_INST_DEFINE(0, stm32_temp_init, NULL,
			     &stm32_temp_dev_data, &stm32_temp_dev_config,
			     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
			     &stm32_temp_driver_api);

#endif /* !DT_NODE_HAS_STATUS_OKAY(DT_INST_IO_CHANNELS_CTLR(0)) */
