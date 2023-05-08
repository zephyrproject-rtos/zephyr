/*
 * Copyright (c) 2018 Kokoon Technology Limited
 * Copyright (c) 2019 Song Qiang <songqiang1304521@gmail.com>
 * Copyright (c) 2019 Endre Karlson
 * Copyright (c) 2020 Teslabs Engineering S.L.
 * Copyright (c) 2021 Marius Scholtz, RIC Electronics
 * Copyright (c) 2023 Hein Wessels, Nobleo Technology
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

#ifdef CONFIG_ADC_STM32_DMA
#include <zephyr/drivers/dma/dma_stm32.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/toolchain/common.h>
#include <stm32_ll_dma.h>
#endif

#define ADC_CONTEXT_USES_KERNEL_TIMER
#define ADC_CONTEXT_ENABLE_ON_COMPLETE
#include "adc_context.h"

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_stm32);

#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/dt-bindings/adc/stm32_adc.h>
#include <zephyr/irq.h>

#if defined(CONFIG_SOC_SERIES_STM32F3X)
#if defined(ADC1_V2_5)
#define STM32F3X_ADC_V2_5
#elif defined(ADC5_V1_1)
#define STM32F3X_ADC_V1_1
#endif
#endif

/* reference voltage for the ADC */
#define STM32_ADC_VREF_MV DT_INST_PROP(0, vref_mv)

#if !defined(CONFIG_SOC_SERIES_STM32C0X) && \
	!defined(CONFIG_SOC_SERIES_STM32F0X) && \
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
/* Length of this array signifies the maximum sequence length */
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
#elif defined(CONFIG_SOC_SERIES_STM32C0X) || \
	defined(CONFIG_SOC_SERIES_STM32L0X) || \
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
	defined(CONFIG_SOC_SERIES_STM32H5X) || \
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

#ifdef CONFIG_ADC_STM32_DMA
struct stream {
	const struct device *dma_dev;
	uint32_t channel;
	struct dma_config dma_cfg;
	struct dma_block_config dma_blk_cfg;
	uint8_t priority;
	bool src_addr_increment;
	bool dst_addr_increment;
};
#endif /* CONFIG_ADC_STM32_DMA */

struct adc_stm32_data {
	struct adc_context ctx;
	const struct device *dev;
	uint16_t *buffer;
	uint16_t *repeat_buffer;

	uint8_t resolution;
	uint32_t channels;
	uint8_t channel_count;
	uint8_t samples_count;
#if defined(CONFIG_SOC_SERIES_STM32C0X) || \
	defined(CONFIG_SOC_SERIES_STM32F0X) || \
	defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32L0X)
	int8_t acq_time_index;
#endif

#ifdef CONFIG_ADC_STM32_DMA
	volatile int dma_error;
	struct stream dma;
#endif
};

struct adc_stm32_cfg {
	ADC_TypeDef *base;
	void (*irq_cfg_func)(void);
	struct stm32_pclken pclken;
	const struct pinctrl_dev_config *pcfg;
	int8_t temp_channel;
	int8_t vref_channel;
	int8_t vbat_channel;
	int8_t res_table_size;
	const uint32_t res_table[];
};

#ifdef CONFIG_ADC_STM32_SHARED_IRQS
static bool init_irq = true;
#endif

#ifdef CONFIG_ADC_STM32_DMA
static int adc_stm32_dma_start(const struct device *dev,
			       void *buffer, size_t channel_count)
{
	const struct adc_stm32_cfg *config = dev->config;
	ADC_TypeDef *adc = (ADC_TypeDef *)config->base;
	struct adc_stm32_data *data = dev->data;
	struct dma_block_config *blk_cfg;
	int ret;

	struct stream *dma = &data->dma;

	blk_cfg = &dma->dma_blk_cfg;

	/* prepare the block */
	blk_cfg->block_size = channel_count * sizeof(int16_t);

	/* Source and destination */
	blk_cfg->source_address = (uint32_t)LL_ADC_DMA_GetRegAddr(adc, LL_ADC_DMA_REG_REGULAR_DATA);
	blk_cfg->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	blk_cfg->source_reload_en = 0;

	blk_cfg->dest_address = (uint32_t)buffer;
	blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	blk_cfg->dest_reload_en = 0;

	/* Manually set the FIFO threshold to 1/4 because the
	 * dmamux DTS entry does not contain fifo threshold
	 */
	blk_cfg->fifo_mode_control = 0;

	/* direction is given by the DT */
	dma->dma_cfg.head_block = blk_cfg;
	dma->dma_cfg.user_data = data;

	ret = dma_config(data->dma.dma_dev, data->dma.channel,
			 &dma->dma_cfg);
	if (ret != 0) {
		LOG_ERR("Problem setting up DMA: %d", ret);
		return ret;
	}

	/* Allow ADC to create DMA request and set to one-shot mode,
	 * as implemented in HAL drivers, if applicable.
	 */
#if defined(ADC_VER_V5_V90)
	if (adc == ADC3) {
		LL_ADC_REG_SetDMATransferMode(adc,
			ADC3_CFGR_DMACONTREQ(LL_ADC_REG_DMA_TRANSFER_LIMITED));
		LL_ADC_EnableDMAReq(adc);
	} else {
		LL_ADC_REG_SetDataTransferMode(adc,
			ADC_CFGR_DMACONTREQ(LL_ADC_REG_DMA_TRANSFER_LIMITED));
	}
#elif defined(ADC_VER_V5_X)
	LL_ADC_REG_SetDataTransferMode(adc, LL_ADC_REG_DMA_TRANSFER_LIMITED);
#endif

	data->dma_error = 0;
	ret = dma_start(data->dma.dma_dev, data->dma.channel);
	if (ret != 0) {
		LOG_ERR("Problem starting DMA: %d", ret);
		return ret;
	}

	LOG_DBG("DMA started");

	return ret;
}
#endif /* CONFIG_ADC_STM32_DMA */

