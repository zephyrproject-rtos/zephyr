/*
 * Copyright (c) 2018 Kokoon Technology Limited
 * Copyright (c) 2019 Song Qiang <songqiang1304521@gmail.com>
 * Copyright (c) 2019 Endre Karlson
 * Copyright (c) 2020 Teslabs Engineering S.L.
 * Copyright (c) 2021 Marius Scholtz, RIC Electronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_adc

#include <errno.h>

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <soc.h>
#include <stm32_ll_adc.h>
#if defined(CONFIG_SOC_SERIES_STM32U5X)
#include <stm32_ll_pwr.h>
#endif /* CONFIG_SOC_SERIES_STM32U5X */

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_stm32);

#include <zephyr/drivers/clock_control/stm32_clock_control.h>

#if defined(CONFIG_SOC_SERIES_STM32F3X)
#if defined(ADC1_V2_5)
#define STM32F3X_ADC_V2_5
#elif defined(ADC5_V1_1)
#define STM32F3X_ADC_V1_1
#endif
#endif

/* reference voltage for the ADC */
#define STM32_ADC_VREF_MV DT_INST_PROP(0, vref_mv)

#if !defined(CONFIG_SOC_SERIES_STM32F0X) && \
	!defined(CONFIG_SOC_SERIES_STM32G0X) && \
	!defined(CONFIG_SOC_SERIES_STM32L0X) && \
	!defined(CONFIG_SOC_SERIES_STM32WLX)
#define RANK(n)		LL_ADC_REG_RANK_##n
static const uint32_t table_rank[] = {
	RANK(1),
	RANK(2),
	RANK(3),
	RANK(4),
	RANK(5),
	RANK(6),
	RANK(7),
	RANK(8),
	RANK(9),
	RANK(10),
	RANK(11),
	RANK(12),
	RANK(13),
	RANK(14),
	RANK(15),
	RANK(16),
};

#define SEQ_LEN(n)	LL_ADC_REG_SEQ_SCAN_ENABLE_##n##RANKS
static const uint32_t table_seq_len[] = {
	LL_ADC_REG_SEQ_SCAN_DISABLE,
	SEQ_LEN(2),
	SEQ_LEN(3),
	SEQ_LEN(4),
	SEQ_LEN(5),
	SEQ_LEN(6),
	SEQ_LEN(7),
	SEQ_LEN(8),
	SEQ_LEN(9),
	SEQ_LEN(10),
	SEQ_LEN(11),
	SEQ_LEN(12),
	SEQ_LEN(13),
	SEQ_LEN(14),
	SEQ_LEN(15),
	SEQ_LEN(16),
};
#endif

#define RES(n)		LL_ADC_RESOLUTION_##n##B
static const uint32_t table_resolution[] = {
#if defined(CONFIG_SOC_SERIES_STM32F1X) || \
	defined(STM32F3X_ADC_V2_5)
	RES(12),
#elif defined(CONFIG_SOC_SERIES_STM32U5X)
	RES(8),
	RES(10),
	RES(12),
	RES(14),
#elif !defined(CONFIG_SOC_SERIES_STM32H7X)
	RES(6),
	RES(8),
	RES(10),
	RES(12),
#else
	RES(8),
	RES(10),
	RES(12),
	RES(14),
	RES(16),
#endif
};

#define SMP_TIME(x, y)		LL_ADC_SAMPLINGTIME_##x##CYCLE##y

/*
 * Conversion time in ADC cycles. Many values should have been 0.5 less,
 * but the adc api system currently does not support describing 'half cycles'.
 * So all half cycles are counted as one.
 */
#if defined(CONFIG_SOC_SERIES_STM32F0X) || defined(CONFIG_SOC_SERIES_STM32F1X)
static const uint16_t acq_time_tbl[8] = {2, 8, 14, 29, 42, 56, 72, 240};
static const uint32_t table_samp_time[] = {
	SMP_TIME(1,   _5),
	SMP_TIME(7,   S_5),
	SMP_TIME(13,  S_5),
	SMP_TIME(28,  S_5),
	SMP_TIME(41,  S_5),
	SMP_TIME(55,  S_5),
	SMP_TIME(71,  S_5),
	SMP_TIME(239, S_5),
};
#elif defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X)
static const uint16_t acq_time_tbl[8] = {3, 15, 28, 56, 84, 112, 144, 480};
static const uint32_t table_samp_time[] = {
	SMP_TIME(3,   S),
	SMP_TIME(15,  S),
	SMP_TIME(28,  S),
	SMP_TIME(56,  S),
	SMP_TIME(84,  S),
	SMP_TIME(112, S),
	SMP_TIME(144, S),
	SMP_TIME(480, S),
};
#elif defined(CONFIG_SOC_SERIES_STM32F3X)
#ifdef ADC5_V1_1
static const uint16_t acq_time_tbl[8] = {2, 3, 5, 8, 20, 62, 182, 602};
static const uint32_t table_samp_time[] = {
	SMP_TIME(1,   _5),
	SMP_TIME(2,   S_5),
	SMP_TIME(4,   S_5),
	SMP_TIME(7,   S_5),
	SMP_TIME(19,  S_5),
	SMP_TIME(61,  S_5),
	SMP_TIME(181, S_5),
	SMP_TIME(601, S_5),
};
#else
static const uint16_t acq_time_tbl[8] = {2, 8, 14, 29, 42, 56, 72, 240};
static const uint32_t table_samp_time[] = {
	SMP_TIME(1,   _5),
	SMP_TIME(7,   S_5),
	SMP_TIME(13,  S_5),
	SMP_TIME(28,  S_5),
	SMP_TIME(41,  S_5),
	SMP_TIME(55,  S_5),
	SMP_TIME(71,  S_5),
	SMP_TIME(239, S_5),
};
#endif /* ADC5_V1_1 */
#elif defined(CONFIG_SOC_SERIES_STM32L0X) || \
	defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32WLX)
