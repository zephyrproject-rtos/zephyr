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
#include <zephyr/toolchain.h>
#include <soc.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <stm32_ll_adc.h>
#include <stm32_ll_system.h>
#if defined(CONFIG_SOC_SERIES_STM32N6X) || \
	defined(CONFIG_SOC_SERIES_STM32U5X)
#include <stm32_ll_pwr.h>
#endif /* CONFIG_SOC_SERIES_STM32U5X */

#ifdef CONFIG_ADC_STM32_DMA
#include <zephyr/drivers/dma/dma_stm32.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/toolchain.h>
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
#include <zephyr/mem_mgmt/mem_attr.h>

#if defined(CONFIG_SOC_SERIES_STM32H7X) || defined(CONFIG_SOC_SERIES_STM32H7RSX)
#include <zephyr/dt-bindings/memory-attr/memory-attr-arm.h>
#include <stm32_ll_system.h>
#endif

#include <zephyr/linker/linker-defs.h>

/* Here are some redefinitions of ADC versions for better readability */
#if defined(CONFIG_SOC_SERIES_STM32F3X)
#if defined(ADC1_V2_5)
#define STM32F37X_ADC /* F37x */
#elif defined(ADC5_V1_1)
#define STM32F3XX_ADC /* Other F3xx */
#endif /* ADC5_V1_1 */
#elif defined(CONFIG_SOC_SERIES_STM32H7X)
#if defined(ADC_VER_V5_V90)
#define STM32H72X_ADC /* H72x and H73x */
#elif defined(ADC_VER_V5_X)
#define STM32H74X_ADC /* H74x and H75x */
#elif defined(ADC_VER_V5_3)
#define STM32H7AX_ADC /* H7Ax and H7Bx */
#endif /* ADC_VER_V5_3 */
#endif /* CONFIG_SOC_SERIES_STM32H7X */

/*
 * Other ADC versions:
 * compat st_stm32f1_adc -> STM32F1, F37x (ADC1_V2_5)
 * compat st_stm32f4_adc -> STM32F2, F4, F7, L1
 */

/* Clock source values */
#define SYNC			1
#define ASYNC			2

/* Sequencer type */
#define NOT_FULLY_CONFIGURABLE	0
#define FULLY_CONFIGURABLE	1

/* Oversampler type */
#define OVERSAMPLER_NONE	0
#define OVERSAMPLER_MINIMAL	1
#define OVERSAMPLER_EXTENDED	2

#define ANY_NUM_COMMON_SAMPLING_TIME_CHANNELS_IS(value) \
	(DT_INST_FOREACH_STATUS_OKAY_VARGS(IS_EQ_PROP_OR, \
					   num_sampling_time_common_channels,\
					   0, value) 0)

#define ANY_ADC_SEQUENCER_TYPE_IS(value) \
	(DT_INST_FOREACH_STATUS_OKAY_VARGS(IS_EQ_STRING_PROP, \
					   st_adc_sequencer,\
					   value) 0)

#define ANY_ADC_OVERSAMPLER_TYPE_IS(value) \
	(DT_INST_FOREACH_STATUS_OKAY_VARGS(IS_EQ_STRING_PROP, \
					   st_adc_oversampler,\
					   value) 0)

#define IS_EQ_PROP_OR(inst, prop, default_value, compare_value) \
	IS_EQ(DT_INST_PROP_OR(inst, prop, default_value), compare_value) ||

#define IS_EQ_STRING_PROP(inst, prop, compare_value) \
	IS_EQ(DT_INST_STRING_UPPER_TOKEN(inst, prop), compare_value) ||

/* reference voltage for the ADC */
#define STM32_ADC_VREF_MV DT_INST_PROP(0, vref_mv)

#if ANY_ADC_SEQUENCER_TYPE_IS(FULLY_CONFIGURABLE)

#if defined(LL_ADC_REG_RANK_28)
#define MAX_RANK	28
#elif defined(LL_ADC_REG_RANK_27)
#define MAX_RANK	27
#else
#define MAX_RANK	16
#endif

#define RANK(i, _)	_CONCAT_1(LL_ADC_REG_RANK_, UTIL_INC(i))
static const uint32_t table_rank[] = {
	LISTIFY(MAX_RANK, RANK, (,))};

#define SEQ_LEN(i, _)	_CONCAT_2(LL_ADC_REG_SEQ_SCAN_ENABLE_, UTIL_INC(UTIL_INC(i)), RANKS)
/* Length of this array signifies the maximum sequence length */
static const uint32_t table_seq_len[] = {
	LL_ADC_REG_SEQ_SCAN_DISABLE,
	LISTIFY(UTIL_DEC(MAX_RANK), SEQ_LEN, (,))
};

#endif /* ANY_ADC_SEQUENCER_TYPE_IS(FULLY_CONFIGURABLE) */

/* Number of different sampling time values */
#define STM32_NB_SAMPLING_TIME	8

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

#if defined(CONFIG_SOC_SERIES_STM32N6X)
typedef uint32_t adc_data_size_t;
#else
typedef uint16_t adc_data_size_t;
#endif

struct adc_stm32_data {
	struct adc_context ctx;
	const struct device *dev;
	adc_data_size_t *buffer;
	adc_data_size_t *repeat_buffer;

	uint8_t resolution;
	uint32_t channels;
	uint8_t channel_count;
	uint8_t samples_count;
	int8_t acq_time_index[2];

#ifdef CONFIG_ADC_STM32_DMA
	volatile int dma_error;
	struct stream dma;
#endif
};

struct adc_stm32_cfg {
	ADC_TypeDef *base;
	void (*irq_cfg_func)(void);
	const struct stm32_pclken *pclken;
	size_t pclk_len;
	uint32_t clk_prescaler;
	const struct pinctrl_dev_config *pcfg;
	const uint16_t sampling_time_table[STM32_NB_SAMPLING_TIME];
	int8_t num_sampling_time_common_channels;
	int8_t sequencer_type;
	int8_t oversampler_type;
	int8_t res_table_size;
	const uint32_t res_table[];
};

#ifdef CONFIG_ADC_STM32_DMA
static void adc_stm32_enable_dma_support(ADC_TypeDef *adc)
{
	/* Allow ADC to create DMA request and set to one-shot mode as implemented in HAL drivers */
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_adc)
	LL_ADC_REG_SetDMATransfer(adc, LL_ADC_REG_DMA_TRANSFER_UNLIMITED);
#elif defined(CONFIG_SOC_SERIES_STM32H7X) || \
	defined(CONFIG_SOC_SERIES_STM32N6X) || \
	defined(CONFIG_SOC_SERIES_STM32U5X)
	/* H72x ADC3 and U5 ADC4 are different from the rest, but this call works also for them,
	 * so no need to call their specific function
	 */
	LL_ADC_REG_SetDataTransferMode(adc, LL_ADC_REG_DMA_TRANSFER_LIMITED);
#else
	/* Default mechanism for other MCUs */
	LL_ADC_REG_SetDMATransfer(adc, LL_ADC_REG_DMA_TRANSFER_LIMITED);
#endif
}

static int adc_stm32_dma_start(const struct device *dev,
			       void *buffer, size_t channel_count)
{
	const struct adc_stm32_cfg *config = dev->config;
	ADC_TypeDef *adc = config->base;
	struct adc_stm32_data *data = dev->data;
	struct dma_block_config *blk_cfg;
	int ret;

	struct stream *dma = &data->dma;

	blk_cfg = &dma->dma_blk_cfg;

	/* prepare the block */
	blk_cfg->block_size = channel_count * sizeof(adc_data_size_t);

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

	adc_stm32_enable_dma_support(adc);

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
 *		zephyr,memory-attr = <( DT_MEM_ARM(ATTR_MPU_RAM_NOCACHE) | ... )>;
 *	};
 */
static bool buf_in_nocache(uintptr_t buf __maybe_unused, size_t len_bytes __maybe_unused)
{
	bool buf_within_nocache = false;

#ifdef CONFIG_NOCACHE_MEMORY
	buf_within_nocache = (buf >= ((uintptr_t)_nocache_ram_start)) &&
		((buf + len_bytes - 1) <= ((uintptr_t)_nocache_ram_end));
	if (buf_within_nocache) {
		return true;
	}
#endif /* CONFIG_NOCACHE_MEMORY */

#ifdef CONFIG_MEM_ATTR
	buf_within_nocache = mem_attr_check_buf(
		(void *)buf, len_bytes, DT_MEM_ARM(ATTR_MPU_RAM_NOCACHE)) == 0;
#endif /* CONFIG_MEM_ATTR */

	return buf_within_nocache;
}
#endif /* defined(CONFIG_ADC_STM32_DMA) && defined(CONFIG_SOC_SERIES_STM32H7X) */

