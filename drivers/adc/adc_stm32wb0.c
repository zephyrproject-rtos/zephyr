/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Terminology used in this file:
 *  - sampling: a single analog-to-digital conversion performed by the ADC
 *  - sequence: one or more sampling(s) performed one after the other by the
 *	ADC after a single programmation. This is the meaning used in the
 *	STM32WB0 ADC documentation.
 *  - round: all ADC operations needed to read all channels in the adc_sequence passed
 *	to adc_read. Zephyr calls this a "sampling", but we use the term "round" to
 *	prevent confusion with STM32 terminology. A single round may require multiple
 *	sequences to be performed by the ADC to be completed, due to hardware limitations.
 *
 *	When Zephyr's "sequence" feature is used, the same round is repeated multiple times.
 *
 * - idle mode: clock & ADC configuration that minimizes power consumption
 *	- Only the ADC digital domain clock is turned on:
 *         - ADC is powered off (CTRL.ADC_CTRL_ADC_ON_OFF = 0)
 *         - ADC analog domain clock is turned off
 *	- If applicable:
 *		- ADC LDO is disabled
 *		- ADC I/O Booster clock is turned off
 *		- ADC I/O Booster is disabled
 *		- ADC-SMPS clock synchronization is disabled
 */

#define DT_DRV_COMPAT st_stm32wb0_adc

#include <errno.h>
#include <stdbool.h>

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys/math_extras.h>

#include <soc.h>
#include <stm32_ll_adc.h>
#include <stm32_ll_utils.h>

#ifdef CONFIG_ADC_STM32_DMA
#include <zephyr/drivers/dma/dma_stm32.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/toolchain.h>
#include <stm32_ll_dma.h>
#endif

#define ADC_CONTEXT_USES_KERNEL_TIMER
#define ADC_CONTEXT_ENABLE_ON_COMPLETE
#include "adc_context.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_stm32wb0, CONFIG_ADC_LOG_LEVEL);

/**
 * Driver private definitions & assertions
 */
#define ADC_INSTANCE		0
#define ADC_NODE		DT_DRV_INST(ADC_INSTANCE)
#define ADC_USE_IO_BOOSTER	DT_PROP_OR(ADC_NODE, io_booster, 0)

#define LL_ADC_EXTERNAL_CHANNEL_NUM	12	/* must be a plain constant for LISTIFY */
#define LL_ADC_EXTERNAL_CHANNEL_MAX	(LL_ADC_CHANNEL_VINP3_VINM3 + 1U)
#define LL_ADC_CHANNEL_MAX		(LL_ADC_CHANNEL_TEMPSENSOR + 1U)
#define LL_ADC_VIN_RANGE_INVALID	((uint8_t)0xFFU)

#define NUM_CALIBRATION_POINTS		4	/* 4 calibration point registers (COMP_[0-3]) */

#if !defined(ADC_CONF_SAMPLE_RATE_MSB)
#	define NUM_ADC_SAMPLE_RATES	4	/* SAMPLE_RATE on 2 bits */
#else
#	define NUM_ADC_SAMPLE_RATES	32	/* SAMPLE_RATE on 5 bits */
#endif

/* The STM32WB0 has a 12-bit ADC, but the resolution can be
 * enhanced to 16-bit by oversampling (using the downsampler)
 */
#define ADC_MIN_RESOLUTION	12
#define ADC_MAX_RESOLUTION	16

/* ADC channel type definitions are not provided by LL as
 * it uses per-type functions instead. Bring our own.
 */
#define ADC_CHANNEL_TYPE_SINGLE_NEG	(0x00U)	/* Single-ended, positive */
#define ADC_CHANNEL_TYPE_SINGLE_POS	(0x01U)	/* Single-ended, negative */
#define	ADC_CHANNEL_TYPE_DIFF		(0x02U)	/* Differential */
#define ADC_CHANNEL_TYPE_INVALID	(0xFFU)	/* Invalid */

/** See RM0505 §6.2.1 "System clock details" */
BUILD_ASSERT(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC >= (8 * 1000 * 1000),
	"STM32WB0: system clock frequency must be at least 8MHz to use ADC");

/**
 * Driver private structures
 */
struct adc_stm32wb0_data {
	struct adc_context ctx;
	const struct device *const dev;

	/**
	 * Bitmask of all channels requested as part of this round
	 * but not sampled yet.
	 */
	uint32_t unsampled_channels;

	/**
	 * Pointer in output buffer where the first data sample of the round
	 * is stored. This is used to reload next_sample_ptr when the user
	 * callback asks to repeat a round.
	 */
	uint16_t *round_buf_pointer;

	/**
	 * Pointer in output buffer where the next data sample from ADC should
	 * be stored.
	 */
	uint16_t *next_sample_ptr;

#if defined(CONFIG_ADC_STM32_DMA)
	/** Size of the sequence currently scheduled and executing */
	size_t sequence_length;
	struct dma_config dmac_config;
	struct dma_block_config dma_block_config;
#endif

	/** Channels configuration */
	struct adc_stm32wb0_channel_config {
		/** Vinput range selection */
		uint8_t vinput_range;
	} channel_config[LL_ADC_CHANNEL_MAX];
};

struct adc_stm32wb0_config {
	ADC_TypeDef *reg;
	const struct pinctrl_dev_config *pinctrl_cfg;
	/** ADC digital domain clock */
	struct stm32_pclken dig_clk;
	/** ADC analog domain clock */
	struct stm32_pclken ana_clk;
#if defined(CONFIG_ADC_STM32_DMA)
	const struct device *dmac;
	uint32_t dma_channel;
#endif
};

/**
 * Driver private utility functions
 */

/**
 * In STM32CubeWB0 v1.0.0, the LL_GetPackageType is buggy and returns wrong values.
 * This bug is reported in the ST internal bugtracker under reference 185295.
 * For now, implement the function ourselves.
 */