static const uint16_t acq_time_tbl[8] = {2, 4, 8, 13, 20, 40, 80, 161};
static const uint32_t table_samp_time[] = {
	SMP_TIME(1,   _5),
	SMP_TIME(3,   S_5),
	SMP_TIME(7,   S_5),
	SMP_TIME(12,  S_5),
	SMP_TIME(19,  S_5),
	SMP_TIME(39,  S_5),
	SMP_TIME(79,  S_5),
	SMP_TIME(160, S_5),
};
#elif defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32G4X)
static const uint16_t acq_time_tbl[8] = {3, 7, 13, 25, 48, 93, 248, 641};
static const uint32_t table_samp_time[] = {
	SMP_TIME(2,   S_5),
	SMP_TIME(6,   S_5),
	SMP_TIME(12,  S_5),
	SMP_TIME(24,  S_5),
	SMP_TIME(47,  S_5),
	SMP_TIME(92,  S_5),
	SMP_TIME(247, S_5),
	SMP_TIME(640, S_5),
};
#elif defined(CONFIG_SOC_SERIES_STM32L1X)
static const uint16_t acq_time_tbl[8] = {5, 10, 17, 25, 49, 97, 193, 385};
static const uint32_t table_samp_time[] = {
	SMP_TIME(4,   S),
	SMP_TIME(9,   S),
	SMP_TIME(16,  S),
	SMP_TIME(24,  S),
	SMP_TIME(48,  S),
	SMP_TIME(96,  S),
	SMP_TIME(192, S),
	SMP_TIME(384, S),
};
#elif defined(CONFIG_SOC_SERIES_STM32H7X)
static const uint16_t acq_time_tbl[8] = {2, 3, 9, 17, 33, 65, 388, 811};
static const uint32_t table_samp_time[] = {
	SMP_TIME(1,   _5),
	SMP_TIME(2,   S_5),
	SMP_TIME(8,   S_5),
	SMP_TIME(16,  S_5),
	SMP_TIME(32,  S_5),
	SMP_TIME(64,  S_5),
	SMP_TIME(387, S_5),
	SMP_TIME(810, S_5),
};
#elif defined(CONFIG_SOC_SERIES_STM32U5X)
static const uint16_t acq_time_tbl[8] = {5, 6, 12, 20, 36, 68, 391, 814};
static const uint32_t table_samp_time[] = {
	SMP_TIME(5,),
	SMP_TIME(6,   S),
	SMP_TIME(12,  S),
	SMP_TIME(20,  S),
	SMP_TIME(36,  S),
	SMP_TIME(68,  S),
	SMP_TIME(391, S_5),
	SMP_TIME(814, S),
};
#endif

/* External channels (maximum). */
#define STM32_CHANNEL_COUNT		20

struct adc_stm32_data {
	struct adc_context ctx;
	const struct device *dev;
	uint16_t *buffer;
	uint16_t *repeat_buffer;

	uint8_t resolution;
	uint8_t channel_count;
#if defined(CONFIG_SOC_SERIES_STM32F0X) || \
	defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32L0X)
	int8_t acq_time_index;
#endif
};

struct adc_stm32_cfg {
	ADC_TypeDef *base;
	void (*irq_cfg_func)(void);
	struct stm32_pclken pclken;
	const struct pinctrl_dev_config *pcfg;
	bool has_temp_channel;
	bool has_vref_channel;
	bool has_vbat_channel;
};

#ifdef CONFIG_ADC_STM32_SHARED_IRQS
static bool init_irq = true;
#endif

static int check_buffer_size(const struct adc_sequence *sequence,
			     uint8_t active_channels)
{
	size_t needed_buffer_size;

	needed_buffer_size = active_channels * sizeof(uint16_t);

	if (sequence->options) {
		needed_buffer_size *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed_buffer_size) {
		LOG_ERR("Provided buffer is too small (%u/%u)",
				sequence->buffer_size, needed_buffer_size);
		return -ENOMEM;
	}

	return 0;
}

static void adc_stm32_start_conversion(const struct device *dev)
{
	const struct adc_stm32_cfg *config = dev->config;
	ADC_TypeDef *adc = (ADC_TypeDef *)config->base;

	LOG_DBG("Starting conversion");

#if defined(CONFIG_SOC_SERIES_STM32F0X) || \
	defined(CONFIG_SOC_SERIES_STM32F3X) || \
	defined(CONFIG_SOC_SERIES_STM32L0X) || \
	defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32G4X) || \
	defined(CONFIG_SOC_SERIES_STM32H7X) || \
	defined(CONFIG_SOC_SERIES_STM32U5X) || \
	defined(CONFIG_SOC_SERIES_STM32WLX)
	LL_ADC_REG_StartConversion(adc);
#else
	LL_ADC_REG_StartConversionSWStart(adc);
#endif
}

#if !defined(CONFIG_SOC_SERIES_STM32F2X) && \
	!defined(CONFIG_SOC_SERIES_STM32F4X) && \
	!defined(CONFIG_SOC_SERIES_STM32F7X) && \
	!defined(CONFIG_SOC_SERIES_STM32F1X) && \
	!defined(STM32F3X_ADC_V2_5) && \
	!defined(CONFIG_SOC_SERIES_STM32L1X)
static void adc_stm32_calib(const struct device *dev)
{
	const struct adc_stm32_cfg *config =
		(const struct adc_stm32_cfg *)dev->config;
	ADC_TypeDef *adc = config->base;

#if defined(STM32F3X_ADC_V1_1) || \
	defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32G4X)
	LL_ADC_StartCalibration(adc, LL_ADC_SINGLE_ENDED);
#elif defined(CONFIG_SOC_SERIES_STM32F0X) || \
	defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32L0X) || \
	defined(CONFIG_SOC_SERIES_STM32WLX)
	LL_ADC_StartCalibration(adc);
#elif defined(CONFIG_SOC_SERIES_STM32U5X)
	LL_ADC_StartCalibration(adc, LL_ADC_CALIB_OFFSET);
#elif defined(CONFIG_SOC_SERIES_STM32H7X)
	LL_ADC_StartCalibration(adc, LL_ADC_CALIB_OFFSET, LL_ADC_SINGLE_ENDED);
#endif
	while (LL_ADC_IsCalibrationOnGoing(adc)) {
	}
}
#endif

/*
 * Disable ADC peripheral, and wait until it is disabled
 */
static inline void adc_stm32_disable(ADC_TypeDef *adc)
{
	if (LL_ADC_IsEnabled(adc) != 1UL) {
		return;
	}

	LL_ADC_Disable(adc);
	while (LL_ADC_IsEnabled(adc) == 1UL) {
	}
}

#if defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32G4X) || \
	defined(CONFIG_SOC_SERIES_STM32H7X) || \
	defined(CONFIG_SOC_SERIES_STM32L0X) || \
	defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32U5X) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32WLX)