static int check_buffer(const struct adc_sequence *sequence,
			     uint8_t active_channels)
{
	size_t needed_buffer_size;

	needed_buffer_size = active_channels * sizeof(adc_data_size_t);

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
	if (!buf_in_nocache((uintptr_t)sequence->buffer, needed_buffer_size)) {
		LOG_ERR("Supplied buffer is not in a non-cacheable region according to DTS.");
		return -EINVAL;
	}
#endif

	return 0;
}

/*
 * Enable ADC peripheral, and wait until ready if required by SOC.
 */
static int adc_stm32_enable(ADC_TypeDef *adc)
{
	if (LL_ADC_IsEnabled(adc) == 1UL) {
		return 0;
	}

#if !DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_adc) && \
	!DT_HAS_COMPAT_STATUS_OKAY(st_stm32f4_adc)
	LL_ADC_ClearFlag_ADRDY(adc);
	LL_ADC_Enable(adc);

	/*
	 * Enabling ADC modules in many series may fail if they are
	 * still not stabilized, this will wait for a short time (about 1ms)
	 * to ensure ADC modules are properly enabled.
	 */
	uint32_t count_timeout = 0;

	while (LL_ADC_IsActiveFlag_ADRDY(adc) == 0) {
#ifdef CONFIG_SOC_SERIES_STM32F0X
		/* For F0, continue to write ADEN=1 until ADRDY=1 */
		if (LL_ADC_IsEnabled(adc) == 0UL) {
			LL_ADC_Enable(adc);
		}
#endif /* CONFIG_SOC_SERIES_STM32F0X */
		count_timeout++;
		k_busy_wait(100);
		if (count_timeout >= 10) {
			return -ETIMEDOUT;
		}
	}
#else
	/*
	 * On STM32F1, F2, F37x, F4, F7 and L1, do not re-enable the ADC.
	 * On F1 and F37x if ADON holds 1 (LL_ADC_IsEnabled is true) and 1 is
	 * written, then conversion starts. That's not what is expected.
	 */
	LL_ADC_Enable(adc);
#endif

	return 0;
}

static void adc_stm32_start_conversion(const struct device *dev)
{
	const struct adc_stm32_cfg *config = dev->config;
	ADC_TypeDef *adc = config->base;

	LOG_DBG("Starting conversion");

#if !defined(CONFIG_SOC_SERIES_STM32F1X) && \
	!DT_HAS_COMPAT_STATUS_OKAY(st_stm32f4_adc)
	LL_ADC_REG_StartConversion(adc);
#else
	LL_ADC_REG_StartConversionSWStart(adc);
#endif
}

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

#if !DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_adc) && \
	!DT_HAS_COMPAT_STATUS_OKAY(st_stm32f4_adc)
	if (LL_ADC_REG_IsConversionOngoing(adc)) {
		LL_ADC_REG_StopConversion(adc);
		while (LL_ADC_REG_IsConversionOngoing(adc)) {
		}
	}
#endif

#if !defined(CONFIG_SOC_SERIES_STM32C0X) && \
	!defined(CONFIG_SOC_SERIES_STM32F0X) && \
	!DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_adc) && \
	!DT_HAS_COMPAT_STATUS_OKAY(st_stm32f4_adc) && \
	!defined(CONFIG_SOC_SERIES_STM32G0X) && \
	!defined(CONFIG_SOC_SERIES_STM32L0X) && \
	!defined(CONFIG_SOC_SERIES_STM32U0X) && \
	!defined(CONFIG_SOC_SERIES_STM32WBAX) && \
	!defined(CONFIG_SOC_SERIES_STM32WLX)
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

#if !DT_HAS_COMPAT_STATUS_OKAY(st_stm32f4_adc)

#define HAS_CALIBRATION

/* Number of ADC clock cycles to wait before of after starting calibration */
#if defined(LL_ADC_DELAY_CALIB_ENABLE_ADC_CYCLES)
#define ADC_DELAY_CALIB_ADC_CYCLES	LL_ADC_DELAY_CALIB_ENABLE_ADC_CYCLES
#elif defined(LL_ADC_DELAY_ENABLE_CALIB_ADC_CYCLES)
#define ADC_DELAY_CALIB_ADC_CYCLES	LL_ADC_DELAY_ENABLE_CALIB_ADC_CYCLES
#elif defined(LL_ADC_DELAY_DISABLE_CALIB_ADC_CYCLES)
#define ADC_DELAY_CALIB_ADC_CYCLES	LL_ADC_DELAY_DISABLE_CALIB_ADC_CYCLES
#endif

static void adc_stm32_calibration_delay(const struct device *dev)
{
	/*
	 * Calibration of F1 and F3 (ADC1_V2_5) must start two cycles after ADON
	 * is set.
	 * Other ADC modules have to wait for some cycles after calibration to
	 * be enabled.
	 */
	const struct adc_stm32_cfg *config = dev->config;
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	uint32_t adc_rate, wait_cycles;

	if (clock_control_get_rate(clk,
		(clock_control_subsys_t) &config->pclken[0], &adc_rate) < 0) {
		LOG_ERR("ADC clock rate get error.");
	}

	if (adc_rate == 0) {
		LOG_ERR("ADC Clock rate null");
		return;
	}
	wait_cycles = SystemCoreClock / adc_rate *
		      ADC_DELAY_CALIB_ADC_CYCLES;

	for (int i = wait_cycles; i >= 0; i--) {
	}
}

#if defined(CONFIG_SOC_SERIES_STM32N6X)
/* Number of ADC measurement during calibration procedure */
#define ADC_CALIBRATION_STEPS (8U)

static void adc_stm32_calibration_measure(ADC_TypeDef *adc, uint32_t *calibration_factor)
{
	uint32_t calib_step;
	uint32_t calib_factor_avg = 0;
	uint8_t done = 0;

	do {
		for (calib_step = 0; calib_step < ADC_CALIBRATION_STEPS; calib_step++) {
			LL_ADC_REG_StartConversion(adc);
			while (LL_ADC_REG_IsConversionOngoing(adc) != 0UL) {
			}

			calib_factor_avg += LL_ADC_REG_ReadConversionData32(adc);
		}

		/* Compute the average data */
		calib_factor_avg /= ADC_CALIBRATION_STEPS;

		if ((calib_factor_avg == 0) && (LL_ADC_IsCalibrationOffsetEnabled(adc) == 0UL)) {
			/* If average is 0 and offset is disabled
			 * set offset and repeat measurements
			 */
			LL_ADC_EnableCalibrationOffset(adc);
		} else {
			*calibration_factor = (uint32_t)(calib_factor_avg);
			done = 1;
		}
	} while (done == 0);
}
#endif

static void adc_stm32_calibration_start(const struct device *dev)
{
	const struct adc_stm32_cfg *config =
		(const struct adc_stm32_cfg *)dev->config;
	ADC_TypeDef *adc = config->base;

#if defined(STM32F3XX_ADC) || \
	defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32H5X) || \
	defined(CONFIG_SOC_SERIES_STM32H7RSX) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32G4X)
	LL_ADC_StartCalibration(adc, LL_ADC_SINGLE_ENDED);
#elif defined(CONFIG_SOC_SERIES_STM32C0X) || \
	defined(CONFIG_SOC_SERIES_STM32F0X) || \
	DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_adc) || \
	defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32L0X) || \
	defined(CONFIG_SOC_SERIES_STM32U0X) || \
	defined(CONFIG_SOC_SERIES_STM32WLX) || \
	defined(CONFIG_SOC_SERIES_STM32WBAX)

	LL_ADC_StartCalibration(adc);