static inline uint32_t ll_get_package_type(void)
{
	return sys_read32(PACKAGE_BASE);
}

static inline struct adc_stm32wb0_data *drv_data_from_adc_ctx(struct adc_context *adc_ctx)
{
	return CONTAINER_OF(adc_ctx, struct adc_stm32wb0_data, ctx);
}

static inline uint8_t vinput_range_from_adc_ref(uint32_t reference)
{
	switch (reference) {
	case ADC_REF_INTERNAL:
	case ADC_REF_VDD_1:
		return LL_ADC_VIN_RANGE_3V6;
	case ADC_REF_VDD_1_2:
		return LL_ADC_VIN_RANGE_2V4;
	case ADC_REF_VDD_1_3:
		return LL_ADC_VIN_RANGE_1V2;
	default:
		return LL_ADC_VIN_RANGE_INVALID;
	}
}

static inline uint32_t ds_width_from_adc_res(uint32_t resolution)
{
	/*
	 * 12 -> 0 (LL_ADC_DS_DATA_WIDTH_12_BIT)
	 * 13 -> 1 (LL_ADC_DS_DATA_WIDTH_13_BIT)
	 * 14 -> 2 (LL_ADC_DS_DATA_WIDTH_14_BIT)
	 * 15 -> 3 (LL_ADC_DS_DATA_WIDTH_15_BIT)
	 * 16 -> 4 (LL_ADC_DS_DATA_WIDTH_16_BIT)
	 */
	return resolution - 12;
}

static inline uint8_t get_channel_type(uint32_t channel)
{
	switch (channel) {
	case LL_ADC_CHANNEL_VINM0:
	case LL_ADC_CHANNEL_VINM1:
	case LL_ADC_CHANNEL_VINM2:
	case LL_ADC_CHANNEL_VINM3:
	case LL_ADC_CHANNEL_VBAT:
		return ADC_CHANNEL_TYPE_SINGLE_NEG;
	case LL_ADC_CHANNEL_VINP0:
	case LL_ADC_CHANNEL_VINP1:
	case LL_ADC_CHANNEL_VINP2:
	case LL_ADC_CHANNEL_VINP3:
	case LL_ADC_CHANNEL_TEMPSENSOR:
		return ADC_CHANNEL_TYPE_SINGLE_POS;
	case LL_ADC_CHANNEL_VINP0_VINM0:
	case LL_ADC_CHANNEL_VINP1_VINM1:
	case LL_ADC_CHANNEL_VINP2_VINM2:
	case LL_ADC_CHANNEL_VINP3_VINM3:
		return ADC_CHANNEL_TYPE_DIFF;
	default:
		__ASSERT_NO_MSG(0);
		return ADC_CHANNEL_TYPE_INVALID;
	}
}

/**
 * @brief Checks all fields of the adc_sequence and asserts they are
 * valid and all configuration options are supported by the driver.
 *
 * @param sequence	adc_sequence to validate
 * @return 0 if the adc_sequence is valid, negative value otherwise
 */
static int validate_adc_sequence(const struct adc_sequence *sequence)
{
	const size_t round_size = sizeof(uint16_t) * POPCOUNT(sequence->channels);
	size_t needed_buf_size;

	if (sequence->channels == 0 ||
		(sequence->channels & ~BIT_MASK(LL_ADC_CHANNEL_MAX)) != 0) {
		LOG_ERR("invalid channels selection");
		return -EINVAL;
	}

	CHECKIF(!sequence->buffer) {
		LOG_ERR("storage buffer pointer is NULL");
		return -EINVAL;
	}

	if (!IN_RANGE(sequence->resolution, ADC_MIN_RESOLUTION, ADC_MAX_RESOLUTION)) {
		LOG_ERR("invalid resolution %u (must be between %u and %u)",
			sequence->resolution, ADC_MIN_RESOLUTION, ADC_MAX_RESOLUTION);
		return -EINVAL;
	}

	/* N.B.: LL define is in the same log2(x) format as the Zephyr variable */
	if (sequence->oversampling > LL_ADC_DS_RATIO_128) {
		LOG_ERR("oversampling unsupported by hardware (max: %lu)", LL_ADC_DS_RATIO_128);
		return -ENOTSUP;
	}

	if (sequence->options) {
		const size_t samplings = (size_t)sequence->options->extra_samplings + 1;

		if (size_mul_overflow(round_size, samplings, &needed_buf_size)) {
			return -ENOMEM;
		}
	} else {
		needed_buf_size = round_size;
	}

	if (needed_buf_size > sequence->buffer_size) {
		return -ENOMEM;
	}

	return 0;
}

/**
 * @brief Set which channel is sampled during a given conversion of the sequence.
 *
 * @param ADCx		ADC registers pointer
 * @param Conversion	Target conversion index (0~15)
 * @param Channel	Channel to sample during specified conversion
 *
 * @note This function is a more convenient implementation of LL_ADC_SetSequencerRanks
 */
static inline void ll_adc_set_conversion_channel(ADC_TypeDef *ADCx,
				uint32_t Conversion, uint32_t Channel)
{
	/**
	 * There are two registers to control the sequencer:
	 *   - SEQ_1 holds channel selection for conversions 0~7
	 *   - SEQ_2 holds channel selection for conversions 8~15
	 *
	 * Notice that all conversions in SEQ_2 have 3rd bit set,
	 * whereas all conversions in SEQ_1 have 3rd bit clear.
	 *
	 * In a SEQ_x register, each channel occupies 4 bits, so the
	 * field for conversion N is at bit offset (4 * (N % 7)).
	 */
	const uint32_t reg = (Conversion & 8) ? 1 : 0;
	const uint32_t shift = 4 * (Conversion & 7);

	MODIFY_REG((&ADCx->SEQ_1)[reg], ADC_SEQ_1_SEQ0 << shift, Channel << shift);
}

