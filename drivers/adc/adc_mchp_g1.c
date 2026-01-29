/*
 * Copyright (c) 2025-2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#if defined(CONFIG_SOC_FAMILY_MICROCHIP_SAM_D5X_E5X) ||                                            \
	defined(CONFIG_SOC_FAMILY_MICROCHIP_PIC32CX_SG)
#include <zephyr/dt-bindings/adc/mchp_sam_d5x_e5x_adc.h>
#endif /* SOC_FAMILY_MICROCHIP_SAM_D5X_E5X || SOC_FAMILY_MICROCHIP_PIC32CX_SG */
#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define DT_DRV_COMPAT microchip_adc_g1

LOG_MODULE_REGISTER(adc_mchp_g1, CONFIG_ADC_LOG_LEVEL);

#define ADC_REGS ((const struct adc_mchp_dev_config *)(dev)->config)->regs
/* Calculate ADC sample length (SAMPLEN) from sampling time (ns), ADC GCLK (Hz),
 * and prescaler value.
 *
 * SAMPLEN = ((sampling_time_ns * gclk_adc_hz) / (prescaler_val * 1_000_000_000)) - 1
 */
#define ADC_CALC_SAMPLEN_NS(sampling_time_ns, gclk_adc_hz, prescaler_val)                          \
	((((uint64_t)(sampling_time_ns) * (gclk_adc_hz)) / ((prescaler_val) * 1000000000ULL)) - 1)

/* Gain correction constants scaled by 2048 for fixed-point arithmetic */
#define ADC_GAIN_CORR_1_2 1024             /* 0.5 gain scaled */
#define ADC_GAIN_CORR_2_3 ((2 * 2048) / 3) /* 0.666... gain scaled */
#define ADC_GAIN_CORR_4_5 ((4 * 2048) / 5) /* 0.8 gain scaled */
#define ADC_GAIN_CORR_1   2048             /* 1.0 gain scaled */

/* ADC resolution options (in bits) */
#define ADC_RESOLUTION_8BIT  8
#define ADC_RESOLUTION_10BIT 10
#define ADC_RESOLUTION_12BIT 12

/* ADC clock prescaler division factors */
#define ADC_PRESCALER_DIV_2   2
#define ADC_PRESCALER_DIV_4   4
#define ADC_PRESCALER_DIV_8   8
#define ADC_PRESCALER_DIV_16  16
#define ADC_PRESCALER_DIV_32  32
#define ADC_PRESCALER_DIV_64  64
#define ADC_PRESCALER_DIV_128 128
#define ADC_PRESCALER_DIV_256 256

#define ADC_DEFAULT_SAMPLEN 3

#define ADC_MAX_OVERSAMPLING_VAL ADC_AVGCTRL_SAMPLENUM_1024_Val

#define TIMEOUT_VALUE_US 1000
#define DELAY_US         2

/* If the SUPC API support is not available, define direct access
 * to SUPC registers and shortcuts for voltage reference register
 */
#ifndef MCHP_SUPC_API_SUPPORT_AVAILABLE
static supc_registers_t *SUPC = (supc_registers_t *)SUPC_REGS;
#define SUPC_VREF (SUPC->SUPC_VREF)
#endif /* MCHP_SUPC_API_SUPPORT_AVAILABLE */

/* PIC32CXSG family specific reserved inputs */
#if defined(CONFIG_SOC_FAMILY_MICROCHIP_PIC32CX_SG)
#define MCHP_ADC_INPUT_POS_RSV_0 0x1C
#define MCHP_ADC_INPUT_POS_RSV_1 0x1D
#endif /* CONFIG_SOC_FAMILY_MICROCHIP_PIC32CX_SG */

struct adc_mchp_channel_cfg {
	bool initialized;
	struct adc_channel_cfg channel_cfg;
};

struct adc_mchp_dev_data {
	struct adc_context ctx;
	const struct device *dev;
	int16_t *buffer;
	int16_t *repeat_buffer;
	uint32_t channels;
	uint32_t freq; /* ADC operating frequency in Hz. After div */
	uint8_t channel_id;
	struct adc_mchp_channel_cfg *channel_config;
};

struct mchp_adc_clock {
	const struct device *clock_dev;
	clock_control_subsys_t mclk_sys;
	clock_control_subsys_t gclk_sys;
};

struct adc_mchp_dev_config {
	adc_registers_t *regs;
	const struct pinctrl_dev_config *pcfg;
	struct mchp_adc_clock adc_clock;
	uint16_t prescaler;   /* Clock prescaler value for the ADC input clock. */
	uint8_t num_channels; /* Maximum number of ADC channels. */
	const uint8_t *calib;
	uint8_t calib_len; /* Number of calib cells */
	uintptr_t nvm_reg;
	void (*config_func)(const struct device *dev);
};