#if defined(CONFIG_ADC_STM32_DMA) && defined(CONFIG_SOC_SERIES_STM32H7X)
/* Returns true if given buffer is in a non-cacheable SRAM region.
 * This is determined using the device tree, meaning the .nocache region won't work.
 * The entire buffer must be in a single region.
 * An example of how the SRAM region can be defined in the DTS:
 *	&sram4 {
 *		zephyr,memory-region-mpu = "RAM_NOCACHE";
 *	};
 */
static bool address_in_non_cacheable_sram(const uint16_t *buffer, const uint16_t size)
{
	/* Default if no valid SRAM region found or buffer+size not located in a single region */
	bool cachable = false;
#define IS_NON_CACHEABLE_REGION_FN(node_id)                                                    \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, zephyr_memory_region_mpu), ({                    \
			const uint32_t region_start = DT_REG_ADDR(node_id);                    \
			const uint32_t region_end = region_start + DT_REG_SIZE(node_id);       \
			if (((uint32_t)buffer >= region_start) &&                              \
				(((uint32_t)buffer + size) < region_end)) {                    \
				cachable = strcmp(DT_PROP(node_id, zephyr_memory_region_mpu),  \
						"RAM_NOCACHE") == 0;                           \
			}                                                                      \
		}),                                                                            \
		(EMPTY))
	DT_FOREACH_STATUS_OKAY(mmio_sram, IS_NON_CACHEABLE_REGION_FN);

	return cachable;
}
#endif /* defined(CONFIG_ADC_STM32_DMA) && defined(CONFIG_SOC_SERIES_STM32H7X) */

static int check_buffer(const struct adc_sequence *sequence,
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

#if defined(CONFIG_ADC_STM32_DMA) && defined(CONFIG_SOC_SERIES_STM32H7X)
	/* Buffer is forced to be in non-cacheable SRAM region to avoid cache maintenance */
	if (!address_in_non_cacheable_sram(sequence->buffer, needed_buffer_size)) {
		LOG_ERR("Supplied buffer is not in a non-cacheable region according to DTS.");
		return -EINVAL;
	}
#endif

	return 0;
}

static void adc_stm32_start_conversion(const struct device *dev)
{
	const struct adc_stm32_cfg *config = dev->config;
	ADC_TypeDef *adc = (ADC_TypeDef *)config->base;

	LOG_DBG("Starting conversion");

#if defined(CONFIG_SOC_SERIES_STM32C0X) || \
	defined(CONFIG_SOC_SERIES_STM32F0X) || \
	defined(CONFIG_SOC_SERIES_STM32F3X) || \
	defined(CONFIG_SOC_SERIES_STM32L0X) || \
	defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32G4X) || \
	defined(CONFIG_SOC_SERIES_STM32H5X) || \
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
	defined(CONFIG_SOC_SERIES_STM32H5X) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32G4X)
	LL_ADC_StartCalibration(adc, LL_ADC_SINGLE_ENDED);
#elif defined(CONFIG_SOC_SERIES_STM32C0X) || \
	defined(CONFIG_SOC_SERIES_STM32F0X) || \
	defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32L0X) || \
	defined(CONFIG_SOC_SERIES_STM32WLX)
	LL_ADC_StartCalibration(adc);
#elif defined(CONFIG_SOC_SERIES_STM32U5X)
	LL_ADC_StartCalibration(adc, LL_ADC_CALIB_OFFSET);
#elif defined(CONFIG_SOC_SERIES_STM32H7X)
	LL_ADC_StartCalibration(adc, LL_ADC_CALIB_OFFSET, LL_ADC_SINGLE_ENDED);
#endif
	/* Make sure ADCAL is cleared before returning for proper operations
	 * on the ADC control register, for enabling the peripheral for example
	 */
	while (LL_ADC_IsCalibrationOnGoing(adc)) {
	}
}
#endif