/**
 * @brief Set the calibration point to use for a chosen channel type and Vinput range.
 *
 * @param ADCx		ADC registers pointer
 * @param Type		Channel type
 * @param Range		Channel Vinput range
 * @param Point		Calibration point to use
 *
 * @note This is a generic version of the LL_ADC_SetCalibPointFor* functions.
 */
static inline void ll_adc_set_calib_point_for_any(ADC_TypeDef *ADCx, uint32_t Type,
						uint32_t Range, uint32_t Point)
{
	__ASSERT(Range == LL_ADC_VIN_RANGE_1V2
		|| Range == LL_ADC_VIN_RANGE_2V4
		|| Range == LL_ADC_VIN_RANGE_3V6, "Range is not valid");

	__ASSERT(Type == ADC_CHANNEL_TYPE_SINGLE_NEG
		|| Type == ADC_CHANNEL_TYPE_SINGLE_POS
		|| Type == ADC_CHANNEL_TYPE_DIFF, "Type is not valid");

	__ASSERT(Point == LL_ADC_CALIB_POINT_1
		|| Point == LL_ADC_CALIB_POINT_2
		|| Point == LL_ADC_CALIB_POINT_3
		|| Point == LL_ADC_CALIB_POINT_4, "Point is not valid");

	/* Register organization is as follows:
	 *
	 * - Group for 1.2V Vinput range
	 * - Group for 2.4V Vinput range
	 * - Group for 3.6V Vinput range
	 *
	 * where Group is organized as:
	 * - Select for Single Negative mode
	 * - Select for Single Positive mode
	 * - Select for Differential mode
	 *
	 * Each select is 2 bits, and each group is thus 6 bits.
	 */

	uint32_t type_shift, group_shift;

	switch (Type) {
	case ADC_CHANNEL_TYPE_SINGLE_NEG:
		type_shift = 0 * 2;
		break;
	case ADC_CHANNEL_TYPE_SINGLE_POS:
		type_shift = 1 * 2;
		break;
	case ADC_CHANNEL_TYPE_DIFF:
		type_shift = 2 * 2;
		break;
	default:
		CODE_UNREACHABLE;
	}

	switch (Range) {
	case LL_ADC_VIN_RANGE_1V2:
		group_shift = 0 * 6;
		break;
	case LL_ADC_VIN_RANGE_2V4:
		group_shift = 1 * 6;
		break;
	case LL_ADC_VIN_RANGE_3V6:
		group_shift = 2 * 6;
		break;
	default:
		CODE_UNREACHABLE;
	}

	const uint32_t shift = (group_shift + type_shift);

	MODIFY_REG(ADCx->COMP_SEL, (ADC_COMP_SEL_OFFSET_GAIN0 << shift), (Point << shift));
}

static void adc_acquire_pm_locks(void)
{
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	if (IS_ENABLED(CONFIG_PM_S2RAM)) {
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
	}
}

static void adc_release_pm_locks(void)
{
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	if (IS_ENABLED(CONFIG_PM_S2RAM)) {
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
	}
}

/**
 * Driver private functions
 */

static void configure_tempsensor_calib_point(ADC_TypeDef *adc, uint32_t calib_point)
{
	uint16_t gain;
#if defined(CONFIG_SOC_STM32WB09XX) || defined(CONFIG_SOC_STM32WB05XX)
	/** RM0505/RM0529 §12.2.1 "Temperature sensor subsystem" */
	gain = 0xFFF;
#else
	/** RM0530 §12.2.2 "Temperature sensor subsystem" */
	gain = LL_ADC_GET_CALIB_GAIN_FOR_VINPX_1V2();
#endif /* CONFIG_SOC_STM32WB09XX | CONFIG_SOC_STM32WB05XX */

	LL_ADC_ConfigureCalibPoint(adc, calib_point, gain, 0x0);
}

/**
 * @brief Obtain calibration data for specified channel type and Vinput range
 * from engineering flash, and write it to specified calibration point
 *
 * @param ADCx		ADC registers pointer
 * @param Point		Calibration point to configure
 * @param Type		Target channel type
 * @param Range		Target channel Vinput range
 */
static void configure_calib_point_from_flash(ADC_TypeDef *ADCx, uint32_t Point,
						uint32_t Type, uint32_t Range)
{
	int8_t offset = 0;
	uint16_t gain = 0;

	switch (Range) {
	case LL_ADC_VIN_RANGE_1V2:
		switch (Type) {
		case ADC_CHANNEL_TYPE_SINGLE_POS:
			gain = LL_ADC_GET_CALIB_GAIN_FOR_VINPX_1V2();
			offset = LL_ADC_GET_CALIB_OFFSET_FOR_VINPX_1V2();
			break;
		case ADC_CHANNEL_TYPE_SINGLE_NEG:
			gain = LL_ADC_GET_CALIB_GAIN_FOR_VINMX_1V2();
			offset = LL_ADC_GET_CALIB_OFFSET_FOR_VINMX_1V2();
			break;
		case ADC_CHANNEL_TYPE_DIFF:
			gain = LL_ADC_GET_CALIB_GAIN_FOR_VINDIFF_1V2();
			offset = LL_ADC_GET_CALIB_OFFSET_FOR_VINDIFF_1V2();
			break;
		}
		break;
	case LL_ADC_VIN_RANGE_2V4:
		switch (Type) {
		case ADC_CHANNEL_TYPE_SINGLE_POS:
			gain = LL_ADC_GET_CALIB_GAIN_FOR_VINPX_2V4();
			offset = LL_ADC_GET_CALIB_OFFSET_FOR_VINPX_2V4();
			break;
		case ADC_CHANNEL_TYPE_SINGLE_NEG:
			gain = LL_ADC_GET_CALIB_GAIN_FOR_VINMX_2V4();
			offset = LL_ADC_GET_CALIB_OFFSET_FOR_VINMX_2V4();
			break;
		case ADC_CHANNEL_TYPE_DIFF:
			gain = LL_ADC_GET_CALIB_GAIN_FOR_VINDIFF_2V4();
			offset = LL_ADC_GET_CALIB_OFFSET_FOR_VINDIFF_2V4();
			break;
		}
		break;
	case LL_ADC_VIN_RANGE_3V6:
		switch (Type) {
		case ADC_CHANNEL_TYPE_SINGLE_POS:
			gain = LL_ADC_GET_CALIB_GAIN_FOR_VINPX_3V6();
			offset = LL_ADC_GET_CALIB_OFFSET_FOR_VINPX_3V6();
			break;
		case ADC_CHANNEL_TYPE_SINGLE_NEG:
			gain = LL_ADC_GET_CALIB_GAIN_FOR_VINMX_3V6();
			offset = LL_ADC_GET_CALIB_OFFSET_FOR_VINMX_3V6();
			break;
		case ADC_CHANNEL_TYPE_DIFF:
			gain = LL_ADC_GET_CALIB_GAIN_FOR_VINDIFF_3V6();
			offset = LL_ADC_GET_CALIB_OFFSET_FOR_VINDIFF_3V6();
			break;
		}
		break;
	}

	LL_ADC_ConfigureCalibPoint(ADCx, Point, gain, offset);
}