/* Wait for synchronization */
static inline void adc_wait_synchronization(adc_registers_t *adc_reg)
{
	if (WAIT_FOR(((adc_reg->ADC_SYNCBUSY & ADC_SYNCBUSY_Msk) == 0),
		     TIMEOUT_VALUE_US,                  /* 1 ms timeout */
		     k_busy_wait(DELAY_US)) == false) { /* 2 µs delay between polls */
		LOG_ERR("Timeout waiting for ADC_SYNCBUSY to clear");
	}
}

/* Enable ADC interrupt. */
static inline void adc_interrupt_enable(adc_registers_t *adc_reg)
{
	/* Enable the RESRDY Interrupt */
	adc_reg->ADC_INTENSET |= ADC_INTENSET_RESRDY(1);
}

/* Clear interrupt. */
static inline void adc_interrupt_clear(adc_registers_t *adc_reg)
{
	/* Clear the interrupt */
	adc_reg->ADC_INTFLAG = ADC_INTFLAG_RESRDY_Msk;
}

/* Enable or disable the ADC controller. */
static inline void adc_controller_enable(adc_registers_t *adc_reg)
{
	adc_reg->ADC_CTRLA |= ADC_CTRLA_ENABLE(1);
}

/* ADC Correction enable */
static inline void adc_correction_enable(adc_registers_t *adc_reg)
{
	adc_reg->ADC_CTRLB |= ADC_CTRLB_CORREN(1);
	adc_wait_synchronization(adc_reg);
}

/* Set ADC Correction OFFSET */
static inline void adc_set_offset_correction(adc_registers_t *adc_reg, int16_t offset_corr)
{
	adc_reg->ADC_OFFSETCORR = ADC_OFFSETCORR_OFFSETCORR(offset_corr);
	adc_wait_synchronization(adc_reg);
}

/* Trigger an ADC conversion. */
static inline void adc_trigger_conversion(adc_registers_t *adc_reg)
{
	adc_reg->ADC_SWTRIG |= ADC_SWTRIG_START(1);
}

/* Read the result of the ADC conversion. */
static inline uint16_t adc_get_conversion_result(adc_registers_t *adc_reg)
{
	return adc_reg->ADC_RESULT;
}

static inline int8_t adc_set_acq_time(adc_registers_t *adc_reg, uint16_t sample_length)
{
	/* Valid sample length: 0–63 */
	if (sample_length >= 64U) {
		LOG_ERR("Invalid Sample Length : %d\n", sample_length);
		return -EINVAL;
	}

	adc_reg->ADC_SAMPCTRL = ADC_SAMPCTRL_SAMPLEN(sample_length);
	adc_wait_synchronization(adc_reg);

	return 0;
}

static int8_t adc_validate_channel_params(enum adc_gain gain, enum adc_reference reference,
					  uint16_t sample_length, uint8_t input_positive,
					  uint8_t input_negative)
{
	/* Validate gain */
	if (gain != ADC_GAIN_1_2 && gain != ADC_GAIN_2_3 && gain != ADC_GAIN_4_5 &&
	    gain != ADC_GAIN_1) {
		LOG_ERR("Invalid gain : %d\n", gain);
		return -EINVAL;
	}

	/* Validate reference */
	if ((reference < ADC_REF_VDD_1) || (reference > ADC_REF_EXTERNAL1)) {
		LOG_ERR("Invalid reference : %d\n", reference);
		return -EINVAL;
	}

	/* Valid sample length range: 0–63 */
	if (sample_length >= 64U) {
		LOG_ERR("Invalid Sample Length : %d\n", sample_length);
		return -EINVAL;
	}

	/*
	 * Validate input_positive:
	 * - Must be <= MCHP_ADC_INPUT_POS_MAX
	 * - Range 0x10–0x17 is reserved
	 */
	if ((input_positive > MCHP_ADC_INPUT_POS_MAX) ||
	    ((input_positive >= MCHP_ADC_INPUT_POS_RSV_MIN) &&
	     (input_positive <= MCHP_ADC_INPUT_POS_RSV_MAX))) {
		LOG_ERR("Invalid input positive : %d\n", input_positive);
		return -EINVAL;
	}

	/* PIC32CXSG family specific reserved inputs */
#if defined(CONFIG_SOC_FAMILY_MICROCHIP_PIC32CX_SG)
	if ((input_positive == MCHP_ADC_INPUT_POS_RSV_0) ||
	    (input_positive == MCHP_ADC_INPUT_POS_RSV_1)) {
		LOG_ERR("Invalid input positive for PIC32CXSG: %d", input_positive);
		return -EINVAL;
	}
#endif /* CONFIG_SOC_FAMILY_MICROCHIP_PIC32CX_SG */

	/* Validate input_negative (0–7) */
	if (input_negative > MCHP_ADC_INPUT_NEG_MAX) {
		LOG_ERR("Invalid input negative : %d\n", input_negative);
		return -EINVAL;
	}

	return 0;
}