#elif defined(CONFIG_SOC_SERIES_STM32U5X)
	if (adc != ADC4) {
		uint32_t dev_id = LL_DBGMCU_GetDeviceID();
		uint32_t rev_id = LL_DBGMCU_GetRevisionID();

		/* Some U5 implement an extended calibration to enhance ADC performance.
		 * It is not available for ADC4.
		 * It is available on all U5 except U575/585 (dev ID 482) revision X (rev ID 2001).
		 * The code below applies the procedure described in RM0456 in the ADC chapter:
		 * "Extended calibration mode"
		 */
		if ((dev_id != 0x482UL) && (rev_id != 0x2001UL)) {
			adc_stm32_enable(adc);
			MODIFY_REG(adc->CR, ADC_CR_CALINDEX, 0x9UL << ADC_CR_CALINDEX_Pos);
			__DMB();
			MODIFY_REG(adc->CALFACT2, 0xFFFFFF00UL, 0x03021100UL);
			__DMB();
			SET_BIT(adc->CALFACT, ADC_CALFACT_LATCH_COEF);
			adc_stm32_disable(adc);
		}
	}
	LL_ADC_StartCalibration(adc, LL_ADC_CALIB_OFFSET);
#elif defined(CONFIG_SOC_SERIES_STM32H7X)
	LL_ADC_StartCalibration(adc, LL_ADC_CALIB_OFFSET, LL_ADC_SINGLE_ENDED);
#elif defined(CONFIG_SOC_SERIES_STM32N6X)
	uint32_t calibration_factor;
	/* Start ADC calibration */
	LL_ADC_StartCalibration(adc, LL_ADC_SINGLE_ENDED);
	/* Disable additional offset before calibration start */
	LL_ADC_DisableCalibrationOffset(adc);

	adc_stm32_calibration_measure(adc, &calibration_factor);

	LL_ADC_SetCalibrationFactor(adc, LL_ADC_SINGLE_ENDED, calibration_factor);
	LL_ADC_StopCalibration(adc);
#endif
	/* Make sure ADCAL is cleared before returning for proper operations
	 * on the ADC control register, for enabling the peripheral for example
	 */
	while (LL_ADC_IsCalibrationOnGoing(adc)) {
	}
}

static int adc_stm32_calibrate(const struct device *dev)
{
	const struct adc_stm32_cfg *config =
		(const struct adc_stm32_cfg *)dev->config;
	ADC_TypeDef *adc = config->base;
	int err;

#if defined(CONFIG_ADC_STM32_DMA)
#if defined(CONFIG_SOC_SERIES_STM32C0X) || \
	defined(CONFIG_SOC_SERIES_STM32F0X) || \
	defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32H7RSX) || \
	defined(CONFIG_SOC_SERIES_STM32L0X) || \
	defined(CONFIG_SOC_SERIES_STM32U0X) || \
	defined(CONFIG_SOC_SERIES_STM32WBAX) || \
	defined(CONFIG_SOC_SERIES_STM32WLX)
	/* Make sure DMA is disabled before starting calibration */
	LL_ADC_REG_SetDMATransfer(adc, LL_ADC_REG_DMA_TRANSFER_NONE);
#elif defined(CONFIG_SOC_SERIES_STM32U5X)
	if (adc == ADC4) {
		/* Make sure DMA is disabled before starting calibration */
		LL_ADC_REG_SetDMATransfer(adc, LL_ADC_REG_DMA_TRANSFER_NONE);
	}
#endif /* CONFIG_SOC_SERIES_* */
#endif /* CONFIG_ADC_STM32_DMA */

#if !DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_adc) && \
	!defined(CONFIG_SOC_SERIES_STM32N6X)
	adc_stm32_disable(adc);
	adc_stm32_calibration_start(dev);
	adc_stm32_calibration_delay(dev);
#endif /* !DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_adc) */

	err = adc_stm32_enable(adc);
	if (err < 0) {
		return err;
	}

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_adc) || \
	defined(CONFIG_SOC_SERIES_STM32N6X)
	adc_stm32_calibration_delay(dev);
	adc_stm32_calibration_start(dev);
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_adc) */

#if defined(CONFIG_SOC_SERIES_STM32H7X) && \
	defined(CONFIG_CPU_CORTEX_M7)
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
#endif /* CONFIG_SOC_SERIES_STM32H7X */

	return 0;
}
#endif /* !DT_HAS_COMPAT_STATUS_OKAY(st_stm32f4_adc) */

#if !defined(CONFIG_SOC_SERIES_STM32F0X) && \
	!defined(CONFIG_SOC_SERIES_STM32F1X) && \
	!defined(CONFIG_SOC_SERIES_STM32F3X) && \
	!DT_HAS_COMPAT_STATUS_OKAY(st_stm32f4_adc)

#define HAS_OVERSAMPLING

#if defined(LL_ADC_OVS_SHIFT_RIGHT_10)
#define MAX_OVS_SHIFT	10
#else
#define MAX_OVS_SHIFT	8
#endif

#define OVS_SHIFT(i, _)	_CONCAT_1(LL_ADC_OVS_SHIFT_RIGHT_, UTIL_INC(i))
static const uint32_t table_oversampling_shift[] = {
	LL_ADC_OVS_SHIFT_NONE,
	LISTIFY(MAX_OVS_SHIFT, OVS_SHIFT, (,))
};

#if ANY_ADC_OVERSAMPLER_TYPE_IS(OVERSAMPLER_MINIMAL)
#define OVS_RATIO(n)		LL_ADC_OVS_RATIO_##n
static const uint32_t table_oversampling_ratio[] = {
	0,
	OVS_RATIO(2),
	OVS_RATIO(4),
	OVS_RATIO(8),
	OVS_RATIO(16),
	OVS_RATIO(32),
	OVS_RATIO(64),
	OVS_RATIO(128),
	OVS_RATIO(256),
};
#endif

/*
 * Function to configure the oversampling scope. It is basically a wrapper over
 * LL_ADC_SetOverSamplingScope() which in addition stops the ADC if needed.
 */
static void adc_stm32_oversampling_scope(ADC_TypeDef *adc, uint32_t ovs_scope)
{
#if defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32L0X) || \
	defined(CONFIG_SOC_SERIES_STM32WLX)
	/*
	 * Setting OVS bits is conditioned to ADC state: ADC must be disabled
	 * or enabled without conversion on going : disable it, it will stop.
	 * For the G0 series, ADC must be disabled to prevent CKMODE bitfield
	 * from getting reset, see errata ES0418 section 2.6.4.
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
	/*
	 * setting OVS bits is conditioned to ADC state: ADC must be disabled
	 * or enabled without conversion on going : disable it, it will stop
	 */
	if ((LL_ADC_GetOverSamplingRatio(adc) == ratio)
	    && (LL_ADC_GetOverSamplingShift(adc) == shift)) {
		return;
	}
	adc_stm32_disable(adc);

	LL_ADC_ConfigOverSamplingRatioShift(adc, ratio, shift);
}

/*
 * Function to configure the oversampling ratio and shift using stm32 LL
 * ratio is directly the sequence->oversampling (a 2^n value)
 * shift is the corresponding LL_ADC_OVS_SHIFT_RIGHT_x constant
 */
static int adc_stm32_oversampling(const struct device *dev, uint8_t ratio)
{
	const struct adc_stm32_cfg *config = dev->config;
	ADC_TypeDef *adc = config->base;

	if (ratio == 0) {
		adc_stm32_oversampling_scope(adc, LL_ADC_OVS_DISABLE);
		return 0;
	} else if (ratio < ARRAY_SIZE(table_oversampling_shift)) {
		adc_stm32_oversampling_scope(adc, LL_ADC_OVS_GRP_REGULAR_CONTINUED);
	} else {
		LOG_ERR("Invalid oversampling");
		return -EINVAL;
	}

	uint32_t shift = table_oversampling_shift[ratio];

#if ANY_ADC_OVERSAMPLER_TYPE_IS(OVERSAMPLER_MINIMAL)
	if (config->oversampler_type == OVERSAMPLER_MINIMAL) {
		/* the LL function expects a value LL_ADC_OVS_RATIO_x */
		adc_stm32_oversampling_ratioshift(adc, table_oversampling_ratio[ratio], shift);
	}
#endif

#if ANY_ADC_OVERSAMPLER_TYPE_IS(OVERSAMPLER_EXTENDED)
	if (config->oversampler_type == OVERSAMPLER_EXTENDED) {
		/* the LL function expects a value from 1 to 1024 */
		adc_stm32_oversampling_ratioshift(adc, 1 << ratio, shift);
	}
#endif

	return 0;
}
#endif /* CONFIG_SOC_SERIES_STM32xxx */