static void adc_enter_idle_mode(ADC_TypeDef *adc, const struct stm32_pclken *ana_clk)
{
	const struct device *clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	int err;

	/* Power down the ADC */
	LL_ADC_Disable(adc);

#if SMPS_MODE != STM32WB0_SMPS_MODE_OFF
	/* Disable SMPS synchronization */
	LL_ADC_SMPSSyncDisable(adc);
#endif /* SMPS_MODE != STM32WB0_SMPS_MODE_OFF */

#if ADC_USE_IO_BOOSTER
	/* Disable ADC I/O booster */
	LL_RCC_IOBOOST_Disable();

#	if defined(RCC_CFGR_IOBOOSTCLKEN)
	/* Disable ADC I/O Booster clock if present */
	LL_RCC_IOBOOSTCLK_Disable();
#	endif
#endif /* ADC_USE_IO_BOOSTER */

#if defined(ADC_CTRL_ADC_LDO_ENA)
	/* Disable ADC voltage regulator */
	LL_ADC_DisableInternalRegulator(adc);
#endif /* ADC_CTRL_ADC_LDO_ENA */

	/* Turn off ADC analog domain clock */
	err = clock_control_off(clk, (clock_control_subsys_t)ana_clk);
	if (err < 0) {
		LOG_WRN("failed to turn off ADC analog clock (%d)", err);
	}

	/* Release power management locks */
	adc_release_pm_locks();
}

static int adc_exit_idle_mode(ADC_TypeDef *adc, const struct stm32_pclken *ana_clk)
{
	const struct device *clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	int err;

	/* Acquire power management locks */
	adc_acquire_pm_locks();

	/* Turn on ADC analog domain clock */
	err = clock_control_on(clk,
		(clock_control_subsys_t)ana_clk);
	if (err < 0) {
		LOG_ERR("failed to turn on ADC analog clock: %d", err);
		adc_release_pm_locks();
		return err;
	}

#if defined(ADC_CTRL_ADC_LDO_ENA)
	/* RM0479 §12.6.3: bit ADC_LDO_ENA must not be set on QFN32 packages.
	 * Using an equality check with supported package types ensures that
	 * we never accidentally set the bit on an unsupported MCU.
	 */
	const uint32_t package_type = ll_get_package_type();

	if (package_type == LL_UTILS_PACKAGETYPE_QFN48
		|| package_type == LL_UTILS_PACKAGETYPE_CSP49) {
		LL_ADC_EnableInternalRegulator(adc);
	}
#endif /* ADC_CTRL_ADC_LDO_ENA */

#if ADC_USE_IO_BOOSTER
#	if defined(RCC_CFGR_IOBOOSTCLKEN)
	/* Enable ADC I/O Booster clock if needed by hardware */
	LL_RCC_IOBOOSTCLK_Enable();
#	endif

	/* Enable ADC I/O Booster */
	LL_RCC_IOBOOST_Enable();
#endif /* ADC_USE_IO_BOOSTER*/

#if SMPS_MODE != STM32WB0_SMPS_MODE_OFF
	/* RM0505 §6.2.2 "Peripherals clock details":
	 * To avoid SNR degradation of the ADC,
	 * SMPS and ADC clocks must be synchronous.
	 */
	LL_ADC_SMPSSyncEnable(adc);
#endif /* SMPS_MODE != STM32WB0_SMPS_MODE_OFF */

	/* Power up the ADC */
	LL_ADC_Enable(adc);

	return err;
}

/**
 * @brief Schedule as many samplings as possible in a sequence
 *	  then start the ADC conversion.
 */