static int8_t adc_set_reference(adc_registers_t *adc_reg, enum adc_reference reference)
{
	uint8_t refctrl = 0;

	switch (reference) {
	case ADC_REF_VDD_1:
		refctrl = ADC_REFCTRL_REFSEL_INTVCC1 | ADC_REFCTRL_REFCOMP(1);
		break;
	case ADC_REF_VDD_1_2:
		refctrl = ADC_REFCTRL_REFSEL_INTVCC0 | ADC_REFCTRL_REFCOMP(1);
		break;
	case ADC_REF_INTERNAL:
		refctrl = ADC_REFCTRL_REFSEL_INTREF | ADC_REFCTRL_REFCOMP(1);
		break;
	case ADC_REF_EXTERNAL0:
		refctrl = ADC_REFCTRL_REFSEL_AREFA;
		break;
	case ADC_REF_EXTERNAL1:
		refctrl = ADC_REFCTRL_REFSEL_AREFB;
		break;
	case ADC_REF_VDD_1_3:
	case ADC_REF_VDD_1_4:
		LOG_ERR("ADC Selected reference is not supported: %d\n", reference);
		return -EINVAL;
	default:
		LOG_ERR("ADC Selected reference is not valid: %d\n", reference);
		return -EINVAL;
	}

	/* Apply reference selection */
	adc_reg->ADC_REFCTRL = refctrl;
	adc_wait_synchronization(adc_reg);

#ifndef MCHP_SUPC_API_SUPPORT_AVAILABLE
	/* Manually enable internal references */
	SUPC_VREF |= SUPC_VREF_SEL_2V4;
#endif

	return 0;
}

static int8_t adc_set_gain(adc_registers_t *adc_reg, enum adc_gain gain)
{
	uint16_t gain_corr = 0;

	/* Gain = 1 → no correction needed */
	if (gain == ADC_GAIN_1) {
		return 0;
	}

	/* Enable correction and clear offset */
	adc_correction_enable(adc_reg);
	adc_set_offset_correction(adc_reg, 0x0);

	/* Select gain correction factor */
	switch (gain) {
	case ADC_GAIN_1_2:
		gain_corr = ADC_GAIN_CORR_1_2;
		break;
	case ADC_GAIN_2_3:
		gain_corr = ADC_GAIN_CORR_2_3;
		break;
	case ADC_GAIN_4_5:
		gain_corr = ADC_GAIN_CORR_4_5;
		break;
	case ADC_GAIN_1:
		/* Normally unreachable, but kept for safety */
		gain_corr = ADC_GAIN_CORR_1;
		break;
	default:
		LOG_ERR("Invalid gain: %d\n", gain_corr);
		return -EINVAL;
	}

	/* Apply gain correction */
	adc_reg->ADC_GAINCORR = ADC_GAINCORR_GAINCORR(gain_corr);
	adc_wait_synchronization(adc_reg);

	return 0;
}