/*
 * Disable ADC peripheral, and wait until it is disabled
 */
static void adc_stm32_disable(ADC_TypeDef *adc)
{
	if (LL_ADC_IsEnabled(adc) != 1UL) {
		return;
	}

	/* Stop ongoing conversion if any
	 * Software must poll ADSTART (or JADSTART) until the bit is reset before assuming
	 * the ADC is completely stopped.
	 */

#if !defined(CONFIG_SOC_SERIES_STM32F2X) && \
	!defined(CONFIG_SOC_SERIES_STM32F4X) && \
	!defined(CONFIG_SOC_SERIES_STM32F7X) && \
	!defined(CONFIG_SOC_SERIES_STM32F1X) && \
	!defined(STM32F3X_ADC_V2_5) && \
	!defined(CONFIG_SOC_SERIES_STM32L1X)
	if (LL_ADC_REG_IsConversionOngoing(adc)) {
		LL_ADC_REG_StopConversion(adc);
		while (LL_ADC_REG_IsConversionOngoing(adc)) {
		}
	}
#endif

#if defined(CONFIG_SOC_SERIES_STM32G4X) || \
	defined(CONFIG_SOC_SERIES_STM32F3X) || \
	defined(CONFIG_SOC_SERIES_STM32H7X) || \
	defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32U5X) || \
	defined(CONFIG_SOC_SERIES_STM32WBX)
	if (LL_ADC_INJ_IsConversionOngoing(adc)) {
		LL_ADC_INJ_StopConversion(adc);
		while (LL_ADC_INJ_IsConversionOngoing(adc)) {
		}
	}
#endif

	LL_ADC_Disable(adc);

	/* Wait ADC is fully disabled so that we don't leave the driver into intermediate state
	 * which could prevent enabling the peripheral
	 */
	while (LL_ADC_IsEnabled(adc) == 1UL) {
	}
}

#if defined(CONFIG_SOC_SERIES_STM32C0X) || \
	defined(CONFIG_SOC_SERIES_STM32G0X) || \
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

/*
 * Function to configure the oversampling ratio and shit using stm32 LL
 * ratio is directly the sequence->oversampling (a 2^n value)
 * shift is the corresponding LL_ADC_OVS_SHIFT_RIGHT_x constant
 */