#ifdef LL_ADC_OVS_RATIO_2
/* table for shifting oversampling mostly for ADC3 != ADC_VER_V5_V90 */
	static const uint32_t stm32_adc_ratio_table[] = {
		0,
		LL_ADC_OVS_RATIO_2,
		LL_ADC_OVS_RATIO_4,
		LL_ADC_OVS_RATIO_8,
		LL_ADC_OVS_RATIO_16,
		LL_ADC_OVS_RATIO_32,
		LL_ADC_OVS_RATIO_64,
		LL_ADC_OVS_RATIO_128,
		LL_ADC_OVS_RATIO_256,
	};
#endif /* ! ADC_VER_V5_V90 */

/*
 * Function to configure the oversampling scope. It is basically a wrapper over
 * LL_ADC_SetOverSamplingScope() which in addition stops the ADC if needed.
 */
static void adc_stm32_oversampling_scope(ADC_TypeDef *adc, uint32_t ovs_scope)
{
#if defined(CONFIG_SOC_SERIES_STM32L0X) || \
	defined(CONFIG_SOC_SERIES_STM32WLX)
	/*
	 * setting OVS bits is conditioned to ADC state: ADC must be disabled
	 * or enabled without conversion on going : disable it, it will stop
	 */
	if (LL_ADC_GetOverSamplingScope(adc) == ovs_scope) {
		return;
	}
	adc_stm32_disable(adc);
#endif
	LL_ADC_SetOverSamplingScope(adc, ovs_scope);
}

#if !defined(CONFIG_SOC_SERIES_STM32H7X)
/*
 * Function to configure the oversampling ratio and shift. It is basically a
 * wrapper over LL_ADC_SetOverSamplingRatioShift() which in addition stops the
 * ADC if needed.
 */
static void adc_stm32_oversampling_ratioshift(ADC_TypeDef *adc, uint32_t ratio, uint32_t shift)
{
#if defined(CONFIG_SOC_SERIES_STM32L0X) || \
	defined(CONFIG_SOC_SERIES_STM32WLX)
	/*
	 * setting OVS bits is conditioned to ADC state: ADC must be disabled
	 * or enabled without conversion on going : disable it, it will stop
	 */
	if ((LL_ADC_GetOverSamplingRatio(adc) == ratio)
	    && (LL_ADC_GetOverSamplingShift(adc) == shift)) {
		return;
	}
	adc_stm32_disable(adc);
#endif
	LL_ADC_ConfigOverSamplingRatioShift(adc, ratio, shift);
}
#endif

/*
 * Function to configure the oversampling ratio and shit using stm32 LL
 * ratio is directly the sequence->oversampling (a 2^n value)
 * shift is the corresponding LL_ADC_OVS_SHIFT_RIGHT_x constant
 */
static void adc_stm32_oversampling(ADC_TypeDef *adc, uint8_t ratio, uint32_t shift)
{
	adc_stm32_oversampling_scope(adc, LL_ADC_OVS_GRP_REGULAR_CONTINUED);
#if defined(CONFIG_SOC_SERIES_STM32H7X)
	/*
	 * Set bits manually to circumvent bug in LL Libraries
	 * https://github.com/STMicroelectronics/STM32CubeH7/issues/177
	 */
#if defined(ADC_VER_V5_V90)
	if (adc == ADC3) {
		MODIFY_REG(adc->CFGR2, (ADC_CFGR2_OVSS | ADC3_CFGR2_OVSR),
			(shift | stm32_adc_ratio_table[ratio]));
	} else {
		MODIFY_REG(adc->CFGR2, (ADC_CFGR2_OVSS | ADC_CFGR2_OVSR),
			(shift | (((1UL << ratio) - 1) << ADC_CFGR2_OVSR_Pos)));
	}
#endif /* ADC_VER_V5_V90*/
	MODIFY_REG(adc->CFGR2, (ADC_CFGR2_OVSS | ADC_CFGR2_OVSR),
		(shift | (((1UL << ratio) - 1) << ADC_CFGR2_OVSR_Pos)));
#elif defined(CONFIG_SOC_SERIES_STM32U5X)
	if (adc == ADC1) {
		/* the LL function expects a value from 1 to 1024 */
		adc_stm32_oversampling_ratioshift(adc, (1 << ratio), shift);
	} else {
		/* the LL function expects a value LL_ADC_OVS_RATIO_x */
		adc_stm32_oversampling_ratioshift(adc, stm32_adc_ratio_table[ratio], shift);
	}
#else /* CONFIG_SOC_SERIES_STM32H7X */
	adc_stm32_oversampling_ratioshift(adc, stm32_adc_ratio_table[ratio], shift);
#endif /* CONFIG_SOC_SERIES_STM32H7X */
}
#endif /* CONFIG_SOC_SERIES_STM32xxx */

/*
 * Enable ADC peripheral, and wait until ready if required by SOC.
 */
static int adc_stm32_enable(ADC_TypeDef *adc)
{
	if (LL_ADC_IsEnabled(adc) == 1UL) {
		return 0;
	}
#if defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32G4X) || \
	defined(CONFIG_SOC_SERIES_STM32H7X) || \
	defined(CONFIG_SOC_SERIES_STM32U5X) || \
	defined(CONFIG_SOC_SERIES_STM32WLX)

	LL_ADC_ClearFlag_ADRDY(adc);
	LL_ADC_Enable(adc);

	/*
	 * Enabling ADC modules in L4, WB, G0 and G4 series may fail if they are
	 * still not stabilized, this will wait for a short time to ensure ADC
	 * modules are properly enabled.
	 */
	uint32_t count_timeout = 0;

	while (LL_ADC_IsActiveFlag_ADRDY(adc) == 0) {
		if (LL_ADC_IsEnabled(adc) == 0UL) {
			LL_ADC_Enable(adc);
			count_timeout++;
			if (count_timeout == 10) {
				return -ETIMEDOUT;
			}
		}
	}
#else
	/*
	 * On the stm32F10x, do not re-enable the ADC :
	 * if ADON holds 1 (LL_ADC_IsEnabled is true) and 1 is written,
	 * then conversion starts ; that's not what is expected
	 */
	LL_ADC_Enable(adc);
#endif

	return 0;
}

/*
 * Enable internal channel source
 */
static void adc_stm32_set_common_path(const struct device *dev, uint32_t PathInternal)
{
	const struct adc_stm32_cfg *config =
		(const struct adc_stm32_cfg *)dev->config;
	ADC_TypeDef *adc = (ADC_TypeDef *)config->base;

	ARG_UNUSED(adc); /* Avoid 'unused variable' warning for some families */

	/* Do not remove existing paths */
	PathInternal |= LL_ADC_GetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(adc));
	LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(adc), PathInternal);
}