#ifdef CONFIG_ADC_STM32_DMA
static void dma_callback(const struct device *dev, void *user_data,
			 uint32_t channel, int status)
{
	/* user_data directly holds the adc device */
	struct adc_stm32_data *data = user_data;
#if !DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_adc) /* Avoid unused variables */
	const struct adc_stm32_cfg *config = data->dev->config;
	ADC_TypeDef *adc = config->base;
#endif /* !DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_adc) */

	LOG_DBG("dma callback");

	if (channel == data->dma.channel) {
#if !DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_adc)
		if (LL_ADC_IsActiveFlag_OVR(adc)) {
			LL_ADC_ClearFlag_OVR(adc);
			LOG_ERR("ADC overrun error occurred. Reduce clock source frequency, "
				"increase prescaler value or increase sampling times.");
		}
#endif /* !DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_adc) */
		if (status >= 0) {
			data->samples_count = data->channel_count;
			data->buffer += data->channel_count;
			/* Stop the DMA engine, only to start it again when the callback returns
			 * ADC_ACTION_REPEAT or ADC_ACTION_CONTINUE, or the number of samples
			 * haven't been reached Starting the DMA engine is done
			 * within adc_context_start_sampling
			 */
			dma_stop(data->dma.dma_dev, data->dma.channel);
			/* No need to invalidate the cache because it's assumed that
			 * the address is in a non-cacheable SRAM region.
			 */
			adc_context_on_sampling_done(&data->ctx, dev);
			pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE,
						 PM_ALL_SUBSTATES);
			if (IS_ENABLED(CONFIG_PM_S2RAM)) {
				pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_RAM,
							 PM_ALL_SUBSTATES);
			}
		} else if (status < 0) {
			LOG_ERR("DMA sampling complete, but DMA reported error %d", status);
			data->dma_error = status;
#if !DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_adc) && !DT_HAS_COMPAT_STATUS_OKAY(st_stm32f4_adc)
			LL_ADC_REG_StopConversion(adc);
#endif
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
	ADC_TypeDef *adc = config->base;

	uintptr_t addr = (uintptr_t)adc + reg;

	return ((*(volatile uint32_t *)addr >> shift) & mask);
}

static void set_reg_value(const struct device *dev, uint32_t reg,
			  uint32_t shift, uint32_t mask, uint32_t value)
{
	const struct adc_stm32_cfg *config = dev->config;
	ADC_TypeDef *adc = config->base;

	uintptr_t addr = (uintptr_t)adc + reg;

	MODIFY_REG(*(volatile uint32_t *)addr, (mask << shift), (value << shift));
}

static int set_resolution(const struct device *dev,
			  const struct adc_sequence *sequence)
{
	const struct adc_stm32_cfg *config = dev->config;
	ADC_TypeDef *adc = config->base;
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

static int set_sequencer(const struct device *dev)
{
	const struct adc_stm32_cfg *config = dev->config;
	struct adc_stm32_data *data = dev->data;
	ADC_TypeDef *adc = config->base;

	uint8_t channel_id;
	uint8_t channel_index = 0;
	uint32_t channels_mask = 0;

	/* Iterate over selected channels in bitmask keeping track of:
	 * - channel_index: ranging from 0 -> ( data->channel_count - 1 )
	 * - channel_id: ordinal position of channel in data->channels bitmask
	 */
	for (uint32_t channels = data->channels; channels;
		      channels &= ~BIT(channel_id), channel_index++) {
		channel_id = find_lsb_set(channels) - 1;

		uint32_t channel = __LL_ADC_DECIMAL_NB_TO_CHANNEL(channel_id);

		channels_mask |= channel;

#if ANY_ADC_SEQUENCER_TYPE_IS(FULLY_CONFIGURABLE)
		if (config->sequencer_type == FULLY_CONFIGURABLE) {
#if defined(CONFIG_SOC_SERIES_STM32H7X) || \
	defined(CONFIG_SOC_SERIES_STM32N6X) || \
	defined(CONFIG_SOC_SERIES_STM32U5X)
			/*
			 * Each channel in the sequence must be previously enabled in PCSEL.
			 * This register controls the analog switch integrated in the IO level.
			 */
			LL_ADC_SetChannelPreselection(adc, channel);
#endif /* CONFIG_SOC_SERIES_STM32H7X || CONFIG_SOC_SERIES_STM32U5X */
			LL_ADC_REG_SetSequencerRanks(adc, table_rank[channel_index], channel);
			LL_ADC_REG_SetSequencerLength(adc, table_seq_len[channel_index]);
		}
#endif /* ANY_ADC_SEQUENCER_TYPE_IS(FULLY_CONFIGURABLE) */
	}

#if ANY_ADC_SEQUENCER_TYPE_IS(NOT_FULLY_CONFIGURABLE)
	if (config->sequencer_type == NOT_FULLY_CONFIGURABLE) {
		LL_ADC_REG_SetSequencerChannels(adc, channels_mask);

#if !defined(CONFIG_SOC_SERIES_STM32F0X) && \
	!defined(CONFIG_SOC_SERIES_STM32L0X) && \
	!defined(CONFIG_SOC_SERIES_STM32U5X) && \
	!defined(CONFIG_SOC_SERIES_STM32WBAX)
		/*
		 * After modifying sequencer it is mandatory to wait for the
		 * assertion of CCRDY flag
		 */
		while (LL_ADC_IsActiveFlag_CCRDY(adc) == 0) {
		}
		LL_ADC_ClearFlag_CCRDY(adc);
#endif /* !CONFIG_SOC_SERIES_STM32F0X && !L0X && !U5X && !WBAX */
	}
#endif /* ANY_ADC_SEQUENCER_TYPE_IS(NOT_FULLY_CONFIGURABLE) */

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_adc) || \
	DT_HAS_COMPAT_STATUS_OKAY(st_stm32f4_adc)
	LL_ADC_SetSequencersScanMode(adc, LL_ADC_SEQ_SCAN_ENABLE);
#endif /* st_stm32f1_adc || st_stm32f4_adc */

	return 0;
}

static int start_read(const struct device *dev,
		      const struct adc_sequence *sequence)
{
	const struct adc_stm32_cfg *config = dev->config;
	struct adc_stm32_data *data = dev->data;
	ADC_TypeDef *adc = config->base;
	int err;

	data->buffer = sequence->buffer;
	data->channels = sequence->channels;
	data->channel_count = POPCOUNT(data->channels);
	data->samples_count = 0;

	if (data->channel_count == 0) {
		LOG_ERR("No channels selected");
		return -EINVAL;
	}

#if ANY_ADC_SEQUENCER_TYPE_IS(FULLY_CONFIGURABLE)
	if (data->channel_count > ARRAY_SIZE(table_seq_len)) {
		LOG_ERR("Too many channels for sequencer. Max: %d", ARRAY_SIZE(table_seq_len));
		return -EINVAL;
	}
#endif /* ANY_ADC_SEQUENCER_TYPE_IS(FULLY_CONFIGURABLE) */

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_adc) && !defined(CONFIG_ADC_STM32_DMA)
	/* Multiple samplings is only supported with DMA for F1 */
	if (data->channel_count > 1) {
		LOG_ERR("Without DMA, this device only supports single channel sampling");
		return -EINVAL;
	}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_adc) && !CONFIG_ADC_STM32_DMA */

	/* Check and set the resolution */
	err = set_resolution(dev, sequence);
	if (err < 0) {
		return err;
	}

	/* Configure the sequencer */
	err = set_sequencer(dev);
	if (err < 0) {
		return err;
	}

	err = check_buffer(sequence, data->channel_count);
	if (err) {
		return err;
	}

#ifdef HAS_OVERSAMPLING
	err = adc_stm32_oversampling(dev, sequence->oversampling);
	if (err) {
		return err;
	}
#else
	if (sequence->oversampling) {
		LOG_ERR("Oversampling not supported");
		return -ENOTSUP;
	}
#endif /* HAS_OVERSAMPLING */

	if (sequence->calibrate) {
#if defined(HAS_CALIBRATION)
		adc_stm32_calibrate(dev);
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

#if !DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_adc)
	LL_ADC_ClearFlag_OVR(adc);
#endif /* !DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_adc) */

#if !defined(CONFIG_ADC_STM32_DMA)
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32f4_adc)
	/* Trigger an ISR after each sampling (not just end of sequence) */
	LL_ADC_REG_SetFlagEndOfConversion(adc, LL_ADC_REG_FLAG_EOC_UNITARY_CONV);
	LL_ADC_EnableIT_EOCS(adc);
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_adc)
	LL_ADC_EnableIT_EOS(adc);