static void adc_stm32_oversampling(ADC_TypeDef *adc, uint8_t ratio, uint32_t shift)
{
	adc_stm32_oversampling_scope(adc, LL_ADC_OVS_GRP_REGULAR_CONTINUED);
#if defined(CONFIG_SOC_SERIES_STM32H7X)
	/* Certain variants of the H7, such as STM32H72x/H73x has ADC3
	 * as a separate entity and require special handling.
	 */
#if defined(ADC_VER_V5_V90)
	if (adc != ADC3) {
		/* the LL function expects a value from 1 to 1024 */
		adc_stm32_oversampling_ratioshift(adc, 1 << ratio, shift);
	} else {
		/* the LL function expects a value LL_ADC_OVS_RATIO_x */
		adc_stm32_oversampling_ratioshift(adc, stm32_adc_ratio_table[ratio], shift);
	}
#else
	/* the LL function expects a value from 1 to 1024 */
	adc_stm32_oversampling_ratioshift(adc, 1 << ratio, shift);
#endif /* defined(ADC_VER_V5_V90) */
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
#if defined(CONFIG_SOC_SERIES_STM32C0X) || \
	defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32G4X) || \
	defined(CONFIG_SOC_SERIES_STM32H5X) || \
	defined(CONFIG_SOC_SERIES_STM32H7X) || \
	defined(CONFIG_SOC_SERIES_STM32U5X) || \
	defined(CONFIG_SOC_SERIES_STM32WLX)

	LL_ADC_ClearFlag_ADRDY(adc);
	LL_ADC_Enable(adc);
	/*
	 * Enabling ADC modules in WL, L4, WB, G0 and G4 series may fail if they are
	 * still not stabilized, this will wait for a short time (about 1ms)
	 * to ensure ADC modules are properly enabled.
	 */
	uint32_t count_timeout = 0;

	while (LL_ADC_IsActiveFlag_ADRDY(adc) == 0) {
		if (LL_ADC_IsEnabled(adc) == 0UL) {
			LL_ADC_Enable(adc);
		}
		count_timeout++;
		k_busy_wait(100);
		if (count_timeout >= 10) {
			return -ETIMEDOUT;
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

static void adc_stm32_setup_channel(const struct device *dev, uint8_t channel_id)
{
	const struct adc_stm32_cfg *config = dev->config;
	ADC_TypeDef *adc = (ADC_TypeDef *)config->base;

	if (config->temp_channel == channel_id) {
		adc_stm32_disable(adc);
		adc_stm32_set_common_path(dev, LL_ADC_PATH_INTERNAL_TEMPSENSOR);
		k_usleep(LL_ADC_DELAY_TEMPSENSOR_STAB_US);
	}

	if (config->vref_channel == channel_id) {
		adc_stm32_disable(adc);
		adc_stm32_set_common_path(dev, LL_ADC_PATH_INTERNAL_VREFINT);
#ifdef LL_ADC_DELAY_VREFINT_STAB_US
		k_usleep(LL_ADC_DELAY_VREFINT_STAB_US);
#endif
	}

#if defined(LL_ADC_CHANNEL_VBAT)
	/* Enable the bridge divider only when needed for ADC conversion. */
	if (config->vbat_channel == channel_id) {
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

static void adc_stm32_teardown_channels(const struct device *dev)
{
	const struct adc_stm32_cfg *config = dev->config;
	struct adc_stm32_data *data = dev->data;
	ADC_TypeDef *adc = (ADC_TypeDef *)config->base;
	uint8_t channel_id;

	for (uint32_t channels = data->channels; channels; channels &= ~BIT(channel_id)) {
		channel_id = find_lsb_set(channels) - 1;
		if (config->temp_channel == channel_id) {
			adc_stm32_disable(adc);
			adc_stm32_unset_common_path(dev, LL_ADC_PATH_INTERNAL_TEMPSENSOR);
		}

		if (config->vref_channel == channel_id) {
			adc_stm32_disable(adc);
			adc_stm32_unset_common_path(dev, LL_ADC_PATH_INTERNAL_VREFINT);
		}

#if defined(LL_ADC_CHANNEL_VBAT)
		/* Enable the bridge divider only when needed for ADC conversion. */
		if (config->vbat_channel == channel_id) {
			adc_stm32_disable(adc);
			adc_stm32_unset_common_path(dev, LL_ADC_PATH_INTERNAL_VBAT);
		}
#endif /* LL_ADC_CHANNEL_VBAT */
	}

	adc_stm32_enable(adc);
}

#ifdef CONFIG_ADC_STM32_DMA
static void dma_callback(const struct device *dev, void *user_data,
			 uint32_t channel, int status)
{
	/* user_data directly holds the adc device */
	struct adc_stm32_data *data = user_data;
	const struct adc_stm32_cfg *config = dev->config;
	ADC_TypeDef *adc = (ADC_TypeDef *)config->base;

	LOG_DBG("dma callback");

	if (channel == data->dma.channel) {
#if !defined(CONFIG_SOC_SERIES_STM32F1X)
		if (LL_ADC_IsActiveFlag_OVR(adc) || (status == 0)) {
#else
		if (status == 0) {
#endif /* !defined(CONFIG_SOC_SERIES_STM32F1X) */
			data->samples_count = data->channel_count;
			data->buffer += data->channel_count;
			/* Stop the DMA engine, only to start it again when the callback returns
			 * ADC_ACTION_REPEAT or ADC_ACTION_CONTINUE, or the number of samples
			 * haven't been reached Starting the DMA engine is done
			 * within adc_context_start_sampling
			 */
			dma_stop(data->dma.dma_dev, data->dma.channel);
#if !defined(CONFIG_SOC_SERIES_STM32F1X)
			LL_ADC_ClearFlag_OVR(adc);
#endif /* !defined(CONFIG_SOC_SERIES_STM32F1X) */
			/* No need to invalidate the cache because it's assumed that
			 * the address is in a non-cacheable SRAM region.
			 */
			adc_context_on_sampling_done(&data->ctx, dev);
		} else {
			LOG_ERR("DMA sampling complete, but DMA reported error %d", status);
			data->dma_error = status;
			LL_ADC_REG_StopConversion(adc);
			dma_stop(data->dma.dma_dev, data->dma.channel);
			adc_context_complete(&data->ctx, status);
		}
	}
}
#endif /* CONFIG_ADC_STM32_DMA */

static uint8_t get_reg_value(const struct device *dev, uint32_t reg,
			     uint32_t shift, uint32_t mask)
{
	const struct adc_stm32_cfg *config = dev->config;
	ADC_TypeDef *adc = (ADC_TypeDef *)config->base;

	uintptr_t addr = (uintptr_t)adc + reg;

	return ((*(volatile uint32_t *)addr >> shift) & mask);
}

static void set_reg_value(const struct device *dev, uint32_t reg,
			  uint32_t shift, uint32_t mask, uint32_t value)
{
	const struct adc_stm32_cfg *config = dev->config;
	ADC_TypeDef *adc = (ADC_TypeDef *)config->base;

	uintptr_t addr = (uintptr_t)adc + reg;

	MODIFY_REG(*(volatile uint32_t *)addr, (mask << shift), (value << shift));
}

static int set_resolution(const struct device *dev,
			  const struct adc_sequence *sequence)
{
	const struct adc_stm32_cfg *config = dev->config;
	ADC_TypeDef *adc = (ADC_TypeDef *)config->base;
	uint8_t res_reg_addr = 0xFF;
	uint8_t res_shift = 0;
	uint8_t res_mask = 0;
	uint8_t res_reg_val = 0;
	int i;

	for (i = 0; i < config->res_table_size; i++) {
		if (sequence->resolution == STM32_ADC_GET_REAL_VAL(config->res_table[i])) {
			res_reg_addr = STM32_ADC_GET_REG(config->res_table[i]);
			res_shift = STM32_ADC_GET_SHIFT(config->res_table[i]);
			res_mask = STM32_ADC_GET_MASK(config->res_table[i]);
			res_reg_val = STM32_ADC_GET_REG_VAL(config->res_table[i]);
			break;
		}
	}

	if (i == config->res_table_size) {
		LOG_ERR("Invalid resolution");
		return -EINVAL;
	}

	/*
	 * Some MCUs (like STM32F1x) have no register to configure resolution.
	 * These MCUs have a register address value of 0xFF and should be
	 * ignored.
	 */
	if (res_reg_addr != 0xFF) {
		/*
		 * We don't use LL_ADC_SetResolution and LL_ADC_GetResolution
		 * because they don't strictly use hardware resolution values
		 * and makes internal conversions for some series.
		 * (see stm32h7xx_ll_adc.h)
		 * Instead we set the register ourselves if needed.
		 */
		if (get_reg_value(dev, res_reg_addr, res_shift, res_mask) != res_reg_val) {
			/*
			 * Writing ADC_CFGR1 register while ADEN bit is set
			 * resets RES[1:0] bitfield. We need to disable and enable adc.
			 */
			adc_stm32_disable(adc);
			set_reg_value(dev, res_reg_addr, res_shift, res_mask, res_reg_val);
		}
	}

	return 0;
}

static int start_read(const struct device *dev,
		      const struct adc_sequence *sequence)
{
	const struct adc_stm32_cfg *config = dev->config;
	struct adc_stm32_data *data = dev->data;
	ADC_TypeDef *adc = (ADC_TypeDef *)config->base;
	int err;

	data->buffer = sequence->buffer;
	data->channels = sequence->channels;
	data->channel_count = POPCOUNT(data->channels);
	data->samples_count = 0;

	if (data->channel_count == 0) {
		LOG_ERR("No channels selected");
		return -EINVAL;
	}

	if (data->channels > BIT(STM32_CHANNEL_COUNT) - 1) {
		LOG_ERR("Channels bitmask uses out of range channel");
		return -EINVAL;
	}

#if !defined(CONFIG_SOC_SERIES_STM32C0X) && \
	!defined(CONFIG_SOC_SERIES_STM32F0X) && \
	!defined(CONFIG_SOC_SERIES_STM32G0X) && \
	!defined(CONFIG_SOC_SERIES_STM32L0X) && \
	!defined(CONFIG_SOC_SERIES_STM32WLX)
	if (data->channel_count > ARRAY_SIZE(table_seq_len)) {
		LOG_ERR("Too many channels for sequencer. Max: %d", ARRAY_SIZE(table_seq_len));
		return -EINVAL;
	}
#else
	if (data->channel_count > 1) {
		LOG_ERR("This device only supports single channel sampling");
		return -EINVAL;
	}
#endif

	/* Check and set the resolution */
	err = set_resolution(dev, sequence);
	if (err < 0) {
		return err;
	}

	uint8_t channel_id;
	uint8_t channel_index = 0;

	/* Iterate over selected channels in bitmask keeping track of:
	 * - channel_index: ranging from 0 -> ( data->channel_count - 1 )
	 * - channel_id: ordinal position of channel in data->channels bitmask
	 */
	for (uint32_t channels = data->channels; channels;
		      channels &= ~BIT(channel_id), channel_index++) {
		channel_id = find_lsb_set(channels) - 1;

		uint32_t channel = __LL_ADC_DECIMAL_NB_TO_CHANNEL(channel_id);

		adc_stm32_setup_channel(dev, channel_id);

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
#elif defined(CONFIG_SOC_SERIES_STM32C0X) || defined(CONFIG_SOC_SERIES_STM32G0X)
		/* C0 and G0 in "not fully configurable" sequencer mode */
		LL_ADC_REG_SetSequencerChannels(adc, channel);
		LL_ADC_REG_SetSequencerConfigurable(adc, LL_ADC_REG_SEQ_FIXED);
		while (LL_ADC_IsActiveFlag_CCRDY(adc) == 0) {
		}
		LL_ADC_ClearFlag_CCRDY(adc);
#elif defined(CONFIG_SOC_SERIES_STM32U5X)
		if (adc != ADC4) {
			LL_ADC_REG_SetSequencerRanks(adc, table_rank[channel_index], channel);
			LL_ADC_REG_SetSequencerLength(adc, table_seq_len[channel_index]);
		} else {
			LL_ADC_REG_SetSequencerConfigurable(adc, LL_ADC_REG_SEQ_FIXED);
			LL_ADC_REG_SetSequencerLength(adc,
						      BIT(__LL_ADC_CHANNEL_TO_DECIMAL_NB(channel)));
		}
#else
		LL_ADC_REG_SetSequencerRanks(adc, table_rank[channel_index], channel);
		LL_ADC_REG_SetSequencerLength(adc, table_seq_len[channel_index]);
#endif
	}

	err = check_buffer(sequence, data->channel_count);
	if (err) {
		return err;
	}

#if defined(CONFIG_SOC_SERIES_STM32C0X) || \
	defined(CONFIG_SOC_SERIES_STM32G0X) || \
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

#if !defined(CONFIG_SOC_SERIES_STM32F1X)
	LL_ADC_ClearFlag_OVR(adc);
#endif /* !defined(CONFIG_SOC_SERIES_STM32F1X) */

#if !defined(CONFIG_ADC_STM32_DMA)
#if defined(CONFIG_SOC_SERIES_STM32C0X) || \
	defined(CONFIG_SOC_SERIES_STM32F0X) || \
	defined(STM32F3X_ADC_V1_1) || \
	defined(CONFIG_SOC_SERIES_STM32L0X) || \
	defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32G4X) || \
	defined(CONFIG_SOC_SERIES_STM32H5X) || \
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
#endif /* CONFIG_ADC_STM32_DMA */

	/* This call will start the DMA */
	adc_context_start_read(&data->ctx, sequence);

	int result = adc_context_wait_for_completion(&data->ctx);

#ifdef CONFIG_ADC_STM32_DMA
	/* check if there's anything wrong with dma start */
	result = (data->dma_error ? data->dma_error : result);
#endif

	return result;
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_stm32_data *data =
		CONTAINER_OF(ctx, struct adc_stm32_data, ctx);

	data->repeat_buffer = data->buffer;

#ifdef CONFIG_ADC_STM32_DMA
	adc_stm32_dma_start(data->dev, data->buffer, data->channel_count);
#endif
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

#ifndef CONFIG_ADC_STM32_DMA
static void adc_stm32_isr(const struct device *dev)
{
	struct adc_stm32_data *data = dev->data;
	const struct adc_stm32_cfg *config =
		(const struct adc_stm32_cfg *)dev->config;
	ADC_TypeDef *adc = config->base;

	*data->buffer++ = LL_ADC_REG_ReadConversionData32(adc);

	/* ISR is triggered after each conversion, and at the end-of-sequence. */
	if (++data->samples_count == data->channel_count) {
		data->samples_count = 0;
		adc_context_on_sampling_done(&data->ctx, dev);
	}

	LOG_DBG("%s ISR triggered.", dev->name);
}
#endif /* !CONFIG_ADC_STM32_DMA */

static void adc_context_on_complete(struct adc_context *ctx, int status)
{
	struct adc_stm32_data *data =
		CONTAINER_OF(ctx, struct adc_stm32_data, ctx);

	ARG_UNUSED(status);

	adc_stm32_teardown_channels(data->dev);
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
#elif defined(CONFIG_SOC_SERIES_STM32C0X)
	LL_ADC_SetSamplingTimeCommonChannels(adc, LL_ADC_SAMPLINGTIME_COMMON_1,
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
#elif defined(CONFIG_SOC_SERIES_STM32U5X)
	if (adc != ADC4) {
		LL_ADC_SetChannelSamplingTime(adc,
					      __LL_ADC_DECIMAL_NB_TO_CHANNEL(id),
					      table_samp_time[acq_time_index]);
	} else {
		LL_ADC_SetSamplingTimeCommonChannels(adc,
						     LL_ADC_SAMPLINGTIME_COMMON_1,
				       table_samp_time[acq_time_index]);
	}
#else
	LL_ADC_SetChannelSamplingTime(adc,
		__LL_ADC_DECIMAL_NB_TO_CHANNEL(id),
		table_samp_time[acq_time_index]);
#endif
}

static int adc_stm32_channel_setup(const struct device *dev,
				   const struct adc_channel_cfg *channel_cfg)
{
#if defined(CONFIG_SOC_SERIES_STM32C0X) || \
	defined(CONFIG_SOC_SERIES_STM32F0X) || \
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
#if defined(CONFIG_SOC_SERIES_STM32C0X) || \
	defined(CONFIG_SOC_SERIES_STM32F0X) || \
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

	LOG_DBG("Initializing %s", dev->name);

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	data->dev = dev;
#if defined(CONFIG_SOC_SERIES_STM32C0X) || \
	defined(CONFIG_SOC_SERIES_STM32F0X) || \
	defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32L0X)
	/*
	 * All conversion time for all channels on one ADC instance for F0 and
	 * L0 series chips has to be the same. For C0 and G0 currently only one
	 * of the two available common channel conversion times is used.
	 * This additional variable is for checking if the conversion time
	 * selection of all channels on one ADC instance is the same.
	 */
	data->acq_time_index = -1;
#endif

	if (clock_control_on(clk,
		(clock_control_subsys_t) &config->pclken) != 0) {
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

#ifdef CONFIG_ADC_STM32_DMA
	if ((data->dma.dma_dev != NULL) &&
	    !device_is_ready(data->dma.dma_dev)) {
		LOG_ERR("%s device not ready", data->dma.dma_dev->name);
		return -ENODEV;
	}
#endif

#if defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32G4X) || \
	defined(CONFIG_SOC_SERIES_STM32H5X) || \
	defined(CONFIG_SOC_SERIES_STM32H7X) || \
	defined(CONFIG_SOC_SERIES_STM32U5X)
	/*
	 * L4, WB, G4, H5, H7 and U5 series STM32 needs to be awaken from deep sleep
	 * mode, and restore its calibration parameters if there are some
	 * previously stored calibration parameters.
	 */

	LL_ADC_DisableDeepPowerDown(adc);
#elif defined(CONFIG_SOC_SERIES_STM32WLX)
	/* The ADC clock must be disabled by clock gating during CPU1 sleep/stop */
	LL_APB2_GRP1_DisableClockSleep(LL_APB2_GRP1_PERIPH_ADC);
#endif
	/*
	 * C0, F3, L4, WB, G0 and G4 ADC modules need some time
	 * to be stabilized before performing any enable or calibration actions.
	 */
#if defined(STM32F3X_ADC_V1_1) || \
	defined(CONFIG_SOC_SERIES_STM32C0X) || \
	defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32G4X) || \
	defined(CONFIG_SOC_SERIES_STM32H5X) || \
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
#elif defined(CONFIG_SOC_SERIES_STM32C0X) || \
	defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32G4X) || \
	defined(CONFIG_SOC_SERIES_STM32H7X)
	LL_ADC_SetCommonClock(__LL_ADC_COMMON_INSTANCE(adc),
			      LL_ADC_CLOCK_SYNC_PCLK_DIV4);
#elif defined(CONFIG_SOC_SERIES_STM32H5X)
	LL_ADC_SetCommonClock(__LL_ADC_COMMON_INSTANCE(adc),
			      LL_ADC_CLOCK_ASYNC_DIV6);
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
	adc_stm32_disable(adc);
	adc_stm32_calib(dev);
#endif

#if defined(CONFIG_SOC_SERIES_STM32C0X) || \
	defined(CONFIG_SOC_SERIES_STM32F0X) || \
	defined(CONFIG_SOC_SERIES_STM32L0X) || \
	defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32G4X) || \
	defined(CONFIG_SOC_SERIES_STM32H5X) || \
	defined(CONFIG_SOC_SERIES_STM32H7X) || \
	defined(CONFIG_SOC_SERIES_STM32U5X) || \
	defined(CONFIG_SOC_SERIES_STM32WLX)
	if (LL_ADC_IsActiveFlag_ADRDY(adc)) {
		LL_ADC_ClearFlag_ADRDY(adc);
	}
#endif

#if defined(CONFIG_SOC_SERIES_STM32C0X) || \
	defined(CONFIG_SOC_SERIES_STM32F0X) || \
	defined(STM32F3X_ADC_V1_1) || \
	defined(CONFIG_SOC_SERIES_STM32L0X) || \
	defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32G4X) || \
	defined(CONFIG_SOC_SERIES_STM32H5X) || \
	defined(CONFIG_SOC_SERIES_STM32H7X) || \
	defined(CONFIG_SOC_SERIES_STM32U5X) || \
	defined(CONFIG_SOC_SERIES_STM32WLX)
	/*
	 * ADC modules on these series have to wait for some cycles to be
	 * enabled.
	 */
	uint32_t adc_rate, wait_cycles;

	if (clock_control_get_rate(clk,
		(clock_control_subsys_t) &config->pclken, &adc_rate) < 0) {
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
#if defined(CONFIG_SOC_SERIES_STM32G4X)
	return LL_ADC_IsActiveFlag_EOC(adc) ||
#else
	return LL_ADC_IsActiveFlag_EOCS(adc) ||
#endif /* CONFIG_SOC_SERIES_STM32G4X */
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

#define ADC_STM32_IRQ_CONFIG(index)
#define ADC_STM32_IRQ_FUNC(index)					\
	.irq_cfg_func = adc_stm32_irq_init,
#define ADC_DMA_CHANNEL(id, dir, DIR, src, dest)

#elif defined(CONFIG_ADC_STM32_DMA) /* !CONFIG_ADC_STM32_SHARED_IRQS */

#define ADC_DMA_CHANNEL_INIT(index, name, dir_cap, src_dev, dest_dev)			\
	.dma = {									\
		.dma_dev = DEVICE_DT_GET(STM32_DMA_CTLR(index, name)),			\
		.channel = DT_INST_DMAS_CELL_BY_NAME(index, name, channel),		\
		.dma_cfg = {								\
			.dma_slot = STM32_DMA_SLOT(index, name, slot),			\
			.channel_direction = STM32_DMA_CONFIG_DIRECTION(		\
				STM32_DMA_CHANNEL_CONFIG(index, name)),			\
			.source_data_size = STM32_DMA_CONFIG_##src_dev##_DATA_SIZE(	\
				STM32_DMA_CHANNEL_CONFIG(index, name)),			\
			.dest_data_size = STM32_DMA_CONFIG_##dest_dev##_DATA_SIZE(	\
				STM32_DMA_CHANNEL_CONFIG(index, name)),			\
			.source_burst_length = 1,       /* SINGLE transfer */		\
			.dest_burst_length = 1,         /* SINGLE transfer */		\
			.channel_priority = STM32_DMA_CONFIG_PRIORITY(			\
				STM32_DMA_CHANNEL_CONFIG(index, name)),			\
			.dma_callback = dma_callback,					\
			.block_count = 2,						\
		},									\
		.src_addr_increment = STM32_DMA_CONFIG_##src_dev##_ADDR_INC(		\
			STM32_DMA_CHANNEL_CONFIG(index, name)),				\
		.dst_addr_increment = STM32_DMA_CONFIG_##dest_dev##_ADDR_INC(		\
			STM32_DMA_CHANNEL_CONFIG(index, name)),				\
	}