static int8_t adc_set_input_positive(adc_registers_t *adc_reg, uint8_t input_positive)
{
	/* Validate input_positive:
	 * - Must be <= 0x1E (highest valid value is DAC0)
	 * - Values 0x10 to 0x17 are reserved and must not be used
	 */
	if ((input_positive > MCHP_ADC_INPUT_POS_MAX) ||
	    ((input_positive >= MCHP_ADC_INPUT_POS_RSV_MIN) &&
	     (input_positive <= MCHP_ADC_INPUT_POS_RSV_MAX))) {
		LOG_ERR("Invalid input positive: %d\n", input_positive);
		return -EINVAL;
	}

	/* PIC32CXSG family specific reserved inputs */
#if defined(CONFIG_SOC_FAMILY_MICROCHIP_PIC32CX_SG)
	if ((input_positive == MCHP_ADC_INPUT_POS_RSV_0) ||
	    (input_positive == MCHP_ADC_INPUT_POS_RSV_1)) {
		LOG_ERR("Invalid input positive for PIC32CXSG: %d", input_positive);
		return -EINVAL;
	}
#endif /* CONFIG_SOC_FAMILY_MICROCHIP_PIC32CX_SG */

	/* Set the MUXPOS field in ADC_INPUTCTRL register */
	adc_reg->ADC_INPUTCTRL = ADC_INPUTCTRL_MUXPOS(input_positive);

#ifndef MCHP_SUPC_API_SUPPORT_AVAILABLE
	/* Manual SUPC configuration when required */
	switch (input_positive) {
#ifdef ADC_INPUTCTRL_MUXPOS_TEMP_Val
	case ADC_INPUTCTRL_MUXPOS_TEMP_Val:
		/* Temperature sensor – no SUPC configuration needed */
		break;

#endif /* ADC_INPUTCTRL_MUXPOS_TEMP_Val */
#ifdef ADC_INPUTCTRL_MUXPOS_PTAT_Val
	case ADC_INPUTCTRL_MUXPOS_PTAT_Val:
		/* Enable TSEN and select PTAT */
		SUPC_VREF |= SUPC_VREF_TSEN(1);
		SUPC_VREF &= ~SUPC_VREF_TSSEL_Msk; /* PTAT */
		break;

#endif /* ADC_INPUTCTRL_MUXPOS_PTAT_Val */
#ifdef ADC_INPUTCTRL_MUXPOS_CTAT_Val
	case ADC_INPUTCTRL_MUXPOS_CTAT_Val:
		/* Enable TSEN and select CTAT */
		SUPC_VREF |= SUPC_VREF_TSEN(1);
		SUPC_VREF |= SUPC_VREF_TSSEL(1); /* CTAT */
		break;

#endif /* ADC_INPUTCTRL_MUXPOS_CTAT_Val */
	case ADC_INPUTCTRL_MUXPOS_BANDGAP_Val:
		/* Enable 2.4V bandgap via SUPC */
		SUPC_VREF |= SUPC_VREF_VREFOE(1);
		SUPC_VREF |= SUPC_VREF_SEL_2V4;
		break;

	default:
		/* No SUPC configuration needed */
		break;
	}
#endif /* MCHP_SUPC_API_SUPPORT_AVAILABLE */

	return 0;
}

static int8_t adc_set_input_negative(adc_registers_t *adc_reg, uint8_t input_negative,
				     bool differential)
{
	/* Validate input_negative range (only valid for differential mode) */
	if (input_negative > MCHP_ADC_INPUT_NEG_MAX) {
		LOG_ERR("Invalid input negative: %d\n", input_negative);
		return -EINVAL;
	}

	if (differential == true) {
		/* Enable differential mode and set specified negative input */
		adc_reg->ADC_INPUTCTRL |= ADC_INPUTCTRL_DIFFMODE(1);
		adc_reg->ADC_INPUTCTRL |= ADC_INPUTCTRL_MUXNEG(input_negative);
	} else {
#if defined(CONFIG_SOC_FAMILY_MICROCHIP_SAM_D5X_E5X)
		/* In single-ended mode, connect negative input to GND */
		adc_reg->ADC_INPUTCTRL |= ADC_INPUTCTRL_MUXNEG(ADC_INPUTCTRL_MUXNEG_GND_Val);
#elif defined(CONFIG_SOC_FAMILY_MICROCHIP_PIC32CX_SG)
		adc_reg->ADC_INPUTCTRL |= ADC_INPUTCTRL_MUXNEG(ADC_INPUTCTRL_MUXNEG_AVSS_Val);
#endif /* CONFIG_SOC_FAMILY_MICROCHIP_PIC32CX_SG */
	}

	return 0;
}

static int8_t adc_set_oversampling(adc_registers_t *adc_reg, uint8_t oversampling)
{
	/*
	 * Oversampling configuration:
	 * 0x0 = 1 sample
	 * 0x1 = 2 samples
	 * 0x2 = 4 samples
	 * 0x3 = 8 samples
	 * 0x4 = 16 samples
	 * 0x5 = 32 samples
	 * 0x6 = 64 samples
	 * 0x7 = 128 samples
	 * 0x8 = 256 samples
	 * 0x9 = 512 samples
	 * 0xA = 1024 samples
	 *
	 * Valid range: 0 to 10 (inclusive)
	 */
	if (oversampling > ADC_MAX_OVERSAMPLING_VAL) {
		LOG_ERR("Invalid oversampling: %d\n", oversampling);
		return -EINVAL;
	}

	adc_reg->ADC_SWTRIG = ADC_AVGCTRL_SAMPLENUM(oversampling);
	adc_wait_synchronization(adc_reg);

	return 0;
}