#else
	LL_ADC_EnableIT_EOC(adc);
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
	const struct device *dev = data->dev;
	const struct adc_stm32_cfg *config = dev->config;
	__maybe_unused ADC_TypeDef *adc = config->base;

	data->repeat_buffer = data->buffer;

#ifdef CONFIG_ADC_STM32_DMA
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32f4_adc)
	/* Make sure DMA bit of ADC register CR2 is set to 0 before starting a DMA transfer */
	LL_ADC_REG_SetDMATransfer(adc, LL_ADC_REG_DMA_TRANSFER_NONE);
#endif
	adc_stm32_dma_start(dev, data->buffer, data->channel_count);
#endif
	adc_stm32_start_conversion(dev);
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

#if !DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_adc)
	if (LL_ADC_IsActiveFlag_OVR(adc)) {
		LL_ADC_ClearFlag_OVR(adc);
		LOG_ERR("ADC overrun error occurred. Use DMA, reduce clock source frequency, "
			"increase prescaler value or increase sampling times.");
	}
#endif /* !DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_adc) */

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_adc)
	if (LL_ADC_IsActiveFlag_EOS(adc) == 1) {
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32f4_adc)
	if (LL_ADC_IsActiveFlag_EOCS(adc) == 1) {
#else
	if (LL_ADC_IsActiveFlag_EOC(adc) == 1) {
#endif
		*data->buffer++ = LL_ADC_REG_ReadConversionData32(adc);
		/* ISR is triggered after each conversion, and at the end-of-sequence. */
		if (++data->samples_count == data->channel_count) {
			data->samples_count = 0;
			adc_context_on_sampling_done(&data->ctx, dev);
			pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE,
						 PM_ALL_SUBSTATES);
			if (IS_ENABLED(CONFIG_PM_S2RAM)) {
				pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_RAM,
							 PM_ALL_SUBSTATES);
			}
		}
	}

	LOG_DBG("%s ISR triggered.", dev->name);
}
#endif /* !CONFIG_ADC_STM32_DMA */

static void adc_context_on_complete(struct adc_context *ctx, int status)
{
	struct adc_stm32_data *data =
		CONTAINER_OF(ctx, struct adc_stm32_data, ctx);
	const struct adc_stm32_cfg *config = data->dev->config;
	__maybe_unused ADC_TypeDef *adc = config->base;

	ARG_UNUSED(status);

	/* Reset acquisition time used for the sequence */
	data->acq_time_index[0] = -1;
	data->acq_time_index[1] = -1;

#if defined(CONFIG_SOC_SERIES_STM32H7X) || defined(CONFIG_SOC_SERIES_STM32U5X)
	/* Reset channel preselection register */
	LL_ADC_SetChannelPreselection(adc, 0);
#endif /* CONFIG_SOC_SERIES_STM32H7X || CONFIG_SOC_SERIES_STM32U5X */
}

static int adc_stm32_read(const struct device *dev,
			  const struct adc_sequence *sequence)
{
	struct adc_stm32_data *data = dev->data;
	int error;

	adc_context_lock(&data->ctx, false, NULL);
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	if (IS_ENABLED(CONFIG_PM_S2RAM)) {
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
	}
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
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	if (IS_ENABLED(CONFIG_PM_S2RAM)) {
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
	}
	error = start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}
#endif

static int adc_stm32_sampling_time_check(const struct device *dev, uint16_t acq_time)
{
	const struct adc_stm32_cfg *config =
		(const struct adc_stm32_cfg *)dev->config;

	if (acq_time == ADC_ACQ_TIME_DEFAULT) {
		return 0;
	}

	if (acq_time == ADC_ACQ_TIME_MAX) {
		return STM32_NB_SAMPLING_TIME - 1;
	}

	for (int i = 0; i < STM32_NB_SAMPLING_TIME; i++) {
		if (acq_time == ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS,
					     config->sampling_time_table[i])) {
			return i;
		}
	}

	LOG_ERR("Sampling time value not supported.");
	return -EINVAL;
}

static int adc_stm32_sampling_time_setup(const struct device *dev, uint8_t id,
					 uint16_t acq_time)
{
	const struct adc_stm32_cfg *config =
		(const struct adc_stm32_cfg *)dev->config;
	ADC_TypeDef *adc = config->base;
	__maybe_unused struct adc_stm32_data *data = dev->data;

	int acq_time_index;

	acq_time_index = adc_stm32_sampling_time_check(dev, acq_time);
	if (acq_time_index < 0) {
		return acq_time_index;
	}

	/*
	 * For all series we use the fact that the macros LL_ADC_SAMPLINGTIME_*
	 * that should be passed to the set functions are all coded on 3 bits
	 * with 0 shift (ie 0 to 7). So acq_time_index is equivalent to the
	 * macro we would use for the desired sampling time.
	 */
	switch (config->num_sampling_time_common_channels) {
	case 0:
#if ANY_NUM_COMMON_SAMPLING_TIME_CHANNELS_IS(0)
		LL_ADC_SetChannelSamplingTime(adc,
					      __LL_ADC_DECIMAL_NB_TO_CHANNEL(id),
					      (uint32_t)acq_time_index);
#endif
		break;
	case 1:
#if ANY_NUM_COMMON_SAMPLING_TIME_CHANNELS_IS(1)
		/* Only one sampling time can be selected for all channels.
		 * The first one we find is used, all others must match.
		 */
		if ((data->acq_time_index[0] == -1) ||
			(acq_time_index == data->acq_time_index[0])) {
			/* Reg is empty or value matches */
			data->acq_time_index[0] = acq_time_index;
			LL_ADC_SetSamplingTimeCommonChannels(adc,
							     (uint32_t)acq_time_index);
		} else {
			/* Reg is used and value does not match */
			LOG_ERR("Multiple sampling times not supported");
			return -EINVAL;
		}
#endif
		break;
	case 2:
#if ANY_NUM_COMMON_SAMPLING_TIME_CHANNELS_IS(2)
		/* Two different sampling times can be selected for all channels.
		 * The first two we find are used, all others must match either one.
		 */
		if ((data->acq_time_index[0] == -1) ||
			(acq_time_index == data->acq_time_index[0])) {
			/* 1st reg is empty or value matches 1st reg */
			data->acq_time_index[0] = acq_time_index;
			LL_ADC_SetChannelSamplingTime(adc,
						      __LL_ADC_DECIMAL_NB_TO_CHANNEL(id),
						      LL_ADC_SAMPLINGTIME_COMMON_1);
			LL_ADC_SetSamplingTimeCommonChannels(adc,
							     LL_ADC_SAMPLINGTIME_COMMON_1,
							     (uint32_t)acq_time_index);
		} else if ((data->acq_time_index[1] == -1) ||
			(acq_time_index == data->acq_time_index[1])) {
			/* 2nd reg is empty or value matches 2nd reg */
			data->acq_time_index[1] = acq_time_index;
			LL_ADC_SetChannelSamplingTime(adc,
						      __LL_ADC_DECIMAL_NB_TO_CHANNEL(id),
						      LL_ADC_SAMPLINGTIME_COMMON_2);
			LL_ADC_SetSamplingTimeCommonChannels(adc,
							     LL_ADC_SAMPLINGTIME_COMMON_2,
							     (uint32_t)acq_time_index);
		} else {
			/* Both regs are used, value does not match any of them */
			LOG_ERR("Only two different sampling times supported");
			return -EINVAL;
		}
#endif
		break;
	default:
		LOG_ERR("Number of common sampling time channels not supported");
		return -EINVAL;
	}
	return 0;
}

static int adc_stm32_channel_setup(const struct device *dev,
				   const struct adc_channel_cfg *channel_cfg)
{
#ifdef CONFIG_SOC_SERIES_STM32H5X
	const struct adc_stm32_cfg *config = (const struct adc_stm32_cfg *)dev->config;
	ADC_TypeDef *adc = config->base;
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

	if (adc_stm32_sampling_time_setup(dev, channel_cfg->channel_id,
					  channel_cfg->acquisition_time) != 0) {
		LOG_ERR("Invalid sampling time");
		return -EINVAL;
	}

#ifdef CONFIG_SOC_SERIES_STM32H5X
	if (adc == ADC1) {
		if (channel_cfg->channel_id == 0) {
			LL_ADC_EnableChannel0_GPIO(adc);
		}
	}
#endif

	LOG_DBG("Channel setup succeeded!");

	return 0;
}