#define ADC_DMA_CHANNEL(id, dir, DIR, src, dest)					\
	COND_CODE_1(DT_INST_DMAS_HAS_NAME(id, dir),					\
		    (ADC_DMA_CHANNEL_INIT(id, dir, DIR, src, dest)),			\
		    (EMPTY))

#define ADC_STM32_IRQ_CONFIG(index)					\
static void adc_stm32_cfg_func_##index(void){ EMPTY }
#define ADC_STM32_IRQ_FUNC(index)					\
	.irq_cfg_func = adc_stm32_cfg_func_##index,

#else /* CONFIG_ADC_STM32_DMA */

#define ADC_STM32_IRQ_CONFIG(index)					\
static void adc_stm32_cfg_func_##index(void)				\
{									\
	IRQ_CONNECT(DT_INST_IRQN(index),				\
		    DT_INST_IRQ(index, priority),			\
		    adc_stm32_isr, DEVICE_DT_INST_GET(index), 0);	\
	irq_enable(DT_INST_IRQN(index));				\
}
#define ADC_STM32_IRQ_FUNC(index)					\
	.irq_cfg_func = adc_stm32_cfg_func_##index,
#define ADC_DMA_CHANNEL(id, dir, DIR, src, dest)

#endif /* CONFIG_ADC_STM32_DMA && CONFIG_ADC_STM32_SHARED_IRQS */