static void schedule_and_start_adc_sequence(ADC_TypeDef *adc, struct adc_stm32wb0_data *data)
{
	uint32_t remaining_unsampled = data->unsampled_channels;
	uint32_t allocated_calib_points = 0;
	uint32_t sequence_length = 0;
	bool temp_sensor_scheduled = false;

	/**
	 * These tables are used to keep track of which calibration
	 * point registers are used for what type of acquisition, in
	 * order to share the same calibration point for different
	 * channels if they use compatible configurations.
	 *
	 * Initialize only the first table with invalid values; since
	 * both tables are updated at the same time, this is sufficient
	 * to know when to stop programming calibration points.
	 */
	uint8_t calib_pt_ch_type[NUM_CALIBRATION_POINTS] = {
		ADC_CHANNEL_TYPE_INVALID, ADC_CHANNEL_TYPE_INVALID,
		ADC_CHANNEL_TYPE_INVALID, ADC_CHANNEL_TYPE_INVALID
	};
	uint8_t calib_pt_vin_range[NUM_CALIBRATION_POINTS];

	/* Schedule as many channels as possible for sampling */
	for (uint32_t channel = 0;
		channel < LL_ADC_CHANNEL_MAX && remaining_unsampled != 0U;
		channel++) {
		const uint32_t ch_bit = BIT(channel);

		if ((remaining_unsampled & ch_bit) == 0) {
			continue;
		}

		/* Get channel information */
		const uint8_t ch_type = get_channel_type(channel);
		const uint8_t ch_vin_range = data->channel_config[channel].vinput_range;

		/* Attempt to find a compatible calibration point */
		uint32_t calib_pt = 0;

		for (; calib_pt < allocated_calib_points; calib_pt++) {
			if (calib_pt_ch_type[calib_pt] == ch_type
				&& calib_pt_vin_range[calib_pt] == ch_vin_range) {
				break;
			}
		}

		if (calib_pt == allocated_calib_points) {
			/* No compatible calibration point found.
			 * If an unallocated calibration point remains, use it.
			 * Otherwise, this channel cannot be scheduled; since we must
			 * perform samplings in order, exit the scheduling loop.
			 */
			if (allocated_calib_points < NUM_CALIBRATION_POINTS) {
				allocated_calib_points++;
			} else {
				/* Exit scheduling loop */
				break;
			}
		}

		if (channel == LL_ADC_CHANNEL_TEMPSENSOR) {
			if (calib_pt_ch_type[calib_pt] == ADC_CHANNEL_TYPE_INVALID) {
				/**
				 * Temperature sensor is a special channel: it must be sampled
				 * with special gain/offset instead of the calibration values found
				 * in engineering flash. For this reason, it must NOT be scheduled
				 * with any other 1.2V Vinput range, single-ended positive channel.
				 *
				 * If this check succeeds, then no such channel is scheduled, and we
				 * can add the temperature sensor to this sequence. We're sure there
				 * won't be any conflict because the temperature sensor is the last
				 * channel. Otherwise, a channel with 1.2V Vinput range has been
				 * scheduled and we must delay the temperature sensor measurement to
				 * another sequence.
				 */
				temp_sensor_scheduled = true;
			} else {
				/* Exit scheduling loop before scheduling temperature sensor */
				break;
			}
		}

		/* Ensure calibration point tables are updated.
		 * This is unneeded if the entry was already filled up,
		 * but cheaper than checking for duplicate work.
		 */
		calib_pt_ch_type[calib_pt] = ch_type;
		calib_pt_vin_range[calib_pt] = ch_vin_range;

		/* Remove channel from unscheduled list */
		remaining_unsampled &= ~ch_bit;

		/* Add channel to sequence */
		ll_adc_set_conversion_channel(adc, sequence_length, channel);
		sequence_length++;

		/* Select the calibration point to use for channel */
		ll_adc_set_calib_point_for_any(adc, ch_type, ch_vin_range, calib_pt);

		/* Configure the channel Vinput range selection.
		 * This must not be done for internal channels, which
		 * use a hardwired Vinput range selection instead.
		 */
		if (channel < LL_ADC_EXTERNAL_CHANNEL_MAX) {
			LL_ADC_SetChannelVoltageRange(adc, channel, ch_vin_range);
		}
#if !defined(CONFIG_ADC_STM32_DMA)
		/* If DMA is not enabled, only schedule one channel at a time.
		 * Otherwise, the ADC will overflow and everything will break.
		 */
		__ASSERT_NO_MSG(sequence_length == 1);
		break;
#endif
	}

	/* Configure all (used) calibration points */
	for (int i = 0; i < NUM_CALIBRATION_POINTS; i++) {
		uint8_t type = calib_pt_ch_type[i];
		uint8_t range = calib_pt_vin_range[i];

		if (type == ADC_CHANNEL_TYPE_INVALID) {
			break;
		} else if ((type == ADC_CHANNEL_TYPE_SINGLE_POS)
				&& (range == LL_ADC_VIN_RANGE_1V2)
				&& temp_sensor_scheduled) {
			/* Configure special calibration point for temperature sensor */
			configure_tempsensor_calib_point(adc, i);
		} else {
			configure_calib_point_from_flash(adc, i, type, range);
		}
	}

	__ASSERT_NO_MSG(sequence_length > 0);

	/* Now that scheduling is done, we can set the sequence length */
	LL_ADC_SetSequenceLength(adc, sequence_length);

	/* Save unsampled channels (if any) for next sequence */
	data->unsampled_channels = remaining_unsampled;

#if defined(CONFIG_ADC_STM32_DMA)
	const struct adc_stm32wb0_config *config = data->dev->config;
	int err;

	/* Save sequence length in driver data for later usage */
	data->sequence_length = sequence_length;

	/* Prepare the DMA controller for ADC->memory transfers */
	data->dma_block_config.source_address = (uint32_t)&adc->DS_DATAOUT;
	data->dma_block_config.dest_address = (uint32_t)data->next_sample_ptr;
	data->dma_block_config.block_size = data->sequence_length * sizeof(uint16_t);

	err = dma_config(config->dmac, config->dma_channel, &data->dmac_config);
	if (err < 0) {
		LOG_ERR("%s: FAIL - dma_config returns %d", __func__, err);
		adc_context_complete(&data->ctx, err);
		return;
	}

	err = dma_start(config->dmac, config->dma_channel);
	if (err < 0) {
		LOG_ERR("%s: FAIL - dma_start returns %d", __func__, err);
		adc_context_complete(&data->ctx, err);
		return;
	}
#endif

	/* Start conversion sequence */
	LL_ADC_StartConversion(adc);
}

static inline void handle_end_of_sequence(ADC_TypeDef *adc, struct adc_stm32wb0_data *data)
{
	if (data->unsampled_channels != 0) {
		/* Some channels requested for this round have
		 * not been sampled yet. Schedule and start another
		 * acquisition sequence.
		 */
		schedule_and_start_adc_sequence(adc, data);
	} else {
		/* All channels sampled: round is complete. */
		adc_context_on_sampling_done(&data->ctx, data->dev);
	}
}