static void adc_stm32_setup_channels(const struct device *dev, uint8_t channel_id)
{
	const struct adc_stm32_cfg *config = dev->config;
	ADC_TypeDef *adc = (ADC_TypeDef *)config->base;

#ifdef CONFIG_SOC_SERIES_STM32G4X
	if (config->has_temp_channel) {
#if DT_NODE_HAS_STATUS(DT_NODELABEL(adc1), okay)
		if ((__LL_ADC_CHANNEL_TO_DECIMAL_NB(LL_ADC_CHANNEL_TEMPSENSOR_ADC1) == channel_id)
		    && (config->base == ADC1)) {
			adc_stm32_disable(adc);
			adc_stm32_set_common_path(dev, LL_ADC_PATH_INTERNAL_TEMPSENSOR);
			k_usleep(LL_ADC_DELAY_TEMPSENSOR_STAB_US);
		}
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(adc5), okay)
		if ((__LL_ADC_CHANNEL_TO_DECIMAL_NB(LL_ADC_CHANNEL_TEMPSENSOR_ADC5) == channel_id)
		   && (config->base == ADC5)) {
			adc_stm32_disable(adc);
			adc_stm32_set_common_path(dev, LL_ADC_PATH_INTERNAL_TEMPSENSOR);
			k_usleep(LL_ADC_DELAY_TEMPSENSOR_STAB_US);
		}
#endif
	}
#else
	if (config->has_temp_channel &&
		__LL_ADC_CHANNEL_TO_DECIMAL_NB(LL_ADC_CHANNEL_TEMPSENSOR) == channel_id) {
		adc_stm32_disable(adc);
		adc_stm32_set_common_path(dev, LL_ADC_PATH_INTERNAL_TEMPSENSOR);
#ifdef LL_ADC_DELAY_TEMPSENSOR_STAB_US
		k_usleep(LL_ADC_DELAY_TEMPSENSOR_STAB_US);
#endif
	}
#endif /* CONFIG_SOC_SERIES_STM32G4X */

	if (config->has_vref_channel &&
		__LL_ADC_CHANNEL_TO_DECIMAL_NB(LL_ADC_CHANNEL_VREFINT) == channel_id) {
		adc_stm32_disable(adc);
		adc_stm32_set_common_path(dev, LL_ADC_PATH_INTERNAL_VREFINT);
#ifdef LL_ADC_DELAY_VREFINT_STAB_US
		k_usleep(LL_ADC_DELAY_VREFINT_STAB_US);
#endif
	}
#if defined(LL_ADC_CHANNEL_VBAT)
	/* Enable the bridge divider only when needed for ADC conversion. */
	if (config->has_vbat_channel &&
		__LL_ADC_CHANNEL_TO_DECIMAL_NB(LL_ADC_CHANNEL_VBAT) == channel_id) {
		adc_stm32_disable(adc);
		adc_stm32_set_common_path(dev, LL_ADC_PATH_INTERNAL_VBAT);
	}

#endif /* LL_ADC_CHANNEL_VBAT */
}

static void adc_stm32_unset_common_path(const struct device *dev, uint32_t PathInternal)
{
	const struct adc_stm32_cfg *config = dev->config;
	ADC_TypeDef *adc = (ADC_TypeDef *)config->base;
	const uint32_t currentPath = LL_ADC_GetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(adc));

	ARG_UNUSED(adc); /* Avoid 'unused variable' warning for some families */

	PathInternal = ~PathInternal & currentPath;
	LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(adc), PathInternal);
}

static void adc_stm32_teardown_channels(const struct device *dev, uint8_t channel_id)
{
	const struct adc_stm32_cfg *config = dev->config;
	ADC_TypeDef *adc = (ADC_TypeDef *)config->base;

#ifdef CONFIG_SOC_SERIES_STM32G4X
	if (config->has_temp_channel) {
#if DT_NODE_HAS_STATUS(DT_NODELABEL(adc1), okay)
		if ((__LL_ADC_CHANNEL_TO_DECIMAL_NB(LL_ADC_CHANNEL_TEMPSENSOR_ADC1) == channel_id)
		    && (config->base == ADC1)) {
			adc_stm32_disable(adc);
			adc_stm32_unset_common_path(dev, LL_ADC_PATH_INTERNAL_TEMPSENSOR);
		}
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(adc5), okay)
		if ((__LL_ADC_CHANNEL_TO_DECIMAL_NB(LL_ADC_CHANNEL_TEMPSENSOR_ADC5) == channel_id)
		   && (config->base == ADC5)) {
			adc_stm32_disable(adc);
			adc_stm32_unset_common_path(dev, LL_ADC_PATH_INTERNAL_TEMPSENSOR);
		}
#endif
	}
#else
	if (config->has_temp_channel &&
		(__LL_ADC_CHANNEL_TO_DECIMAL_NB(LL_ADC_CHANNEL_TEMPSENSOR) == channel_id)) {
		adc_stm32_disable(adc);
		adc_stm32_unset_common_path(dev, LL_ADC_PATH_INTERNAL_TEMPSENSOR);
	}
#endif /* CONFIG_SOC_SERIES_STM32G4X */

	if (config->has_vref_channel &&
		(__LL_ADC_CHANNEL_TO_DECIMAL_NB(LL_ADC_CHANNEL_VREFINT) == channel_id)) {
		adc_stm32_disable(adc);
		adc_stm32_unset_common_path(dev, LL_ADC_PATH_INTERNAL_VREFINT);
	}
#if defined(LL_ADC_CHANNEL_VBAT)
	/* Enable the bridge divider only when needed for ADC conversion. */
	if (config->has_vbat_channel &&
		(__LL_ADC_CHANNEL_TO_DECIMAL_NB(LL_ADC_CHANNEL_VBAT) == channel_id)) {
		adc_stm32_disable(adc);
		adc_stm32_unset_common_path(dev, LL_ADC_PATH_INTERNAL_VBAT);
	}
#endif /* LL_ADC_CHANNEL_VBAT */
	adc_stm32_enable(adc);
}