#define ADC_STM32_INIT(index)						\
									\
PINCTRL_DT_INST_DEFINE(index);						\
									\
ADC_STM32_IRQ_CONFIG(index)						\
									\
static const struct adc_stm32_cfg adc_stm32_cfg_##index = {		\
	.base = (ADC_TypeDef *)DT_INST_REG_ADDR(index),			\
	ADC_STM32_IRQ_FUNC(index)					\
	.pclken = {							\
		.enr = DT_INST_CLOCKS_CELL(index, bits),		\
		.bus = DT_INST_CLOCKS_CELL(index, bus),			\
	},								\
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),			\
	.temp_channel = DT_INST_PROP_OR(index, temp_channel, 0xFF),	\
	.vref_channel = DT_INST_PROP_OR(index, vref_channel, 0xFF),	\
	.vbat_channel = DT_INST_PROP_OR(index, vbat_channel, 0xFF),	\
	.res_table_size = DT_INST_PROP_LEN(index, resolutions),		\
	.res_table = DT_INST_PROP(index, resolutions),			\
};									\
									\
static struct adc_stm32_data adc_stm32_data_##index = {			\
	ADC_CONTEXT_INIT_TIMER(adc_stm32_data_##index, ctx),		\
	ADC_CONTEXT_INIT_LOCK(adc_stm32_data_##index, ctx),		\
	ADC_CONTEXT_INIT_SYNC(adc_stm32_data_##index, ctx),		\
	ADC_DMA_CHANNEL(index, dmamux, NULL, PERIPHERAL, MEMORY)	\
};									\
									\
DEVICE_DT_INST_DEFINE(index,						\
		    &adc_stm32_init, NULL,				\
		    &adc_stm32_data_##index, &adc_stm32_cfg_##index,	\
		    POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,		\
		    &api_stm32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ADC_STM32_INIT)