static int initiate_read_operation(const struct device *dev,
				const struct adc_sequence *sequence)
{
	const struct adc_stm32wb0_config *config = dev->config;
	struct adc_stm32wb0_data *data = dev->data;
	ADC_TypeDef *adc = (ADC_TypeDef *)config->reg;
	int err;

	err = validate_adc_sequence(sequence);
	if (err < 0) {
		return err;
	}

	/* Take ADC out of idle mode before getting to work */
	err = adc_exit_idle_mode(adc, &config->ana_clk);
	if (err < 0) {
		return err;
	}

	/* Initialize output pointers to first byte of user buffer */
	data->next_sample_ptr = data->round_buf_pointer = sequence->buffer;

	/* Configure resolution */
	LL_ADC_SetDSDataOutputWidth(adc, ds_width_from_adc_res(sequence->resolution));

	/* Configure oversampling */
	LL_ADC_SetDSDataOutputRatio(adc, sequence->oversampling);

	/* Start reading using the ADC */
	adc_context_start_read(&data->ctx, sequence);

	return 0;
}

#if !defined(CONFIG_ADC_STM32_DMA)
void adc_stm32wb0_isr(const struct device *dev)
{
	const struct adc_stm32wb0_config *config = dev->config;
	struct adc_stm32wb0_data *data = dev->data;
	ADC_TypeDef *adc = config->reg;

	/* Down sampler output data available */
	if (LL_ADC_IsActiveFlag_EODS(adc)) {
		/* Clear pending interrupt flag */
		LL_ADC_ClearFlag_EODS(adc);

		/* Write ADC data to output buffer and update pointer */
		*data->next_sample_ptr++ = LL_ADC_DSGetOutputData(adc);
	}

	/* Down sampler overflow detected - return error */
	if (LL_ADC_IsActiveFlag_OVRDS(adc)) {
		LL_ADC_ClearFlag_OVRDS(adc);

		LOG_ERR("ADC overflow\n");

		adc_context_complete(&data->ctx, -EIO);
		return;
	}

	if (!LL_ADC_IsActiveFlag_EOS(adc)) {
		/* ADC sequence not finished yet */
		return;
	}

	/* Clear pending interrupt flag */
	LL_ADC_ClearFlag_EOS(adc);

	/* Execute end-of-sequence logic */
	handle_end_of_sequence(adc, data);
}
#else /* CONFIG_ADC_STM32_DMA */
static void adc_stm32wb0_dma_callback(const struct device *dma, void *user_data,
					uint32_t dma_channel, int dma_status)
{
	struct adc_stm32wb0_data *data = user_data;
	const struct device *dev = data->dev;
	const struct adc_stm32wb0_config *config = dev->config;
	ADC_TypeDef *adc = config->reg;
	int err;

	/* N.B.: some of this code is borrowed from existing ADC driver,
	 * but may be not applicable to STM32WB0 series' ADC.
	 */
	if (dma_channel == config->dma_channel) {
		if (LL_ADC_IsActiveFlag_OVRDS(adc) || (dma_status >= 0)) {
			/* Sequence finished - update driver data accordingly */
			data->next_sample_ptr += data->sequence_length;

			/* Stop the DMA controller */
			err = dma_stop(config->dmac, config->dma_channel);
			LOG_DBG("%s: dma_stop returns %d", __func__, err);

			LL_ADC_ClearFlag_OVRDS(adc);

			/* Execute the common end-of-sequence logic */
			handle_end_of_sequence(adc, data);
		} else { /* dma_status < 0 */
			LOG_ERR("%s: dma error %d", __func__, dma_status);
			LL_ADC_StopConversion(adc);

			err = dma_stop(config->dmac, config->dma_channel);

			LOG_DBG("dma_stop returns %d", err);

			adc_context_complete(&data->ctx, dma_status);
		}
	} else {
		LOG_DBG("dma_channel 0x%08X != config->dma_channel 0x%08X",
			dma_channel, config->dma_channel);
	}
}
#endif /* !CONFIG_ADC_STM32_DMA */

/**
 * adc_context API implementation
 */
static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_stm32wb0_data *data = drv_data_from_adc_ctx(ctx);
	const struct adc_stm32wb0_config *config = data->dev->config;

	/* Mark all channels of this round as unsampled */
	data->unsampled_channels = data->ctx.sequence.channels;

	/* Schedule and start first sequence of this round */
	schedule_and_start_adc_sequence(config->reg, data);
}

static void adc_context_update_buffer_pointer(
	struct adc_context *ctx, bool repeat_sampling)
{
	struct adc_stm32wb0_data *data = drv_data_from_adc_ctx(ctx);

	if (repeat_sampling) {
		/* Roll back output pointer to address of first sample in round */
		data->next_sample_ptr = data->round_buf_pointer;
	} else /* a new round is starting: */ {
		/* Save address of first sample in round in case we have to repeat it */
		data->round_buf_pointer = data->next_sample_ptr;
	}
}

static void adc_context_on_complete(struct adc_context *ctx, int status)
{
	struct adc_stm32wb0_data *data = drv_data_from_adc_ctx(ctx);
	const struct adc_stm32wb0_config *config = data->dev->config;

	ARG_UNUSED(status);

	/**
	 * All ADC operations are complete.
	 * Save power by placing ADC in idle mode.
	 */
	adc_enter_idle_mode(config->reg, &config->ana_clk);

	/* Prevent data corruption if something goes wrong. */
	data->next_sample_ptr = NULL;
}

/**
 * Driver subsystem API implementation
 */