#if defined(CONFIG_SOC_SERIES_STM32C0X) ||                                                     \
	defined(CONFIG_SOC_SERIES_STM32G0X) ||                                                     \
	defined(CONFIG_SOC_SERIES_STM32L0X) ||                                                     \
	defined(CONFIG_SOC_SERIES_STM32U0X) ||                                                     \
	(defined(CONFIG_SOC_SERIES_STM32WBX) && defined(ADC_SUPPORT_2_5_MSPS)) ||                  \
	defined(CONFIG_SOC_SERIES_STM32WLX)
#define ADC_STM32_HAS_INDIVIDUAL_CLOCKS
#endif

#if defined(CONFIG_SOC_SERIES_STM32H7X) || defined(ADC_STM32_HAS_INDIVIDUAL_CLOCKS)
static bool adc_stm32_is_clk_sync(const struct adc_stm32_cfg *config)
{
	if (config->clk_prescaler == LL_ADC_CLOCK_SYNC_PCLK_DIV1 ||
	    config->clk_prescaler == LL_ADC_CLOCK_SYNC_PCLK_DIV2 ||
	    config->clk_prescaler == LL_ADC_CLOCK_SYNC_PCLK_DIV4) {
		return true;
	}

	return false;
}
#endif

#if defined(CONFIG_SOC_SERIES_STM32H7X)
static inline int adc_stm32_get_input_freq_prescaler(void)
{
	int presc = 2;

#ifdef STM32H74X_ADC
	/* For revision Y we have no prescaler of 2 */
	if (LL_DBGMCU_GetRevisionID() <= 0x1003) {
		presc = 1;
	}
#endif

	return presc;
}

static int adc_stm32_get_clock_prescaler(const struct adc_stm32_cfg *config)
{
	switch (config->clk_prescaler) {
	case LL_ADC_CLOCK_SYNC_PCLK_DIV1:
	case LL_ADC_CLOCK_ASYNC_DIV1:
		return 1;
	case LL_ADC_CLOCK_SYNC_PCLK_DIV2:
	case LL_ADC_CLOCK_ASYNC_DIV2:
		return 2;
	case LL_ADC_CLOCK_SYNC_PCLK_DIV4:
	case LL_ADC_CLOCK_ASYNC_DIV4:
		return 4;
	case LL_ADC_CLOCK_ASYNC_DIV6:
		return 6;
	case LL_ADC_CLOCK_ASYNC_DIV8:
		return 8;
	case LL_ADC_CLOCK_ASYNC_DIV10:
		return 10;
	case LL_ADC_CLOCK_ASYNC_DIV12:
		return 12;
	case LL_ADC_CLOCK_ASYNC_DIV16:
		return 16;
	case LL_ADC_CLOCK_ASYNC_DIV32:
		return 32;
	case LL_ADC_CLOCK_ASYNC_DIV64:
		return 64;
	case LL_ADC_CLOCK_ASYNC_DIV128:
		return 128;
	case LL_ADC_CLOCK_ASYNC_DIV256:
		return 256;
	default:
		return -EINVAL;
	}
}

static int adc_stm32h7_setup_boost(const struct adc_stm32_cfg *config, ADC_TypeDef *adc,
				   const struct device *clk)
{
	clock_control_subsys_t clk_src;
	uint32_t input_freq;
	uint32_t boost;
	int presc;

	/* Get the input frequency */
	clk_src = (clock_control_subsys_t)(adc_stm32_is_clk_sync(config) ? &config->pclken[0]
									 : &config->pclken[1]);

	if (clock_control_get_rate(clk, clk_src, &input_freq) != 0) {
		LOG_ERR("Failed to get ADC clock frequency");
		return -EIO;
	}

	/* Adjust the pre-scaler value so that we can divide down the clock */
	presc = adc_stm32_get_clock_prescaler(config);
	if (presc < 0) {
		LOG_ERR("Invalid clock prescaler value");
		return presc;
	}

	input_freq /= presc * adc_stm32_get_input_freq_prescaler();

	if (input_freq <= KHZ(6250)) {
		boost = LL_ADC_BOOST_MODE_6MHZ25;
	} else if (input_freq <= KHZ(12500)) {
		boost = LL_ADC_BOOST_MODE_12MHZ5;
	} else if (input_freq <= MHZ(20)) {
		boost = LL_ADC_BOOST_MODE_20MHZ;
	} else if (input_freq <= MHZ(25)) {
		boost = LL_ADC_BOOST_MODE_25MHZ;
	} else if (input_freq <= MHZ(50)) {
		boost = LL_ADC_BOOST_MODE_50MHZ;
	} else {
		LOG_WRN("ADC clock frequency too high %u", input_freq);
		return -ERANGE;
	}

	LL_ADC_SetBoostMode(adc, boost);

	return 0;
}
#endif

/* This symbol takes the value 1 if one of the device instances */
/* is configured in dts with a domain clock */
#if STM32_DT_INST_DEV_DOMAIN_CLOCK_SUPPORT
#define STM32_ADC_DOMAIN_CLOCK_SUPPORT 1
#else
#define STM32_ADC_DOMAIN_CLOCK_SUPPORT 0
#endif

static int adc_stm32_set_clock(const struct device *dev)
{
	const struct adc_stm32_cfg *config = dev->config;
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	__maybe_unused ADC_TypeDef *adc = config->base;
	int ret = 0;

	if (clock_control_on(clk,
		(clock_control_subsys_t) &config->pclken[0]) != 0) {
		return -EIO;
	}

	if (IS_ENABLED(STM32_ADC_DOMAIN_CLOCK_SUPPORT) && (config->pclk_len > 1)) {
		/* Enable ADC clock source */
		if (clock_control_configure(clk,
					    (clock_control_subsys_t) &config->pclken[1],
					    NULL) != 0) {
			return -EIO;
		}
	}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(st_adc_clock_source)
#if defined(CONFIG_SOC_SERIES_STM32F0X)
	LL_ADC_SetClock(adc, config->clk_prescaler);
#elif defined(ADC_STM32_HAS_INDIVIDUAL_CLOCKS)
	if (adc_stm32_is_clk_sync(config)) {
		LL_ADC_SetClock(adc, config->clk_prescaler);
	} else {
		LL_ADC_SetCommonClock(__LL_ADC_COMMON_INSTANCE(adc),
				      config->clk_prescaler);
		LL_ADC_SetClock(adc, LL_ADC_CLOCK_ASYNC);
	}
#else
	LL_ADC_SetCommonClock(__LL_ADC_COMMON_INSTANCE(adc),
			      config->clk_prescaler);

#ifdef CONFIG_SOC_SERIES_STM32H7X
	/* Set boost according to input frequency */
	ret = adc_stm32h7_setup_boost(config, adc, clk);
#endif
#endif
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(st_adc_clock_source) */

	return ret;
}

static void adc_stm32_enable_analog_supply(void)
{
#if defined(CONFIG_SOC_SERIES_STM32N6X)
	LL_PWR_EnableVddADC();
#elif defined(CONFIG_SOC_SERIES_STM32U5X)
	LL_PWR_EnableVDDA();
#endif /* CONFIG_SOC_SERIES_STM32U5X */
}

#ifdef CONFIG_PM_DEVICE
static void adc_stm32_disable_analog_supply(void)
{
#if defined(CONFIG_SOC_SERIES_STM32N6X)
	LL_PWR_DisableVddADC();
#elif defined(CONFIG_SOC_SERIES_STM32U5X)
	LL_PWR_DisableVDDA();
#endif /* CONFIG_SOC_SERIES_STM32U5X */
}
#endif