static int8_t adc_set_resolution(adc_registers_t *adc_reg, uint8_t resolution, uint8_t oversampling)
{
	uint16_t resolution_val;

	switch (resolution) {
	case ADC_RESOLUTION_8BIT:
		if (oversampling != 0) {
			LOG_ERR("Invalid oversampling: %d\n", oversampling);
			return -EINVAL;
		}
		resolution_val = ADC_CTRLB_RESSEL_8BIT;
		break;
	case ADC_RESOLUTION_10BIT:
		if (oversampling != 0) {
			LOG_ERR("Invalid oversampling: %d\n", oversampling);
			return -EINVAL;
		}
		resolution_val = ADC_CTRLB_RESSEL_10BIT;
		break;
	case ADC_RESOLUTION_12BIT:
		if (oversampling != 0) {
			resolution_val = ADC_CTRLB_RESSEL_16BIT;
		} else {
			resolution_val = ADC_CTRLB_RESSEL_12BIT;
		}
		break;
	default:
		LOG_ERR("Invalid oversampling: %d\n", oversampling);
		return -EINVAL;
	}

	adc_reg->ADC_CTRLB = resolution_val;
	adc_wait_synchronization(adc_reg);

	return 0;
}

static int8_t adc_set_prescalar(adc_registers_t *adc_reg, uint16_t prescaler)
{
	int8_t ret = 0;
	uint16_t prescaler_val = 0;

	switch (prescaler) {
	case ADC_PRESCALER_DIV_2:
		prescaler_val = ADC_CTRLA_PRESCALER_DIV2;
		break;
	case ADC_PRESCALER_DIV_4:
		prescaler_val = ADC_CTRLA_PRESCALER_DIV4;
		break;
	case ADC_PRESCALER_DIV_8:
		prescaler_val = ADC_CTRLA_PRESCALER_DIV8;
		break;
	case ADC_PRESCALER_DIV_16:
		prescaler_val = ADC_CTRLA_PRESCALER_DIV16;
		break;
	case ADC_PRESCALER_DIV_32:
		prescaler_val = ADC_CTRLA_PRESCALER_DIV32;
		break;
	case ADC_PRESCALER_DIV_64:
		prescaler_val = ADC_CTRLA_PRESCALER_DIV64;
		break;
	case ADC_PRESCALER_DIV_128:
		prescaler_val = ADC_CTRLA_PRESCALER_DIV128;
		break;
	case ADC_PRESCALER_DIV_256:
		prescaler_val = ADC_CTRLA_PRESCALER_DIV256;
		break;
	default:
		/*
		 * Fall back to default prescaler if the provided value is invalid.
		 * Acceptable values must be powers of 2 within the range 1–256.
		 * (prescaler <= 0 || prescaler > 256 || (prescaler & (prescaler - 1)))
		 * Notify user of fallback to ensure the ADC doesn't fail to initialize.
		 */
		prescaler_val = ADC_CTRLA_PRESCALER_DIV2;
		LOG_WRN("Warning: Invalid ADC prescaler value %d, using default (DIV2).\n",
			prescaler);
		break;
	}

	adc_reg->ADC_CTRLA = prescaler_val;
	adc_wait_synchronization(adc_reg);

	return ret;
}

static int8_t adc_select_channels(const struct device *dev, struct adc_channel_cfg *channel_config)
{
	int8_t ret;

	/* Set channels */
	ret = adc_set_input_positive(ADC_REGS, channel_config->input_positive);
	if (ret != 0) {
		LOG_ERR("Invalid input_positive : %d\n", channel_config->input_positive);
		return ret;
	}

	/* Set channels */
	ret = adc_set_input_negative(ADC_REGS, channel_config->input_negative,
				     channel_config->differential);
	if (ret != 0) {
		LOG_ERR("Invalid input_negative : %d\n", channel_config->input_negative);
	}

	return ret;
}

static int adc_get_sample_length(uint16_t acq_time, uint32_t adc_clk, uint8_t prescalar)
{
	uint16_t sample_length;

	switch (ADC_ACQ_TIME_UNIT(acq_time)) {
	case ADC_ACQ_TIME_TICKS:
		sample_length = ADC_ACQ_TIME_VALUE(acq_time) - 1;
		break;
	case ADC_ACQ_TIME_MICROSECONDS:
		sample_length = (uint16_t)ADC_CALC_SAMPLEN_NS(
			((ADC_ACQ_TIME_VALUE(acq_time)) * 1000), adc_clk, prescalar);
		break;
	case ADC_ACQ_TIME_NANOSECONDS:
		sample_length = (uint16_t)ADC_CALC_SAMPLEN_NS(ADC_ACQ_TIME_VALUE(acq_time), adc_clk,
							      prescalar);
		break;
	default:
		/* Unsupported acquisition time unit or ADC_ACQ_TIME_DEFAULT */
		sample_length = ADC_DEFAULT_SAMPLEN;
		break;
	}

	return sample_length;
}