static int start_read(const struct device *dev,
		      const struct adc_sequence *sequence)
{
	const struct adc_stm32_cfg *config = dev->config;
	struct adc_stm32_data *data = dev->data;
	ADC_TypeDef *adc = (ADC_TypeDef *)config->base;
	uint8_t resolution;
	int err;

	switch (sequence->resolution) {
#if defined(CONFIG_SOC_SERIES_STM32F1X) || \
	defined(STM32F3X_ADC_V2_5)
	case 12:
		resolution = table_resolution[0];
		break;
#elif defined(CONFIG_SOC_SERIES_STM32U5X)
	case 8:
		resolution = table_resolution[0];
		break;
	case 10:
		resolution = table_resolution[1];
		break;
	case 12:
		resolution = table_resolution[2];
		break;
	case 14:
		resolution = table_resolution[3];
		break;
#elif !defined(CONFIG_SOC_SERIES_STM32H7X)
	case 6:
		resolution = table_resolution[0];
		break;
	case 8:
		resolution = table_resolution[1];
		break;
	case 10:
		resolution = table_resolution[2];
		break;
	case 12:
		resolution = table_resolution[3];
		break;
#else
	case 8:
		resolution = table_resolution[0];
		break;
	case 10:
		resolution = table_resolution[1];
		break;
	case 12:
		resolution = table_resolution[2];
		break;
	case 14:
		resolution = table_resolution[3];
		break;
	case 16:
		resolution = table_resolution[4];
		break;
#endif
	default:
		LOG_ERR("Invalid resolution");
		return -EINVAL;
	}

	uint32_t channels = sequence->channels;
	uint8_t index = find_lsb_set(channels) - 1;

	if (channels > BIT(index)) {
		LOG_ERR("Only single channel supported");
		return -ENOTSUP;
	}

	adc_stm32_setup_channels(dev, index);

	data->buffer = sequence->buffer;

	uint32_t channel = __LL_ADC_DECIMAL_NB_TO_CHANNEL(index);
#if defined(CONFIG_SOC_SERIES_STM32H7X)
	/*
	 * Each channel in the sequence must be previously enabled in PCSEL.
	 * This register controls the analog switch integrated in the IO level.
	 */
	LL_ADC_SetChannelPreSelection(adc, channel);
#elif defined(CONFIG_SOC_SERIES_STM32U5X)
	/*
	 * Each channel in the sequence must be previously enabled in PCSEL.
	 * This register controls the analog switch integrated in the IO level.
	 * Only for ADC1 instance (ADC4 has no Channel preselection capability).
	 */
	if (adc == ADC1) {
		LL_ADC_SetChannelPreselection(adc, channel);
	}
#endif
#if defined(CONFIG_SOC_SERIES_STM32F0X) || \
	defined(CONFIG_SOC_SERIES_STM32L0X)
	LL_ADC_REG_SetSequencerChannels(adc, channel);
#elif defined(CONFIG_SOC_SERIES_STM32WLX)
	/* Init the the ADC group for REGULAR conversion*/
	LL_ADC_REG_SetSequencerConfigurable(adc, LL_ADC_REG_SEQ_CONFIGURABLE);
	LL_ADC_REG_SetTriggerSource(adc, LL_ADC_REG_TRIG_SOFTWARE);
	LL_ADC_REG_SetSequencerLength(adc, LL_ADC_REG_SEQ_SCAN_DISABLE);
	LL_ADC_REG_SetOverrun(adc, LL_ADC_REG_OVR_DATA_OVERWRITTEN);
	LL_ADC_REG_SetSequencerRanks(adc, LL_ADC_REG_RANK_1, channel);
	LL_ADC_REG_SetSequencerChannels(adc, channel);
	/* Wait for config complete config is ready */
	while (LL_ADC_IsActiveFlag_CCRDY(adc) == 0) {
	}
	LL_ADC_ClearFlag_CCRDY(adc);
#elif defined(CONFIG_SOC_SERIES_STM32G0X)
	/* STM32G0 in "not fully configurable" sequencer mode */
	LL_ADC_REG_SetSequencerChannels(adc, channel);
	LL_ADC_REG_SetSequencerConfigurable(adc, LL_ADC_REG_SEQ_FIXED);
	while (LL_ADC_IsActiveFlag_CCRDY(adc) == 0) {
	}
	LL_ADC_ClearFlag_CCRDY(adc);

#else
	LL_ADC_REG_SetSequencerRanks(adc, table_rank[0], channel);
	LL_ADC_REG_SetSequencerLength(adc, table_seq_len[0]);
#endif
	data->channel_count = 1;

	err = check_buffer_size(sequence, data->channel_count);
	if (err) {
		return err;
	}

#if defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32WLX)
	if (LL_ADC_GetResolution(adc) != resolution) {
		/*
		 * Writing ADC_CFGR1 register while ADEN bit is set
		 * resets RES[1:0] bitfield. We need to disable and enable adc.
		 */
		adc_stm32_disable(adc);
		LL_ADC_SetResolution(adc, resolution);
	}
#elif !defined(CONFIG_SOC_SERIES_STM32F1X) && \
	!defined(STM32F3X_ADC_V2_5)
	LL_ADC_SetResolution(adc, resolution);
#endif

#if defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32G4X) || \
	defined(CONFIG_SOC_SERIES_STM32H7X) || \
	defined(CONFIG_SOC_SERIES_STM32L0X) || \
	defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32U5X) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32WLX)

	switch (sequence->oversampling) {
	case 0:
		adc_stm32_oversampling_scope(adc, LL_ADC_OVS_DISABLE);
		break;
	case 1:
		adc_stm32_oversampling(adc, 1, LL_ADC_OVS_SHIFT_RIGHT_1);
		break;
	case 2:
		adc_stm32_oversampling(adc, 2, LL_ADC_OVS_SHIFT_RIGHT_2);
		break;
	case 3:
		adc_stm32_oversampling(adc, 3, LL_ADC_OVS_SHIFT_RIGHT_3);
		break;
	case 4:
		adc_stm32_oversampling(adc, 4, LL_ADC_OVS_SHIFT_RIGHT_4);
		break;
	case 5:
		adc_stm32_oversampling(adc, 5, LL_ADC_OVS_SHIFT_RIGHT_5);
		break;
	case 6:
		adc_stm32_oversampling(adc, 6, LL_ADC_OVS_SHIFT_RIGHT_6);
		break;
	case 7:
		adc_stm32_oversampling(adc, 7, LL_ADC_OVS_SHIFT_RIGHT_7);
		break;
	case 8:
		adc_stm32_oversampling(adc, 8, LL_ADC_OVS_SHIFT_RIGHT_8);
		break;
#if defined(CONFIG_SOC_SERIES_STM32H7X) || defined(CONFIG_SOC_SERIES_STM32U5X)
	/* stm32 U5, H7 ADC1 & 2 have oversampling ratio from 1..1024 */
	case 9:
		adc_stm32_oversampling(adc, 9, LL_ADC_OVS_SHIFT_RIGHT_9);
		break;
	case 10:
		adc_stm32_oversampling(adc, 10, LL_ADC_OVS_SHIFT_RIGHT_10);
		break;
#endif /* CONFIG_SOC_SERIES_STM32H7X */
	default:
		LOG_ERR("Invalid oversampling");
		adc_stm32_enable(adc);
		return -EINVAL;
	}