static int adc_stm32_init(const struct device *dev)
{
	struct adc_stm32_data *data = dev->data;
	const struct adc_stm32_cfg *config = dev->config;
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	__maybe_unused ADC_TypeDef *adc = config->base;
	int err;

	LOG_DBG("Initializing %s", dev->name);

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	data->dev = dev;

	/*
	 * For series that use common channels for sampling time, all
	 * conversion time for all channels on one ADC instance has to
	 * be the same.
	 * For series that use two common channels, there can be up to two
	 * conversion times selected for all channels in a sequence.
	 * This additional table is for checking that the conversion time
	 * selection of all channels respects these requirements.
	 */
	data->acq_time_index[0] = -1;
	data->acq_time_index[1] = -1;

	adc_stm32_set_clock(dev);

	/* Configure ADC inputs as specified in Device Tree, if any */
	err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if ((err < 0) && (err != -ENOENT)) {
		/*
		 * If the ADC is used only with internal channels, then no pinctrl is
		 * provided in Device Tree, and pinctrl_apply_state returns -ENOENT,
		 * but this should not be treated as an error.
		 */
		LOG_ERR("ADC pinctrl setup failed (%d)", err);
		return err;
	}

	adc_stm32_enable_analog_supply();

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
	defined(CONFIG_SOC_SERIES_STM32H7RSX) || \
	defined(CONFIG_SOC_SERIES_STM32N6X) || \
	defined(CONFIG_SOC_SERIES_STM32U5X)
	/*
	 * L4, WB, G4, H5, H7 and U5 series STM32 needs to be awaken from deep sleep
	 * mode, and restore its calibration parameters if there are some
	 * previously stored calibration parameters.
	 */
	LL_ADC_DisableDeepPowerDown(adc);
#endif

	/*
	 * Many ADC modules need some time to be stabilized before performing
	 * any enable or calibration actions.
	 */
#if !defined(CONFIG_SOC_SERIES_STM32F0X) && \
	!DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_adc) && \
	!DT_HAS_COMPAT_STATUS_OKAY(st_stm32f4_adc) && \
	!defined(CONFIG_SOC_SERIES_STM32N6X)
	LL_ADC_EnableInternalRegulator(adc);
	/* Wait for Internal regulator stabilisation
	 * Some series have a dedicated status bit, others relie on a delay
	 */
#if defined(CONFIG_SOC_SERIES_STM32H7X) && defined(STM32H72X_ADC)
	/* ADC3 on H72x/H73x doesn't have the LDORDY status bit */
	if (adc == ADC3) {
		k_busy_wait(LL_ADC_DELAY_INTERNAL_REGUL_STAB_US);
	} else {
		while (LL_ADC_IsActiveFlag_LDORDY(adc) == 0) {
		}
	}
#elif defined(CONFIG_SOC_SERIES_STM32H7X) || \
	defined(CONFIG_SOC_SERIES_STM32U5X) || \
	defined(CONFIG_SOC_SERIES_STM32WBAX)
	while (LL_ADC_IsActiveFlag_LDORDY(adc) == 0) {
	}
#else
	k_busy_wait(LL_ADC_DELAY_INTERNAL_REGUL_STAB_US);
#endif
#endif

	if (config->irq_cfg_func) {
		config->irq_cfg_func();
	}

#if defined(HAS_CALIBRATION)
	adc_stm32_calibrate(dev);
	LL_ADC_REG_SetTriggerSource(adc, LL_ADC_REG_TRIG_SOFTWARE);
#endif /* HAS_CALIBRATION */

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int adc_stm32_suspend_setup(const struct device *dev)
{
	const struct adc_stm32_cfg *config = dev->config;
	ADC_TypeDef *adc = config->base;
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	int err;

	/* Disable ADC */
	adc_stm32_disable(adc);

#if !defined(CONFIG_SOC_SERIES_STM32F0X) && \
	!DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_adc) && \
	!DT_HAS_COMPAT_STATUS_OKAY(st_stm32f4_adc) && \
	!defined(CONFIG_SOC_SERIES_STM32N6X)
	/* Disable ADC internal voltage regulator */
	LL_ADC_DisableInternalRegulator(adc);
	while (LL_ADC_IsInternalRegulatorEnabled(adc) == 1U) {
	}
#endif

#if defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32G4X) || \
	defined(CONFIG_SOC_SERIES_STM32H5X) || \
	defined(CONFIG_SOC_SERIES_STM32H7X) || \
	defined(CONFIG_SOC_SERIES_STM32H7RSX) || \
	defined(CONFIG_SOC_SERIES_STM32N6X) || \
	defined(CONFIG_SOC_SERIES_STM32U5X)
	/*
	 * L4, WB, G4, H5, H7 and U5 series STM32 needs to be put into
	 * deep sleep mode.
	 */

	LL_ADC_EnableDeepPowerDown(adc);
#endif

	adc_stm32_disable_analog_supply();

	/* Stop device clock. Note: fixed clocks are not handled yet. */
	err = clock_control_off(clk, (clock_control_subsys_t)&config->pclken[0]);
	if (err != 0) {
		LOG_ERR("Could not disable ADC clock");
		return err;
	}

	/* Move pins to sleep state */
	err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_SLEEP);
	if ((err < 0) && (err != -ENOENT)) {
		/*
		 * If returning -ENOENT, no pins where defined for sleep mode :
		 * Do not output on console (might sleep already) when going to sleep,
		 * "ADC pinctrl sleep state not available"
		 * and don't block PM suspend.
		 * Else return the error.
		 */
		return err;
	}

	return 0;
}

static int adc_stm32_pm_action(const struct device *dev,
			       enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return adc_stm32_init(dev);
	case PM_DEVICE_ACTION_SUSPEND:
		return adc_stm32_suspend_setup(dev);
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static DEVICE_API(adc, api_stm32_driver_api) = {
	.channel_setup = adc_stm32_channel_setup,
	.read = adc_stm32_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_stm32_read_async,
#endif
	.ref_internal = STM32_ADC_VREF_MV, /* VREF is usually connected to VDD */
};

/* Macros for ADC clock source and prescaler */
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(st_adc_clock_source)

#if defined(CONFIG_SOC_SERIES_STM32F0X)
/* LL_ADC_CLOCK_ASYNC_DIV1 doesn't exist in F0 LL. Define it here. */
#define LL_ADC_CLOCK_ASYNC_DIV1 LL_ADC_CLOCK_ASYNC
#endif

/* st_prescaler property requires 2 elements : clock ASYNC/SYNC and DIV */
#define ADC_STM32_CLOCK(x)	DT_INST_STRING_UPPER_TOKEN(x, st_adc_clock_source)
#define ADC_STM32_DIV(x)	DT_INST_PROP(x, st_adc_prescaler)

/* Macro to set the prefix depending on the 1st element: check if it is SYNC or ASYNC */
#define ADC_STM32_CLOCK_PREFIX(x)			\
	COND_CODE_1(IS_EQ(ADC_STM32_CLOCK(x), SYNC),	\
		(LL_ADC_CLOCK_SYNC_PCLK_DIV),		\
		(LL_ADC_CLOCK_ASYNC_DIV))

/* Concat prefix (1st element) and DIV value (2nd element) of st,adc-prescaler */
#define ADC_STM32_DT_PRESC(x)	\
	_CONCAT(ADC_STM32_CLOCK_PREFIX(x), ADC_STM32_DIV(x))

/* Macro to check if the ADC instance clock setup is correct */
#define ADC_STM32_CHECK_DT_CLOCK(x)								\
	BUILD_ASSERT(IS_EQ(ADC_STM32_CLOCK(x), SYNC) || (DT_INST_NUM_CLOCKS(x) > 1),		\
		     "ASYNC clock mode defined without ASYNC clock defined in device tree")

#else /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(st_adc_clock_source) */

#define ADC_STM32_DT_PRESC(x)	0
#define ADC_STM32_CHECK_DT_CLOCK(x)

#endif /* !DT_ANY_INST_HAS_PROP_STATUS_OKAY(st_adc_clock_source) */


#if defined(CONFIG_ADC_STM32_DMA)

#define ADC_DMA_CHANNEL_INIT(index, src_dev, dest_dev)					\
	.dma = {									\
		.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_IDX(index, 0)),		\
		.channel = DT_INST_DMAS_CELL_BY_IDX(index, 0, channel),			\
		.dma_cfg = {								\
			.dma_slot = STM32_DMA_SLOT_BY_IDX(index, 0, slot),		\
			.channel_direction = STM32_DMA_CONFIG_DIRECTION(		\
				STM32_DMA_CHANNEL_CONFIG_BY_IDX(index, 0)),		\
			.source_data_size = STM32_DMA_CONFIG_##src_dev##_DATA_SIZE(	\
				STM32_DMA_CHANNEL_CONFIG_BY_IDX(index, 0)),		\
			.dest_data_size = STM32_DMA_CONFIG_##dest_dev##_DATA_SIZE(	\
				STM32_DMA_CHANNEL_CONFIG_BY_IDX(index, 0)),		\
			.source_burst_length = 1,       /* SINGLE transfer */		\
			.dest_burst_length = 1,         /* SINGLE transfer */		\
			.channel_priority = STM32_DMA_CONFIG_PRIORITY(			\
				STM32_DMA_CHANNEL_CONFIG_BY_IDX(index, 0)),		\
			.dma_callback = dma_callback,					\
			.block_count = 2,						\
		},									\
		.src_addr_increment = STM32_DMA_CONFIG_##src_dev##_ADDR_INC(		\
			STM32_DMA_CHANNEL_CONFIG_BY_IDX(index, 0)),			\
		.dst_addr_increment = STM32_DMA_CONFIG_##dest_dev##_ADDR_INC(		\
			STM32_DMA_CHANNEL_CONFIG_BY_IDX(index, 0)),			\
	}