int adc_stm32wb0_channel_setup(const struct device *dev,
				const struct adc_channel_cfg *channel_cfg)
{
	CHECKIF(dev == NULL) { return -ENODEV; }
	CHECKIF(channel_cfg == NULL) { return -EINVAL; }
	const bool is_diff_channel =
		(channel_cfg->channel_id == LL_ADC_CHANNEL_VINP0_VINM0
		|| channel_cfg->channel_id == LL_ADC_CHANNEL_VINP1_VINM1
		|| channel_cfg->channel_id == LL_ADC_CHANNEL_VINP2_VINM2
		|| channel_cfg->channel_id == LL_ADC_CHANNEL_VINP3_VINM3);
	const uint8_t vin_range =  vinput_range_from_adc_ref(channel_cfg->reference);
	const uint32_t channel_id = channel_cfg->channel_id;
	struct adc_stm32wb0_data *data = dev->data;
	int res;

	/* Forbid reconfiguration while operation in progress */
	res = k_sem_take(&data->ctx.lock, K_NO_WAIT);
	if (res < 0) {
		return res;
	}

	/* Validate channel configuration parameters */
	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("gain unsupported by hardware");
		res = -ENOTSUP;
		goto unlock_and_return;
	}

	if (vin_range == LL_ADC_VIN_RANGE_INVALID) {
		LOG_ERR("invalid channel voltage reference");
		res = -EINVAL;
		goto unlock_and_return;
	}

	if (channel_id >= LL_ADC_CHANNEL_MAX) {
		LOG_ERR("invalid channel id %d", channel_cfg->channel_id);
		res = -EINVAL;
		goto unlock_and_return;
	} else if (is_diff_channel != channel_cfg->differential) {
		/* channel_cfg->differential flag does not match
		 * with the selected channel's type
		 */
		LOG_ERR("differential flag does not match channel type");
		res = -EINVAL;
		goto unlock_and_return;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("acquisition time unsupported by hardware");
		res = -ENOTSUP;
		goto unlock_and_return;
	}

	/* Verify that the correct reference is selected for special channels */
	if (channel_id == LL_ADC_CHANNEL_VBAT && vin_range != LL_ADC_VIN_RANGE_3V6) {
		LOG_ERR("invalid reference for Vbat channel");
		res = -EINVAL;
		goto unlock_and_return;
	} else if (channel_id == LL_ADC_CHANNEL_TEMPSENSOR && vin_range != LL_ADC_VIN_RANGE_1V2) {
		LOG_ERR("invalid reference for temperature sensor channel");
		res = -EINVAL;
		goto unlock_and_return;
	}

	/* Save the channel configuration in driver data.
	 * Note that the only configuration option available
	 * is the ADC channel reference (= Vinput range).
	 */
	data->channel_config[channel_id].vinput_range = vin_range;

unlock_and_return:
	/* Unlock the instance after updating configuration */
	k_sem_give(&data->ctx.lock);

	return res;
}

int adc_stm32wb0_read(const struct device *dev,
			const struct adc_sequence *sequence)
{
	CHECKIF(dev == NULL) { return -ENODEV; }
	struct adc_stm32wb0_data *data = dev->data;
	int err;

	adc_context_lock(&data->ctx, false, NULL);

	/* When context is locked in synchronous mode, this
	 * function blocks until the whole operation is complete.
	 */
	err = initiate_read_operation(dev, sequence);

	if (err >= 0) {
		err = adc_context_wait_for_completion(&data->ctx);
	} else {
		adc_release_pm_locks();
	}

	adc_context_release(&data->ctx, err);

	return err;
}

#if defined(CONFIG_ADC_ASYNC)
int adc_stm32wb0_read_async(const struct device *dev,
	const struct adc_sequence *sequence, struct k_poll_signal *async)
{
	CHECKIF(dev == NULL) { return -ENODEV; }
	struct adc_stm32wb0_data *data = dev->data;
	int err;

	adc_context_lock(&data->ctx, true, async);

	/* When context is locked in synchronous mode, this
	 * function blocks until the whole operation is complete.
	 */
	err = initiate_read_operation(dev, sequence);
	if (err < 0) {
		adc_release_pm_locks();
	}

	adc_context_release(&data->ctx, err);

	return err;
}
#endif /* CONFIG_ADC_ASYNC */

static const struct adc_driver_api api_stm32wb0_driver_api = {
	.channel_setup = adc_stm32wb0_channel_setup,
	.read = adc_stm32wb0_read,
#if defined(CONFIG_ADC_ASYNC)
	.read_async = adc_stm32wb0_read_async,
#endif /* CONFIG_ADC_ASYNC */
	.ref_internal = 3600U	/* ADC_REF_INTERNAL is mapped to Vinput 3.6V range */
};

int adc_stm32wb0_init(const struct device *dev)
{
	const struct device *clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct adc_stm32wb0_config *config = dev->config;
	struct adc_stm32wb0_data *data = dev->data;
	ADC_TypeDef *adc = config->reg;
	int err;

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* Turn on ADC digital clock (always on) */
	err = clock_control_on(clk,
		(clock_control_subsys_t)&config->dig_clk);
	if (err < 0) {
		LOG_ERR("failed to turn on ADC digital clock (%d)", err);
		return err;
	}

	/* Configure DT-provided signals when available */
	err = pinctrl_apply_state(config->pinctrl_cfg, PINCTRL_STATE_DEFAULT);
	if (err < 0 && err != -ENOENT) {
		/* ENOENT indicates no entry - should not be treated as failure */
		LOG_ERR("fail to apply ADC pinctrl state (%d)", err);
		return err;
	}

#if defined(ADC_SUPPORT_AUDIO_FEATURES)
	/* Configure ADC for analog sampling */
	LL_ADC_SetADCMode(adc, LL_ADC_OP_MODE_ADC);
#endif

#if defined(PWR_CR2_ENTS)
	/* Enable on-die temperature sensor */
	LL_PWR_EnableTempSens();
#endif

	/* Set ADC sample rate to 1 Msps (maximum speed) */
	LL_ADC_SetSampleRate(adc, LL_ADC_SAMPLE_RATE_16);

	/* Keep new data on overrun, if it ever happens */
	LL_ADC_SetOverrunDS(adc, LL_ADC_NEW_DATA_IS_KEPT);

#if !defined(CONFIG_ADC_STM32_DMA)
	/* Attach ISR and enable ADC interrupt in NVIC */
	IRQ_CONNECT(DT_IRQN(ADC_NODE), DT_IRQ(ADC_NODE, priority),
		adc_stm32wb0_isr, DEVICE_DT_GET(ADC_NODE), 0);
	irq_enable(DT_IRQN(ADC_NODE));

	/* Enable ADC interrupt after each sampling.
	 * NOTE: enabling EOS interrupt is not necessary because
	 * the EODS interrupt flag is also set to high when the
	 * EOS flag is being set to high.
	 */
	LL_ADC_EnableIT_EODS(adc);
#else
	/* Check that DMA controller exists and is ready to be used */
	if (!config->dmac) {
		LOG_ERR("no DMA assigned to ADC in DMA driver mode!");
		return -ENODEV;
	}

	if (!device_is_ready(config->dmac)) {
		LOG_ERR("DMA controller '%s' for ADC not ready", config->dmac->name);
		return -ENODEV;
	}

	/* Finalize DMA configuration structure in driver data */
	data->dmac_config.head_block = &data->dma_block_config;
	data->dmac_config.user_data = data;

	/* Enable DMA datapath in ADC */
	LL_ADC_DMAModeDSEnable(adc);
#endif /* !CONFIG_ADC_STM32_DMA */

	/* Unlock the ADC context to mark ADC as ready to use */
	adc_context_unlock_unconditionally(&data->ctx);

	/* Keep ADC powered down ("idle mode").
	 * It will be awakened on-demand when a call to the ADC API
	 * is performed by the application.
	 */
	return 0;
}