#else
	if (sequence->oversampling) {
		LOG_ERR("Oversampling not supported");
		return -ENOTSUP;
	}
#endif

	if (sequence->calibrate) {
#if !defined(CONFIG_SOC_SERIES_STM32F2X) && \
	!defined(CONFIG_SOC_SERIES_STM32F4X) && \
	!defined(CONFIG_SOC_SERIES_STM32F7X) && \
	!defined(CONFIG_SOC_SERIES_STM32F1X) && \
	!defined(STM32F3X_ADC_V2_5) && \
	!defined(CONFIG_SOC_SERIES_STM32L1X)

		/* we cannot calibrate the ADC while the ADC is enabled */
		adc_stm32_disable(adc);
		adc_stm32_calib(dev);
#else
		LOG_ERR("Calibration not supported");
		return -ENOTSUP;
#endif
	}

	/*
	 * Make sure the ADC is enabled as it might have been disabled earlier
	 * to set the resolution, to set the oversampling or to perform the
	 * calibration.
	 */
	adc_stm32_enable(adc);

#if defined(CONFIG_SOC_SERIES_STM32F0X) || \
	defined(STM32F3X_ADC_V1_1) || \
	defined(CONFIG_SOC_SERIES_STM32L0X) || \
	defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32G4X) || \
	defined(CONFIG_SOC_SERIES_STM32H7X) || \
	defined(CONFIG_SOC_SERIES_STM32U5X) || \
	defined(CONFIG_SOC_SERIES_STM32WLX)
	LL_ADC_EnableIT_EOC(adc);
#elif defined(STM32F3X_ADC_V2_5) || \
	defined(CONFIG_SOC_SERIES_STM32F1X)
	LL_ADC_EnableIT_EOS(adc);
#else
	LL_ADC_EnableIT_EOCS(adc);
#endif

	adc_context_start_read(&data->ctx, sequence);

	err = adc_context_wait_for_completion(&data->ctx);

	adc_stm32_teardown_channels(dev, index);

	return err;
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_stm32_data *data =
		CONTAINER_OF(ctx, struct adc_stm32_data, ctx);

	data->repeat_buffer = data->buffer;

	adc_stm32_start_conversion(data->dev);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat_sampling)
{
	struct adc_stm32_data *data =
		CONTAINER_OF(ctx, struct adc_stm32_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static void adc_stm32_isr(const struct device *dev)
{
	struct adc_stm32_data *data = dev->data;
	const struct adc_stm32_cfg *config =
		(const struct adc_stm32_cfg *)dev->config;
	ADC_TypeDef *adc = config->base;

	*data->buffer++ = LL_ADC_REG_ReadConversionData32(adc);

	adc_context_on_sampling_done(&data->ctx, dev);

	LOG_DBG("%s ISR triggered.", dev->name);
}

static int adc_stm32_read(const struct device *dev,
			  const struct adc_sequence *sequence)
{
	struct adc_stm32_data *data = dev->data;
	int error;

	adc_context_lock(&data->ctx, false, NULL);
	error = start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}

#ifdef CONFIG_ADC_ASYNC
static int adc_stm32_read_async(const struct device *dev,
				 const struct adc_sequence *sequence,
				 struct k_poll_signal *async)
{
	struct adc_stm32_data *data = dev->data;
	int error;

	adc_context_lock(&data->ctx, true, async);
	error = start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}
#endif

static int adc_stm32_check_acq_time(uint16_t acq_time)
{
	if (acq_time == ADC_ACQ_TIME_MAX) {
		return ARRAY_SIZE(acq_time_tbl) - 1;
	}

	for (int i = 0; i < 8; i++) {
		if (acq_time == ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS,
					     acq_time_tbl[i])) {
			return i;
		}
	}

	if (acq_time == ADC_ACQ_TIME_DEFAULT) {
		return 0;
	}

	LOG_ERR("Conversion time not supported.");
	return -EINVAL;
}

static void adc_stm32_setup_speed(const struct device *dev, uint8_t id,
				  uint8_t acq_time_index)
{
	const struct adc_stm32_cfg *config =
		(const struct adc_stm32_cfg *)dev->config;
	ADC_TypeDef *adc = config->base;

#if defined(CONFIG_SOC_SERIES_STM32F0X) || defined(CONFIG_SOC_SERIES_STM32L0X)
	LL_ADC_SetSamplingTimeCommonChannels(adc,
		table_samp_time[acq_time_index]);
#elif defined(CONFIG_SOC_SERIES_STM32G0X)
	/* Errata ES0418 and more: ADC sampling time might be one cycle longer */
	if (acq_time_index  < 2) {
		acq_time_index = 2;
	}
	LL_ADC_SetSamplingTimeCommonChannels(adc, LL_ADC_SAMPLINGTIME_COMMON_1,
		table_samp_time[acq_time_index]);
#elif defined(CONFIG_SOC_SERIES_STM32WLX)
	LL_ADC_SetChannelSamplingTime(adc,
				      __LL_ADC_DECIMAL_NB_TO_CHANNEL(id),
				      LL_ADC_SAMPLINGTIME_COMMON_1);
	LL_ADC_SetSamplingTimeCommonChannels(adc,
					     __LL_ADC_DECIMAL_NB_TO_CHANNEL(id),
					     table_samp_time[acq_time_index]);
#else
	LL_ADC_SetChannelSamplingTime(adc,
		__LL_ADC_DECIMAL_NB_TO_CHANNEL(id),
		table_samp_time[acq_time_index]);
#endif
}

static int adc_stm32_channel_setup(const struct device *dev,
				   const struct adc_channel_cfg *channel_cfg)
{
#if defined(CONFIG_SOC_SERIES_STM32F0X) || \
	defined(CONFIG_SOC_SERIES_STM32G0X) || \
	 defined(CONFIG_SOC_SERIES_STM32L0X)
	struct adc_stm32_data *data = dev->data;
#endif
	int acq_time_index;

	if (channel_cfg->channel_id >= STM32_CHANNEL_COUNT) {
		LOG_ERR("Channel %d is not valid", channel_cfg->channel_id);
		return -EINVAL;
	}

	acq_time_index = adc_stm32_check_acq_time(
				channel_cfg->acquisition_time);
	if (acq_time_index < 0) {
		return acq_time_index;
	}
#if defined(CONFIG_SOC_SERIES_STM32F0X) || \
	defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32L0X)
	if (data->acq_time_index == -1) {
		data->acq_time_index = acq_time_index;
	} else {
		/* All channels of F0/L0 must have identical acquisition time.*/
		if (acq_time_index != data->acq_time_index) {
			return -EINVAL;
		}
	}
#endif

	if (channel_cfg->differential) {
		LOG_ERR("Differential channels are not supported");
		return -EINVAL;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Invalid channel gain");
		return -EINVAL;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("Invalid channel reference");
		return -EINVAL;
	}

	adc_stm32_setup_speed(dev, channel_cfg->channel_id,
				  acq_time_index);

	LOG_DBG("Channel setup succeeded!");

	return 0;
}