static int8_t adc_apply_channel_config(const struct device *dev,
				       struct adc_channel_cfg *channel_config)
{
	int8_t ret;
	const struct adc_mchp_dev_config *const dev_cfg = dev->config;
	struct adc_mchp_dev_data *dev_data = dev->data;

	/* Set Acquisition time */
	uint16_t sample_length = adc_get_sample_length(channel_config->acquisition_time,
						       dev_data->freq, dev_cfg->prescaler);
	ret = adc_set_acq_time(ADC_REGS, sample_length);
	if (ret < 0) {
		LOG_ERR("Selected ADC acquisition time is not valid");
		return ret;
	}

	ret = adc_set_gain(ADC_REGS, channel_config->gain);
	if (ret < 0) {
		LOG_ERR("Invalid gain value: %d", channel_config->gain);
		return ret;
	}

	ret = adc_set_reference(ADC_REGS, channel_config->reference);
	if (ret < 0) {
		LOG_ERR("Invalid reference: %d", channel_config->reference);
	}

	return ret;
}

static void adc_start_channel(const struct device *dev)
{
	struct adc_mchp_dev_data *dev_data = dev->data;
	struct adc_mchp_channel_cfg *channel_config = NULL;

	/* Determine the next channel to process by finding the least significant bit set */
	dev_data->channel_id = find_lsb_set(dev_data->channels) - 1;

	/* Get the configuration for the selected channel */
	channel_config = &dev_data->channel_config[dev_data->channel_id];

	/* Channel configuration has already been validated earlier,
	 * so no need for error checking here.
	 */

	/* Apply configuration for the selected channel */
	adc_apply_channel_config(dev, &channel_config->channel_cfg);

	/* Select the channel(s) to be used in this conversion */
	adc_select_channels(dev, &channel_config->channel_cfg);

	/* Start the ADC conversion */
	adc_trigger_conversion(ADC_REGS);
}

static int adc_check_buffer_size(const struct adc_sequence *sequence, uint8_t active_channels)
{
	size_t needed_buffer_size;

	needed_buffer_size = active_channels * sizeof(uint16_t);
	if (sequence->options != NULL) {
		needed_buffer_size *= (1U + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed_buffer_size) {
		LOG_ERR("Provided buffer is too small (%u/%u)", sequence->buffer_size,
			needed_buffer_size);
		return -ENOMEM;
	}

	return 0;
}

static int adc_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	const struct adc_mchp_dev_config *const dev_cfg = dev->config;
	struct adc_mchp_dev_data *dev_data = dev->data;
	int ret;
	uint32_t channels, channel_count, index;

	if (sequence->channels == 0) {
		LOG_ERR("No chennels selected!\n");
		return -EINVAL;
	}

	/* Set oversampling */
	ret = adc_set_oversampling(ADC_REGS, sequence->oversampling);
	if (ret != 0) {
		LOG_ERR("Invalid oversampling : %d\n", sequence->oversampling);
		return ret;
	}

	/* Set Resolution */
	ret = adc_set_resolution(ADC_REGS, sequence->resolution, sequence->oversampling);
	if (ret != 0) {
		LOG_ERR("Invalid resolution : %d\n", sequence->resolution);
		return ret;
	}

	/* Verify all requested channels are initialized and store resolution */
	channels = sequence->channels;
	channel_count = 0;
	while (channels != 0) {
		/* Iterate through all channels and check if they are initialized */
		index = find_lsb_set(channels) - 1;
		if (index >= dev_cfg->num_channels) {
			LOG_ERR("Invalid channel number : %d", index);
			return -EINVAL;
		}
		/* If the channels is not initialized return invalid */
		if (dev_data->channel_config[index].initialized == false) {
			LOG_ERR("Channel is not initialized");
			return -EINVAL;
		}
		channel_count++;
		channels &= ~BIT(index);
	}

	/* Check buffer */
	ret = adc_check_buffer_size(sequence, channel_count);
	if (ret != 0) {
		LOG_ERR("Check buffer size invalid\n");
		return ret;
	}

	/* Store buffer references for use during sampling */
	dev_data->buffer = sequence->buffer;
	dev_data->repeat_buffer = sequence->buffer;

	/* At this point we allow the scheduler to do other things while
	 * we wait for the conversions to complete. This is provided by the
	 * adc_context functions. However, the caller of this function is
	 * blocked until the results are in.
	 */
	adc_context_start_read(&dev_data->ctx, sequence);

	/* Wait for all ADC conversions to complete before returning, if it's a synchronous
	 * call
	 */
	ret = adc_context_wait_for_completion(&dev_data->ctx);

	return ret;
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_mchp_dev_data *dev_data = CONTAINER_OF(ctx, struct adc_mchp_dev_data, ctx);

	dev_data->channels = ctx->sequence.channels;

	adc_start_channel(dev_data->dev);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct adc_mchp_dev_data *data = CONTAINER_OF(ctx, struct adc_mchp_dev_data, ctx);

	if (repeat_sampling == true) {
		data->buffer = data->repeat_buffer;
	}
}