/**
 * Driver power management implementation
 */
#ifdef CONFIG_PM_DEVICE
static int adc_stm32wb0_pm_action(const struct device *dev,
			       enum pm_device_action action)
{
	const struct adc_stm32wb0_config *config = dev->config;
	ADC_TypeDef *adc = config->reg;
	int res;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return adc_stm32wb0_init(dev);
	case PM_DEVICE_ACTION_SUSPEND:
		adc_enter_idle_mode(adc, &config->ana_clk);

		/* Move pins to sleep state */
		res = pinctrl_apply_state(config->pinctrl_cfg, PINCTRL_STATE_SLEEP);

		/**
		 * -ENOENT is returned if there are no pins defined in DTS for sleep mode.
		 * This is fine and should not block PM from suspending the device.
		 * Silently ignore the error by returning 0 instead.
		 */
		if (res >= 0 || res == -ENOENT) {
			return 0;
		} else {
			return res;
		}
	default:
		return -ENOTSUP;
	}
}
#endif /* CONFIG_PM_DEVICE */

/**
 * Driver device instantiation
 */
PINCTRL_DT_DEFINE(ADC_NODE);

static const struct adc_stm32wb0_config adc_config = {
	.reg = (ADC_TypeDef *)DT_REG_ADDR(ADC_NODE),
	.pinctrl_cfg = PINCTRL_DT_DEV_CONFIG_GET(ADC_NODE),
	.dig_clk = STM32_CLOCK_INFO(0, ADC_NODE),
	.ana_clk = STM32_CLOCK_INFO(1, ADC_NODE),
#if defined(CONFIG_ADC_STM32_DMA)
	.dmac = DEVICE_DT_GET(DT_DMAS_CTLR_BY_IDX(ADC_NODE, 0)),
	.dma_channel = DT_DMAS_CELL_BY_IDX(ADC_NODE, 0, channel),
#endif
};

static struct adc_stm32wb0_data adc_data = {
	ADC_CONTEXT_INIT_TIMER(adc_data, ctx),
	ADC_CONTEXT_INIT_LOCK(adc_data, ctx),
	ADC_CONTEXT_INIT_SYNC(adc_data, ctx),
	.dev = DEVICE_DT_GET(ADC_NODE),
	.channel_config = {
		/* Internal channels selection is hardwired */
		[LL_ADC_CHANNEL_VBAT] = {
			.vinput_range = LL_ADC_VIN_RANGE_3V6
		},
		[LL_ADC_CHANNEL_TEMPSENSOR] = {
			.vinput_range = LL_ADC_VIN_RANGE_1V2
		}
	},
#if defined(CONFIG_ADC_STM32_DMA)
	.dmac_config = {
		.dma_slot = DT_INST_DMAS_CELL_BY_IDX(ADC_INSTANCE, 0, slot),
		.channel_direction = STM32_DMA_CONFIG_DIRECTION(
			STM32_DMA_CHANNEL_CONFIG_BY_IDX(ADC_INSTANCE, 0)),
		.channel_priority = STM32_DMA_CONFIG_PRIORITY(
			STM32_DMA_CHANNEL_CONFIG_BY_IDX(ADC_INSTANCE, 0)),
		.source_data_size = STM32_DMA_CONFIG_PERIPHERAL_DATA_SIZE(
			STM32_DMA_CHANNEL_CONFIG_BY_IDX(ADC_INSTANCE, 0)),
		.dest_data_size = STM32_DMA_CONFIG_MEMORY_DATA_SIZE(
			STM32_DMA_CHANNEL_CONFIG_BY_IDX(ADC_INSTANCE, 0)),
		.source_burst_length = 1,	/* SINGLE transfer */
		.dest_burst_length = 1,		/* SINGLE transfer */
		.block_count = 1,
		.dma_callback = adc_stm32wb0_dma_callback,
		/* head_block and user_data are initialized at runtime */
	},
	.dma_block_config = {
		.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE,
		.source_reload_en = 0,
		.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT,
		.dest_reload_en = 0,
	}
#endif
};

PM_DEVICE_DT_DEFINE(ADC_NODE, adc_stm32wb0_pm_action);

DEVICE_DT_DEFINE(ADC_NODE, &adc_stm32wb0_init, PM_DEVICE_DT_GET(ADC_NODE),
	&adc_data, &adc_config, POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,
	&api_stm32wb0_driver_api);