static int adc_stm32_init(const struct device *dev)
{
	struct adc_stm32_data *data = dev->data;
	const struct adc_stm32_cfg *config = dev->config;
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	ADC_TypeDef *adc = (ADC_TypeDef *)config->base;
	int err;

	LOG_DBG("Initializing....");

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	data->dev = dev;
#if defined(CONFIG_SOC_SERIES_STM32F0X) || \
	defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32L0X)
	/*
	 * All conversion time for all channels on one ADC instance for F0 and
	 * L0 series chips has to be the same. For STM32G0 currently only one
	 * of the two available common channel conversion times is used.
	 * This additional variable is for checking if the conversion time
	 * selection of all channels on one ADC instance is the same.
	 */
	data->acq_time_index = -1;
#endif

	if (clock_control_on(clk,
		(clock_control_subsys_t *) &config->pclken) != 0) {
		return -EIO;
	}

	/* Configure dt provided device signals when available */
	err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		LOG_ERR("ADC pinctrl setup failed (%d)", err);
		return err;
	}
#if defined(CONFIG_SOC_SERIES_STM32U5X)
	/* Enable the independent analog supply */
	LL_PWR_EnableVDDA();
#endif /* CONFIG_SOC_SERIES_STM32U5X */

#if defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32G4X) || \
	defined(CONFIG_SOC_SERIES_STM32H7X) || \
	defined(CONFIG_SOC_SERIES_STM32U5X)
	/*
	 * L4, WB, G4, H7 and U5 series STM32 needs to be awaken from deep sleep
	 * mode, and restore its calibration parameters if there are some
	 * previously stored calibration parameters.
	 */

	LL_ADC_DisableDeepPowerDown(adc);
#elif defined(CONFIG_SOC_SERIES_STM32WLX)
	/* The ADC clock must be disabled by clock gating during CPU1 sleep/stop */
	LL_APB2_GRP1_DisableClockSleep(LL_APB2_GRP1_PERIPH_ADC);
#endif
	/*
	 * F3, L4, WB, G0 and G4 ADC modules need some time
	 * to be stabilized before performing any enable or calibration actions.
	 */
#if defined(STM32F3X_ADC_V1_1) || \
	defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32G4X) || \
	defined(CONFIG_SOC_SERIES_STM32H7X) || \
	defined(CONFIG_SOC_SERIES_STM32U5X) || \
	defined(CONFIG_SOC_SERIES_STM32WLX)
	LL_ADC_EnableInternalRegulator(adc);
	k_busy_wait(LL_ADC_DELAY_INTERNAL_REGUL_STAB_US);
#endif

#if defined(CONFIG_SOC_SERIES_STM32F0X) || \
	defined(CONFIG_SOC_SERIES_STM32L0X) || \
	defined(CONFIG_SOC_SERIES_STM32WLX)
	LL_ADC_SetClock(adc, LL_ADC_CLOCK_SYNC_PCLK_DIV4);
#elif defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32G4X) || \
	defined(CONFIG_SOC_SERIES_STM32H7X)
	LL_ADC_SetCommonClock(__LL_ADC_COMMON_INSTANCE(adc),
			      LL_ADC_CLOCK_SYNC_PCLK_DIV4);
#elif defined(STM32F3X_ADC_V1_1)
	/*
	 * Set the synchronous clock mode to HCLK/1 (DIV1) or HCLK/2 (DIV2)
	 * Both are valid common clock setting values.
	 * The HCLK/1(DIV1) is possible only if
	 * the ahb-prescaler = <1> in the RCC_CFGR.
	 */
	LL_ADC_SetCommonClock(__LL_ADC_COMMON_INSTANCE(adc),
			      LL_ADC_CLOCK_SYNC_PCLK_DIV2);
#elif defined(CONFIG_SOC_SERIES_STM32L1X) || \
	defined(CONFIG_SOC_SERIES_STM32U5X)
	LL_ADC_SetCommonClock(__LL_ADC_COMMON_INSTANCE(adc),
			LL_ADC_CLOCK_ASYNC_DIV4);
#endif

#if !defined(CONFIG_SOC_SERIES_STM32F2X) && \
	!defined(CONFIG_SOC_SERIES_STM32F4X) && \
	!defined(CONFIG_SOC_SERIES_STM32F7X) && \
	!defined(CONFIG_SOC_SERIES_STM32F1X) && \
	!defined(STM32F3X_ADC_V2_5) && \
	!defined(CONFIG_SOC_SERIES_STM32L1X)
	/*
	 * Calibration of F1 and F3 (ADC1_V2_5) series has to be started
	 * after ADC Module is enabled.
	 */
	adc_stm32_calib(dev);
#endif

#if defined(CONFIG_SOC_SERIES_STM32F0X) || \
	defined(CONFIG_SOC_SERIES_STM32L0X) || \
	defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32G4X) || \
	defined(CONFIG_SOC_SERIES_STM32H7X) || \
	defined(CONFIG_SOC_SERIES_STM32U5X) || \
	defined(CONFIG_SOC_SERIES_STM32WLX)
	if (LL_ADC_IsActiveFlag_ADRDY(adc)) {
		LL_ADC_ClearFlag_ADRDY(adc);
	}
#endif