static void adc_mchp_isr(const struct device *dev)
{
	struct adc_mchp_dev_data *dev_data = dev->data;
	uint16_t result;

	adc_interrupt_clear(ADC_REGS);
	result = adc_get_conversion_result(ADC_REGS);
	*dev_data->buffer = result;
	dev_data->buffer++;
	dev_data->channels &= ~BIT(dev_data->channel_id);

	if (dev_data->channels != 0) {
		/* If multiple channels are configured, continue sampling the next channel */
		adc_start_channel(dev);
	} else {
		/* If no additional channels, notify that sampling is complete */
		adc_context_on_sampling_done(&dev_data->ctx, dev);
	}
}

static int adc_mchp_channel_setup(const struct device *dev,
				  const struct adc_channel_cfg *channel_cfg)
{
	struct adc_mchp_dev_data *dev_data = dev->data;
	const struct adc_mchp_dev_config *const dev_cfg = dev->config;
	struct adc_mchp_channel_cfg *channel_config;
	int8_t ret;
	uint16_t sample_length;

	if (channel_cfg->channel_id >= dev_cfg->num_channels) {
		LOG_ERR("Invalid Channel id : %d\n", channel_cfg->channel_id);
		return -EINVAL;
	}
	/* Update the channel configuration */
	channel_config = &dev_data->channel_config[channel_cfg->channel_id];
	channel_config->initialized = false;

	/* Calculate sample length in ADC clock cycles from acquisition time */
	sample_length = adc_get_sample_length(channel_cfg->acquisition_time, dev_data->freq,
					      dev_cfg->prescaler);

	/*
	 * If the device supports individual channel configuration,
	 * it can be configured during channel sequencing.
	 * Validate the channel configuration parameters accordingly.
	 */
	ret = adc_validate_channel_params(channel_cfg->gain, channel_cfg->reference, sample_length,
					  channel_cfg->input_positive, channel_cfg->input_negative);

	if (ret != 0) {
		LOG_ERR("Invalid ADC channel config");
		return ret;
	}

	/* Add the channel config to the channel_config array */
	channel_config->channel_cfg = *channel_cfg;
	channel_config->initialized = true;

	return ret;
}

static int adc_mchp_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_mchp_dev_data *data = dev->data;
	int ret = 0;

	adc_context_lock(&data->ctx, false, NULL);
	ret = adc_start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
}

#ifdef CONFIG_ADC_ASYNC

static int adc_mchp_read_async(const struct device *dev, const struct adc_sequence *sequence,
			       struct k_poll_signal *async)
{
	struct adc_mchp_dev_data *data = dev->data;
	int ret = 0;

	adc_context_lock(&data->ctx, true, async);
	ret = adc_start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
}
#endif /* CONFIG_ADC_ASYNC */

static inline void adc_init_factory_calib_value(const struct device *dev)
{
	const struct adc_mchp_dev_config *cfg = dev->config;
	uint32_t calib_reg = 0;
	uint32_t nvm_val;
	uint8_t adc_bit, nvm_bit, mask;

	/* Read NVM calibration value */
	nvm_val = sys_read32(cfg->nvm_reg);
	for (size_t i = 0; i < cfg->calib_len; i += 3) {
		adc_bit = cfg->calib[i];
		nvm_bit = cfg->calib[i + 1];
		mask = cfg->calib[i + 2];

		calib_reg |= ((nvm_val >> nvm_bit) & mask) << adc_bit;
	}
	ADC_REGS->ADC_CALIB = calib_reg;
}