#define ADC_STM32_IRQ_FUNC(index)					\
	.irq_cfg_func = NULL,

#else /* CONFIG_ADC_STM32_DMA */

/*
 * For series that share interrupt lines for multiple ADC instances
 * and have separate interrupt lines for other ADCs (example,
 * STM32G473 has 5 ADC instances, ADC1 and ADC2 share IRQn 18 while
 * ADC3, ADC4 and ADC5 use IRQns 47, 61 and 62 respectively), generate
 * a single common ISR function for each IRQn and call adc_stm32_isr
 * for each device using that interrupt line for all enabled ADCs.
 *
 * To achieve the above, a "first" ADC instance must be chosen for all
 * ADC instances sharing the same IRQn. This "first" ADC instance
 * generates the code for the common ISR and for installing and
 * enabling it while any other ADC sharing the same IRQn skips this
 * code generation and does nothing. The common ISR code is generated
 * to include calls to adc_stm32_isr for all instances using that same
 * IRQn. From the example above, four ISR functions would be generated
 * for IRQn 18, 47, 61 and 62, with possible "first" ADC instances
 * being ADC1, ADC3, ADC4 and ADC5 if all ADCs were enabled, with the
 * ISR function 18 calling adc_stm32_isr for both ADC1 and ADC2.
 *
 * For some of the macros below, pseudo-code is provided to describe
 * its function.
 */

/*
 * return (irqn == device_irqn(index)) ? index : NULL
 */
#define FIRST_WITH_IRQN_INTERNAL(index, irqn)                                                      \
	COND_CODE_1(IS_EQ(irqn, DT_INST_IRQN(index)), (index,), (EMPTY,))

/*
 * Returns the "first" instance's index:
 *
 * instances = []
 * for instance in all_active_adcs:
 *     instances.append(first_with_irqn_internal(device_irqn(index)))
 * for instance in instances:
 *     if instance == NULL:
 *         instances.remove(instance)
 * return instances[0]
 */
#define FIRST_WITH_IRQN(index)                                                                     \
	GET_ARG_N(1, LIST_DROP_EMPTY(DT_INST_FOREACH_STATUS_OKAY_VARGS(FIRST_WITH_IRQN_INTERNAL,   \
								       DT_INST_IRQN(index))))

/*
 * Provides code for calling adc_stm32_isr for an instance if its IRQn
 * matches:
 *
 * if (irqn == device_irqn(index)):
 *     return "adc_stm32_isr(DEVICE_DT_INST_GET(index));"
 */
#define HANDLE_IRQS(index, irqn)                                                                   \
	COND_CODE_1(IS_EQ(irqn, DT_INST_IRQN(index)), (adc_stm32_isr(DEVICE_DT_INST_GET(index));), \
		    (EMPTY))

/*
 * Name of the common ISR for a given IRQn (taken from a device with a
 * given index). Example, for an ADC instance with IRQn 18, returns
 * "adc_stm32_isr_18".
 */
#define ISR_FUNC(index) UTIL_CAT(adc_stm32_isr_, DT_INST_IRQN(index))

/*
 * Macro for generating code for the common ISRs (by looping of all
 * ADC instances that share the same IRQn as that of the given device
 * by index) and the function for setting up the ISR.
 *
 * Here is where both "first" and non-"first" instances have code
 * generated for their interrupts via HANDLE_IRQS.
 */
#define GENERATE_ISR_CODE(index)                                                                   \
	static void ISR_FUNC(index)(void)                                                          \
	{                                                                                          \
		DT_INST_FOREACH_STATUS_OKAY_VARGS(HANDLE_IRQS, DT_INST_IRQN(index))                \
	}                                                                                          \
                                                                                                   \
	static void UTIL_CAT(ISR_FUNC(index), _init)(void)                                         \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority), ISR_FUNC(index),    \
			    NULL, 0);                                                              \
		irq_enable(DT_INST_IRQN(index));                                                   \
	}

/*
 * Limit generating code to only the "first" instance:
 *
 * if (first_with_irqn(index) == index):
 *     generate_isr_code(index)
 */
#define GENERATE_ISR(index)                                                                        \
	COND_CODE_1(IS_EQ(index, FIRST_WITH_IRQN(index)), (GENERATE_ISR_CODE(index)), (EMPTY))

DT_INST_FOREACH_STATUS_OKAY(GENERATE_ISR)

/* Only "first" instances need to call the ISR setup function */
#define ADC_STM32_IRQ_FUNC(index)                                                                  \
	.irq_cfg_func = COND_CODE_1(IS_EQ(index, FIRST_WITH_IRQN(index)),                          \
				    (UTIL_CAT(ISR_FUNC(index), _init)), (NULL)),

#define ADC_DMA_CHANNEL_INIT(index, src_dev, dest_dev)

#endif /* CONFIG_ADC_STM32_DMA */

#define ADC_DMA_CHANNEL(id, src, dest)							\
	COND_CODE_1(DT_INST_DMAS_HAS_IDX(id, 0),					\
			(ADC_DMA_CHANNEL_INIT(id, src, dest)),				\
			(/* Required for other adc instances without dma */))

#define ADC_STM32_INIT(index)						\
									\
ADC_STM32_CHECK_DT_CLOCK(index);					\
									\
PINCTRL_DT_INST_DEFINE(index);						\
									\
static const struct stm32_pclken pclken_##index[] =			\
				 STM32_DT_INST_CLOCKS(index);		\
									\
static const struct adc_stm32_cfg adc_stm32_cfg_##index = {		\
	.base = (ADC_TypeDef *)DT_INST_REG_ADDR(index),			\
	ADC_STM32_IRQ_FUNC(index)					\
	.pclken = pclken_##index,					\
	.pclk_len = DT_INST_NUM_CLOCKS(index),				\
	.clk_prescaler = ADC_STM32_DT_PRESC(index),			\
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),			\
	.sequencer_type = DT_INST_STRING_UPPER_TOKEN(index, st_adc_sequencer),	\
	.oversampler_type = DT_INST_STRING_UPPER_TOKEN(index, st_adc_oversampler),	\
	.sampling_time_table = DT_INST_PROP(index, sampling_times),	\
	.num_sampling_time_common_channels =				\
		DT_INST_PROP_OR(index, num_sampling_time_common_channels, 0),\
	.res_table_size = DT_INST_PROP_LEN(index, resolutions),		\
	.res_table = DT_INST_PROP(index, resolutions),			\
};									\
									\
static struct adc_stm32_data adc_stm32_data_##index = {			\
	ADC_CONTEXT_INIT_TIMER(adc_stm32_data_##index, ctx),		\
	ADC_CONTEXT_INIT_LOCK(adc_stm32_data_##index, ctx),		\
	ADC_CONTEXT_INIT_SYNC(adc_stm32_data_##index, ctx),		\
	ADC_DMA_CHANNEL(index, PERIPHERAL, MEMORY)			\
};									\
									\
PM_DEVICE_DT_INST_DEFINE(index, adc_stm32_pm_action);			\
									\
DEVICE_DT_INST_DEFINE(index,						\
		    &adc_stm32_init, PM_DEVICE_DT_INST_GET(index),	\
		    &adc_stm32_data_##index, &adc_stm32_cfg_##index,	\
		    POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,		\
		    &api_stm32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ADC_STM32_INIT)