#if defined(CONFIG_SOC_SERIES_STM32F0X) || \
	defined(STM32F3X_ADC_V1_1) || \
	defined(CONFIG_SOC_SERIES_STM32L0X) || \
	defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32G4X) || \
	defined(CONFIG_SOC_SERIES_STM32H7X) || \
	defined(CONFIG_SOC_SERIES_STM32U5X) || \
	defined(CONFIG_SOC_SERIES_STM32WLX)
	/*
	 * ADC modules on these series have to wait for some cycles to be
	 * enabled.
	 */
	uint32_t adc_rate, wait_cycles;

	if (clock_control_get_rate(clk,
		(clock_control_subsys_t *) &config->pclken, &adc_rate) < 0) {
		LOG_ERR("ADC clock rate get error.");
	}

	wait_cycles = SystemCoreClock / adc_rate *
		      LL_ADC_DELAY_CALIB_ENABLE_ADC_CYCLES;

	for (int i = wait_cycles; i >= 0; i--) {
	}
#endif

	err = adc_stm32_enable(adc);
	if (err < 0) {
		return err;
	}

	config->irq_cfg_func();

#if defined(CONFIG_SOC_SERIES_STM32F1X) || \
	defined(STM32F3X_ADC_V2_5)
	/*
	 * Calibration of F1 and F3 (ADC1_V2_5) must starts after two cycles
	 * after ADON is set.
	 */
	LL_ADC_StartCalibration(adc);
	LL_ADC_REG_SetTriggerSource(adc, LL_ADC_REG_TRIG_SOFTWARE);
#endif

#ifdef CONFIG_SOC_SERIES_STM32H7X
	/*
	 * To ensure linearity the factory calibration values
	 * should be loaded on initialization.
	 */
	uint32_t channel_offset = 0U;
	uint32_t linear_calib_buffer = 0U;

	if (adc == ADC1) {
		channel_offset = 0UL;
	} else if (adc == ADC2) {
		channel_offset = 8UL;
	} else   /*Case ADC3*/ {
		channel_offset = 16UL;
	}
	/* Read factory calibration factors */
	for (uint32_t count = 0UL; count < ADC_LINEAR_CALIB_REG_COUNT; count++) {
		linear_calib_buffer = *(uint32_t *)(
			ADC_LINEAR_CALIB_REG_1_ADDR + channel_offset + count
		);
		LL_ADC_SetCalibrationLinearFactor(
			adc, LL_ADC_CALIB_LINEARITY_WORD1 << count,
			linear_calib_buffer
		);
	}
#endif
	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct adc_driver_api api_stm32_driver_api = {
	.channel_setup = adc_stm32_channel_setup,
	.read = adc_stm32_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_stm32_read_async,
#endif
	.ref_internal = STM32_ADC_VREF_MV, /* VREF is usually connected to VDD */
};

#ifdef CONFIG_ADC_STM32_SHARED_IRQS

bool adc_stm32_is_irq_active(ADC_TypeDef *adc)
{
	return LL_ADC_IsActiveFlag_EOCS(adc) ||
	       LL_ADC_IsActiveFlag_OVR(adc) ||
	       LL_ADC_IsActiveFlag_JEOS(adc) ||
	       LL_ADC_IsActiveFlag_AWD1(adc);
}

#define HANDLE_IRQS(index)							\
	static const struct device *const dev_##index =				\
		DEVICE_DT_INST_GET(index);					\
	const struct adc_stm32_cfg *cfg_##index = dev_##index->config;		\
	ADC_TypeDef *adc_##index = (ADC_TypeDef *)(cfg_##index->base);		\
										\
	if (adc_stm32_is_irq_active(adc_##index)) {				\
		adc_stm32_isr(dev_##index);					\
	}

static void adc_stm32_shared_irq_handler(void)
{
	DT_INST_FOREACH_STATUS_OKAY(HANDLE_IRQS);
}

static void adc_stm32_irq_init(void)
{
	if (init_irq) {
		init_irq = false;
		IRQ_CONNECT(DT_INST_IRQN(0),
			DT_INST_IRQ(0, priority),
			adc_stm32_shared_irq_handler, NULL, 0);
		irq_enable(DT_INST_IRQN(0));
	}
}

#define ADC_STM32_CONFIG(index)						\
static const struct adc_stm32_cfg adc_stm32_cfg_##index = {		\
	.base = (ADC_TypeDef *)DT_INST_REG_ADDR(index),			\
	.irq_cfg_func = adc_stm32_irq_init,				\
	.pclken = {							\
		.enr = DT_INST_CLOCKS_CELL(index, bits),		\
		.bus = DT_INST_CLOCKS_CELL(index, bus),			\
	},								\
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),			\
	.has_temp_channel = DT_INST_PROP(index, has_temp_channel),	\
	.has_vref_channel = DT_INST_PROP(index, has_vref_channel),	\
	.has_vbat_channel = DT_INST_PROP(index, has_vbat_channel),	\
};
#else
#define ADC_STM32_CONFIG(index)						\
static void adc_stm32_cfg_func_##index(void)				\
{									\
	IRQ_CONNECT(DT_INST_IRQN(index),				\
		    DT_INST_IRQ(index, priority),			\
		    adc_stm32_isr, DEVICE_DT_INST_GET(index), 0);	\
	irq_enable(DT_INST_IRQN(index));				\
}									\
									\
static const struct adc_stm32_cfg adc_stm32_cfg_##index = {		\
	.base = (ADC_TypeDef *)DT_INST_REG_ADDR(index),			\
	.irq_cfg_func = adc_stm32_cfg_func_##index,			\
	.pclken = {							\
		.enr = DT_INST_CLOCKS_CELL(index, bits),		\
		.bus = DT_INST_CLOCKS_CELL(index, bus),			\
	},								\
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),			\
	.has_temp_channel = DT_INST_PROP(index, has_temp_channel),	\
	.has_vref_channel = DT_INST_PROP(index, has_vref_channel),	\
	.has_vbat_channel = DT_INST_PROP(index, has_vbat_channel),	\
};
#endif /* CONFIG_ADC_STM32_SHARED_IRQS */

#define STM32_ADC_INIT(index)						\
									\
PINCTRL_DT_INST_DEFINE(index);						\
									\
ADC_STM32_CONFIG(index)							\
									\
static struct adc_stm32_data adc_stm32_data_##index = {			\
	ADC_CONTEXT_INIT_TIMER(adc_stm32_data_##index, ctx),		\
	ADC_CONTEXT_INIT_LOCK(adc_stm32_data_##index, ctx),		\
	ADC_CONTEXT_INIT_SYNC(adc_stm32_data_##index, ctx),		\
};									\
									\
DEVICE_DT_INST_DEFINE(index,						\
		    &adc_stm32_init, NULL,				\
		    &adc_stm32_data_##index, &adc_stm32_cfg_##index,	\
		    POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,		\
		    &api_stm32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(STM32_ADC_INIT)