static int adc_mchp_init(const struct device *dev)
{
	const struct adc_mchp_dev_config *const dev_cfg = dev->config;
	struct adc_mchp_dev_data *dev_data = dev->data;
	int8_t ret = 0;

	dev_data->dev = dev;
	/* Enable the ADC Clock */
	LOG_DBG("Clock dev: %p, gclk id: %d, mclk id: %d", dev_cfg->adc_clock.clock_dev,
		(uint32_t)(dev_cfg->adc_clock.gclk_sys), (uint32_t)(dev_cfg->adc_clock.mclk_sys));
	/* On Global clock for ADC */
	ret = clock_control_on(dev_cfg->adc_clock.clock_dev, dev_cfg->adc_clock.gclk_sys);
	if ((ret != 0) && (ret != -EALREADY)) {
		LOG_ERR("Failed to enable the GCLK for ADC: %d", ret);
		return ret;
	}

	/* On Main clock for ADC */
	ret = clock_control_on(dev_cfg->adc_clock.clock_dev, dev_cfg->adc_clock.mclk_sys);
	if ((ret != 0) && (ret != -EALREADY)) {
		LOG_ERR("Failed to enable the MCLK for ADC: %d", ret);
		return ret;
	}
	ret = (ret == -EALREADY) ? 0 : ret;

	/* Get ADC Clock Frequency */
	ret = clock_control_get_rate(dev_cfg->adc_clock.clock_dev, dev_cfg->adc_clock.gclk_sys,
				     &dev_data->freq);
	if (ret != 0) {
		LOG_ERR("Failed to get the clock rate for ADC: %d", ret);
		return ret;
	}

	adc_set_prescalar(ADC_REGS, dev_cfg->prescaler);
	pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	adc_interrupt_clear(ADC_REGS);
	dev_cfg->config_func(dev);
	adc_init_factory_calib_value(dev);
	adc_interrupt_enable(ADC_REGS);
	adc_controller_enable(ADC_REGS);
	adc_context_unlock_unconditionally(&dev_data->ctx);

	return ret;
}

static DEVICE_API(adc, adc_mchp_api) = {
	.channel_setup = adc_mchp_channel_setup,
	.read = adc_mchp_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_mchp_read_async,
#endif
};

#define ADC_MCHP_DEFINE_CONFIG_FUNC(n)                                                             \
	static void adc_mchp_config_##n(const struct device *dev)                                  \
	{                                                                                          \
		/* Placeholder for IRQ and calibration configuration */                            \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, resrdy, irq),                                   \
			    DT_INST_IRQ_BY_NAME(n, resrdy, priority), adc_mchp_isr,                \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQ_BY_NAME(n, resrdy, irq));                                   \
		return;                                                                            \
	}

#define ADC_MCHP_DATA_DEFN(n)                                                                      \
	static struct adc_mchp_channel_cfg adc_channel_config_##n[DT_INST_PROP(n, num_channels)];  \
	static struct adc_mchp_dev_data adc_mchp_data_##n = {                                      \
		ADC_CONTEXT_INIT_TIMER(adc_mchp_data_##n, ctx),                                    \
		ADC_CONTEXT_INIT_LOCK(adc_mchp_data_##n, ctx),                                     \
		ADC_CONTEXT_INIT_SYNC(adc_mchp_data_##n, ctx),                                     \
		.channel_config = adc_channel_config_##n,                                          \
		.freq = 0,                                                                         \
	}

#define ADC_MCHP_CONFIG_DEFN(n)                                                                    \
	static uint8_t calib_map_##n[] = DT_INST_PROP(n, calib_mapping);                           \
	static void adc_mchp_config_##n(const struct device *dev);                                 \
	static struct adc_mchp_dev_config adc_mchp_cfg_##n = {                                     \
		.regs = (adc_registers_t *)DT_INST_REG_ADDR(n),                                    \
		.config_func = adc_mchp_config_##n,                                                \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.prescaler = DT_INST_PROP(n, prescaler),                                           \
		.num_channels = DT_INST_PROP(n, num_channels),                                     \
		.nvm_reg = DT_REG_ADDR(DT_INST_PHANDLE(n, nvm_calib)),                             \
		.calib = calib_map_##n,                                                            \
		.calib_len = DT_INST_PROP_LEN(n, calib_mapping),                                   \
		.adc_clock.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),                         \
		.adc_clock.mclk_sys = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, subsystem)),   \
		.adc_clock.gclk_sys = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, gclk, subsystem))}

#define ADC_MCHP_DEVICE(n)                                                                         \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	ADC_MCHP_CONFIG_DEFN(n);                                                                   \
	ADC_MCHP_DATA_DEFN(n);                                                                     \
	DEVICE_DT_INST_DEFINE(n, adc_mchp_init, NULL, &adc_mchp_data_##n, &adc_mchp_cfg_##n,       \
			      POST_KERNEL, CONFIG_ADC_INIT_PRIORITY, &adc_mchp_api);               \
	ADC_MCHP_DEFINE_CONFIG_FUNC(n);

DT_INST_FOREACH_STATUS_OKAY(ADC_MCHP_DEVICE)
