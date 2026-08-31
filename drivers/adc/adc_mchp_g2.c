/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/irq.h>
#include <stdbool.h>
#if defined(CONFIG_SOC_FAMILY_MICROCHIP_PIC32CZ_CA)
#include <zephyr/dt-bindings/adc/mchp_pic32cz_ca_adc.h>
#endif /* SOC_FAMILY_MICROCHIP_PIC32CZ_CA */

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define DT_DRV_COMPAT microchip_adc_g2

LOG_MODULE_REGISTER(adc_mchp_g2, CONFIG_ADC_LOG_LEVEL);

#define ADC_CALC_TAD_NS(gclk_adc_hz, ctrl_clock_div, adc_div_ratio)                                \
	(((ctrl_clock_div + 1) * 2 * adc_div_ratio * 1000000000ULL) / gclk_adc_hz)

#define ADC_CALC_SAMPLE_COUNT(tad, res, acq_time_ns) ((((uint64_t)(acq_time_ns)) / tad) - res - 3)

#define ADC_CALC_SAMPLE_PHASE_NS(tad, samc) (((uint64_t)(samc) + 2) * (tad))

/* ADC resolution options (in bits) */
#define ADC_RESOLUTION_6BIT  6U
#define ADC_RESOLUTION_8BIT  8U
#define ADC_RESOLUTION_10BIT 10U
#define ADC_RESOLUTION_12BIT 12U

#define ADC_SAMPLE_COUNT_MIN     3U
#define ADC_SAMPLE_COUNT_DEFAULT 5U
#define ADC_SAMPLE_COUNT_MAX     1025U

#define ADC_MAX_OVERSAMPLING_VAL 8U

#define ADC_WKUPEXP_DELAY     10000U
#define ADC_VREF_STABLE_DELAY 20000U
#define TIMEOUT_VALUE_US      1000U
#define DELAY_US              2U

struct adc_mchp_dev_data {
	struct adc_context ctx;
	const struct device *dev;
	int16_t *buffer;
	int16_t *repeat_buffer;
	uint32_t channels;
	uint32_t freq; /* ADC operating gclk frequency in Hz. */
	uint32_t tad;  /* ADC Clock Period */
	uint8_t channel_id;
	uint32_t ch_initialized;
	struct adc_channel_cfg *saved_cfg;
};

struct adc_mchp_dev_config {
	adc_registers_t *regs;
	fuses_calotp_registers_t *fuses;
	const struct pinctrl_dev_config *pcfg;
	uint8_t adc_div_ratio; /* Division Ratio for ADC Sampling Clock */
	uint8_t core_id;
	uint8_t num_channels; /* Number of ADC channels. */
	void (*config_func)(const struct device *dev);
	const struct adc_global_mchp_config *global_cfg;
};

struct mchp_adc_clock {
	const struct device *clock_dev;
	clock_control_subsys_t mclk_sys;
	clock_control_subsys_t gclk_sys;
};

struct adc_global_mchp_config {
	adc_registers_t *regs;
	fuses_calotp_registers_t *fuses;
	struct mchp_adc_clock adc_clock;
	uint8_t num_cores;            /* Numbers of ADC cores */
	uint8_t ctrl_clock_div;       /* divides the GCLK_ADC input clock into ADC control clock */
	uint8_t adc_wkup_clock_count; /* Wake-Up TAD Clock Count */
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

/* Calculate sample count from acquisition time */
static uint32_t adc_get_sample_count(uint32_t tad, uint8_t res, uint16_t acq_time)
{
	uint32_t sample_count, acq_time_ns;

	switch (ADC_ACQ_TIME_UNIT(acq_time)) {
	case ADC_ACQ_TIME_TICKS:
		acq_time_ns = k_ticks_to_ns_floor64(ADC_ACQ_TIME_VALUE(acq_time));
		break;
	case ADC_ACQ_TIME_MICROSECONDS:
		acq_time_ns = (ADC_ACQ_TIME_VALUE(acq_time)) * 1000;
		break;
	case ADC_ACQ_TIME_NANOSECONDS:
		acq_time_ns = ADC_ACQ_TIME_VALUE(acq_time);
		break;
	default:
		/* Unsupported acquisition time unit or ADC_ACQ_TIME_DEFAULT */
		return ADC_SAMPLE_COUNT_DEFAULT;
	}
	sample_count = ADC_CALC_SAMPLE_COUNT(tad, res, acq_time_ns);

	/* Clip if value went out of range */
	sample_count = (sample_count < ADC_SAMPLE_COUNT_MIN) ? ADC_SAMPLE_COUNT_MIN : sample_count;
	sample_count = (sample_count > ADC_SAMPLE_COUNT_MAX) ? ADC_SAMPLE_COUNT_MAX : sample_count;

	return sample_count;
}

static void adc_start_channel(const struct device *dev, struct adc_context *ctx)
{
	const struct adc_mchp_dev_config *const dev_cfg = dev->config;
	struct adc_mchp_dev_data *dev_data = dev->data;
	adc_registers_t *adc_reg = dev_cfg->regs;
	struct adc_channel_cfg *cfg;
	uint8_t core_id = dev_cfg->core_id;
	uint32_t reg_val;
	uint16_t sample_count;
	uint64_t sample_phase_ns;

	/* Determine the next channel to process by finding the least significant bit set */
	dev_data->channel_id = find_lsb_set(dev_data->channels) - 1;

	/* Get the configuration for the selected channel, which is already validated. */
	cfg = &(dev_data->saved_cfg[dev_data->channel_id]);

	/* Apply reference selection */
	reg_val = adc_reg->ADC_CTRLD & (~ADC_CTRLD_VREFSEL_Msk);
	if (cfg->reference == ADC_REF_VDD_1) {
		adc_reg->ADC_CTRLD = reg_val | ADC_CTRLD_VREFSEL_AVDD_AVSS;
	} else {
		adc_reg->ADC_CTRLD = reg_val | ADC_CTRLD_VREFSEL_EXTERNAL_VREFH_AVSS;
	}

	/* Select the channel(s) to be used in this conversion */
	if ((cfg->differential == true) && (core_id != ADC_CORES_NO_DIFF_SUPPORT)) {
		/* Differential mode: output data is two's complementary signed */
		reg_val = adc_reg->CONFIG[core_id].ADC_CHNCFG3 &
			  ~(ADC_CHNCFG3_DIFF_Msk | ADC_CHNCFG3_SIGN_Msk);
		adc_reg->CONFIG[core_id].ADC_CHNCFG3 = reg_val |
						       ADC_CHNCFG3_DIFF(1 << cfg->channel_id) |
						       ADC_CHNCFG3_SIGN(1 << cfg->channel_id);
	}

	/* Enable the ADC controller */
	adc_reg->ADC_CTRLA |= ADC_CTRLA_ENABLE_Msk;
	adc_wait_synchronization(adc_reg);

	/* Wait for voltage reference to be stable. It will only be updated if CTRLA.ENABLE is on */
	if (WAIT_FOR(((adc_reg->ADC_CTLINTFLAG & ADC_CTLINTFLAG_VREFRDY_Msk) ==
		      ADC_CTLINTFLAG_VREFRDY_Msk),
		     ADC_VREF_STABLE_DELAY, k_busy_wait(DELAY_US)) == false) {
		LOG_ERR("Timeout waiting for ADC_VREF_STABLE_DELAY");
	}

/* Select core/channel and Enable Software Controlled Conversion */
#if defined(CONFIG_ADC_MCHP_G2_MULTI_CORE)

	reg_val = adc_reg->ADC_CTRLB &
		  ~(ADC_CTRLB_SWCNVEN_Msk | ADC_CTRLB_ADCHSEL_Msk | ADC_CTRLB_ADCORSEL_Msk);
	adc_reg->ADC_CTRLB = reg_val | ADC_CTRLB_SWCNVEN_Msk | ADC_CTRLB_ADCHSEL(cfg->channel_id) |
			     ADC_CTRLB_ADCORSEL(core_id);

#else /* !CONFIG_ADC_MCHP_G2_MULTI_CORE */

	reg_val = adc_reg->ADC_CTRLB & ~(ADC_CTRLB_SWCNVEN_Msk | ADC_CTRLB_ADCHSEL_Msk);
	adc_reg->ADC_CTRLB = reg_val | ADC_CTRLB_SWCNVEN_Msk | ADC_CTRLB_ADCHSEL(cfg->channel_id);

#endif /* End of CONFIG_ADC_MCHP_G2_MULTI_CORE */

	adc_wait_synchronization(adc_reg);

	adc_reg->INT[core_id].ADC_INTENSET |= ADC_INTENSET_CHRDY(1 << cfg->channel_id);

	sample_count = adc_get_sample_count(dev_data->tad, ctx->sequence.resolution,
					    cfg->acquisition_time);
	sample_phase_ns = ADC_CALC_SAMPLE_PHASE_NS(dev_data->tad, sample_count);

	/* Start sampling the selected channel of the selcted core */
	adc_reg->ADC_CTRLB |= ADC_CTRLB_SAMP_Msk;
	adc_wait_synchronization(adc_reg);

	/* Hold SAMP for the specified sample phase: (SAMC+2)*TAD. */
	k_busy_wait((uint32_t)((sample_phase_ns + 999) / 1000));

	/* Request conversion of the held sample. SAMP must stay set until RQCNVRT
	 * has been issued and accepted (hardware auto-clears RQCNVRT).
	 */
	adc_reg->ADC_CTRLB |= ADC_CTRLB_RQCNVRT_Msk;
	adc_wait_synchronization(adc_reg);

	if (WAIT_FOR(((adc_reg->ADC_CTRLB & ADC_CTRLB_RQCNVRT_Msk) == 0), TIMEOUT_VALUE_US,
		     k_busy_wait(DELAY_US)) == false) {
		LOG_ERR("Timeout waiting for RQCNVRT to clear");
	}

	/* Now safe to stop sampling; the conversion has already latched the sample. */
	adc_reg->ADC_CTRLB &= ~ADC_CTRLB_SAMP_Msk;
	adc_wait_synchronization(adc_reg);
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

/* Function required to be implemented for adc_context. And will be called when a sampling (of one
 * or more channels, depending on the realized sequence) is to be started.
 */
static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_mchp_dev_data *dev_data = CONTAINER_OF(ctx, struct adc_mchp_dev_data, ctx);

	dev_data->channels = ctx->sequence.channels;

	adc_start_channel(dev_data->dev, ctx);
}

static int adc_set_oversampling(adc_registers_t *adc_reg, uint8_t oversampling, uint8_t core_id,
				uint8_t channel)
{
	uint8_t reg_val;

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
	 *
	 * Valid range: 0 to 8 (inclusive)
	 */
	if (oversampling > ADC_MAX_OVERSAMPLING_VAL) {
		LOG_ERR("Invalid oversampling: %d\n", oversampling);
		return -EINVAL;
	}

	if (oversampling > 0) {
		uint8_t oversam_reg_val[ADC_MAX_OVERSAMPLING_VAL] = {
			ADC_FLTCTRL_OVRSAM_2_SAMPLES_Val,   ADC_FLTCTRL_OVRSAM_4_SAMPLES_Val,
			ADC_FLTCTRL_OVRSAM_8_SAMPLES_Val,   ADC_FLTCTRL_OVRSAM_16_SAMPLES_Val,
			ADC_FLTCTRL_OVRSAM_32_SAMPLES_Val,  ADC_FLTCTRL_OVRSAM_64_SAMPLES_Val,
			ADC_FLTCTRL_OVRSAM_128_SAMPLES_Val, ADC_FLTCTRL_OVRSAM_256_SAMPLES_Val};

#if !defined(CONFIG_ADC_MCHP_G2_MULTI_CORE)

		reg_val =
			adc_reg->ADC_FLTCTRL & ~(ADC_FLTCTRL_OVRSAM_Msk | ADC_FLTCTRL_FLTCHNID_Msk);
		reg_val |= (ADC_FLTCTRL_FLTEN_Msk | ADC_FLTCTRL_FMODE_Msk |
			    ADC_FLTCTRL_FLTCHNID(channel) |
			    ADC_FLTCTRL_OVRSAM(oversam_reg_val[oversampling - 1]));

		adc_reg->ADC_FLTCTRL = reg_val;

#elif defined(CONFIG_ADC_MCHP_G2_MULTI_CORE)

		reg_val = adc_reg->ADC_FLTCTRL[core_id] &
			  ~(ADC_FLTCTRL_OVRSAM_Msk | ADC_FLTCTRL_FLTCHNID_Msk);
		reg_val |= (ADC_FLTCTRL_FLTEN_Msk | ADC_FLTCTRL_FMODE_Msk |
			    ADC_FLTCTRL_FLTCHNID(channel) |
			    ADC_FLTCTRL_OVRSAM(oversam_reg_val[oversampling - 1]));

		adc_reg->ADC_FLTCTRL[core_id] = reg_val;

#endif /* End of CONFIG_ADC_MCHP_G2_MULTI_CORE */
	}

	return 0;
}

static int adc_set_resolution(adc_registers_t *adc_reg, uint8_t resolution, uint8_t core_id)
{
	uint16_t resolution_val;
	uint16_t reg_val;

	reg_val = adc_reg->CONFIG[core_id].ADC_CORCTRL & ~(ADC_CORCTRL_SELRES_Msk);
	switch (resolution) {
	case ADC_RESOLUTION_6BIT:
		resolution_val = ADC_CORCTRL_SELRES_6_BITS_Val;
		break;
	case ADC_RESOLUTION_8BIT:
		resolution_val = ADC_CORCTRL_SELRES_8_BITS_Val;
		break;
	case ADC_RESOLUTION_10BIT:
		resolution_val = ADC_CORCTRL_SELRES_10_BITS_Val;
		break;
	case ADC_RESOLUTION_12BIT:
		resolution_val = ADC_CORCTRL_SELRES_12_BITS_Val;
		break;
	default:
		LOG_ERR("Invalid resolution: %d\n", resolution);
		return -EINVAL;
	}
	adc_reg->CONFIG[core_id].ADC_CORCTRL = reg_val | ADC_CORCTRL_SELRES(resolution_val);

	return 0;
}

static int adc_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	const struct adc_mchp_dev_config *const dev_cfg = dev->config;
	struct adc_mchp_dev_data *dev_data = dev->data;
	adc_registers_t *adc_reg = dev_cfg->regs;
	uint8_t core_id = dev_cfg->core_id;
	int ret;
	uint32_t channels, channel_count, index;

	if (sequence->channels == 0) {
		LOG_ERR("No channels selected!\n");
		return -EINVAL;
	}

	/* Set Resolution */
	ret = adc_set_resolution(adc_reg, sequence->resolution, core_id);
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
		if ((dev_data->ch_initialized & (1 << index)) == 0) {
			LOG_ERR("Channel is not initialized");
			return -EINVAL;
		}

		/* Set oversampling */
		ret = adc_set_oversampling(adc_reg, sequence->oversampling, core_id, index);
		if (ret != 0) {
			LOG_ERR("Invalid oversampling : %d\n", sequence->oversampling);
			return ret;
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
	 * adc_context_start_read --> adc_context_start_sampling() --> adc_start_channel()
	 */
	adc_context_start_read(&dev_data->ctx, sequence);

	/* Wait for all ADC conversions to complete, if it's a synchronous call */
	ret = adc_context_wait_for_completion(&dev_data->ctx);

	return ret;
}

/* Function required to be implemented for adc_context. And will be called when the sample buffer
 * pointer should be prepared for writing of next sampling results, the "repeat_sampling" parameter
 * indicates if the results should be written in the same place as before (when true) or as
 * consecutive ones (otherwise).
 */
static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct adc_mchp_dev_data *data = CONTAINER_OF(ctx, struct adc_mchp_dev_data, ctx);

	if (repeat_sampling == true) {
		data->buffer = data->repeat_buffer;
	}
}

static void adc_mchp_isr(const struct device *dev)
{
	const struct adc_mchp_dev_config *const dev_cfg = dev->config;
	struct adc_mchp_dev_data *dev_data = dev->data;
	adc_registers_t *adc_reg = dev_cfg->regs;
	uint16_t result;
	uint8_t core_id = dev_cfg->core_id;

	/* Clear interrupt. */
	adc_reg->INT[core_id].ADC_INTFLAG |= ADC_INTFLAG_CHRDYC(1 << dev_data->channel_id);

#if defined(CONFIG_ADC_MCHP_G2_MULTI_CORE)

	adc_reg->ADC_CORCHDATAID =
		(adc_reg->ADC_CORCHDATAID &
		 ~(ADC_CORCHDATAID_CHRDYID_Msk | ADC_CORCHDATAID_CORDYID_Msk)) |
		(dev_data->channel_id | ADC_CORCHDATAID_CORDYID(dev_cfg->core_id));

#else /* !CONFIG_ADC_MCHP_G2_MULTI_CORE */

	adc_reg->ADC_CORCHDATAID = (adc_reg->ADC_CORCHDATAID & ~(ADC_CORCHDATAID_CHRDYID_Msk)) |
				   (dev_data->channel_id);

#endif /* End of CONFIG_ADC_MCHP_G2_MULTI_CORE */

	result = adc_reg->ADC_CHRDYDAT & ADC_CHRDYDAT_CHRDYDAT_Msk;
	*dev_data->buffer = result;
	dev_data->buffer++;
	dev_data->channels &= ~BIT(dev_data->channel_id);

	if (dev_data->channels != 0) {
		/* If multiple channels are configured, continue sampling the next channel */
		adc_start_channel(dev, &dev_data->ctx);
	} else {
		/* Disable the ADC controller. */
		adc_reg->ADC_CTRLA &= ~ADC_CTRLA_ENABLE_Msk;
		adc_wait_synchronization(adc_reg);

		/* If no additional channels, notify that sampling is complete */
		adc_context_on_sampling_done(&dev_data->ctx, dev);
	}
}

static int adc_mchp_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_mchp_dev_data *data = dev->data;
	int ret;

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

static int adc_mchp_channel_setup(const struct device *dev,
				  const struct adc_channel_cfg *channel_cfg)
{
	struct adc_mchp_dev_data *dev_data = dev->data;
	const struct adc_mchp_dev_config *const dev_cfg = dev->config;
	uint8_t channel_id = channel_cfg->channel_id;
	uint8_t core_id = dev_cfg->core_id;

	if (channel_id >= dev_cfg->num_channels) {
		LOG_ERR("Invalid Channel id : %d\n", channel_id);
		return -EINVAL;
	}

	/* Mark as, not initialized */
	dev_data->ch_initialized &= ~(1 << channel_id);

	/* ADC doesn't have programmable gain */
	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Unsupported gain setting");
		return -ENOTSUP;
	}

	/* Validate reference */
	if ((channel_cfg->reference != ADC_REF_VDD_1) &&
	    (channel_cfg->reference != ADC_REF_EXTERNAL0)) {
		LOG_ERR("Invalid reference : %d\n", channel_cfg->reference);
		return -EINVAL;
	}

/* Validate differential configurations */
#if defined(CONFIG_ADC_MCHP_G2_MULTI_CORE)

	if ((channel_cfg->differential == true) && (core_id == ADC_CORES_NO_DIFF_SUPPORT)) {
		LOG_ERR("Invalid differential core\n");
		return -EINVAL;
	}

#endif /* End of CONFIG_ADC_MCHP_G2_MULTI_CORE */

	if ((channel_cfg->differential == true) && (channel_id != MCHP_ADC_AIN0) &&
	    (channel_id != MCHP_ADC_AIN2) && (channel_id != MCHP_ADC_AIN4)) {
		LOG_ERR("Invalid differential inputs\n");
		return -EINVAL;
	}

	/* Add the channel config to the saved_cfg array */
	dev_data->saved_cfg[channel_id] = *channel_cfg;

	/* If individual channel configuration supported, to do during channel sequencing. */
	dev_data->ch_initialized |= (1 << channel_id);

	return 0;
}

/*
 * The ADC peripheral has one shared register block.
 * Initialization is split across two device types, common and hardware instance specific.
 */
static int adc_mchp_global_init(const struct device *dev)
{
	const struct adc_global_mchp_config *const dev_cfg = dev->config;
	adc_registers_t *adc_reg = dev_cfg->regs;
	fuses_calotp_registers_t *fuses = dev_cfg->fuses;
	uint32_t reg_val;
	int ret = 0;

	/* Switch on ADC gclock */
	ret = clock_control_on(dev_cfg->adc_clock.clock_dev, dev_cfg->adc_clock.gclk_sys);
	if ((ret != 0) && (ret != -EALREADY)) {
		LOG_ERR("Failed to enable the GCLK for ADC: %d", ret);
		return ret;
	}

	/* Switch on ADC mclock */
	ret = clock_control_on(dev_cfg->adc_clock.clock_dev, dev_cfg->adc_clock.mclk_sys);
	if ((ret != 0) && (ret != -EALREADY)) {
		LOG_ERR("Failed to enable the MCLK for ADC: %d", ret);
		return ret;
	}

	/* Software reset */
	adc_reg->ADC_CTRLA |= ADC_CTRLA_SWRST_Msk;
	adc_wait_synchronization(adc_reg);

	/* Configure calibration value for all the ADC cores */
	for (int core = 0; core < dev_cfg->num_cores; core++) {
		adc_reg->CONFIG[core].ADC_CALCTRL = fuses->FUSES_FCCFG65;
	}

	/* Enables and powers up the ADC module */
	reg_val = adc_reg->ADC_CTRLA & (~ADC_CTRLA_ONDEMAND_Msk);
	adc_reg->ADC_CTRLA = reg_val | ADC_CTRLA_ANAEN_Msk;

	/* Configure ctrl_clock_div and wkupexp */
	reg_val = adc_reg->ADC_CTRLD & ~(ADC_CTRLD_CTLCKDIV_Msk | ADC_CTRLD_WKUPEXP_Msk);
	reg_val |= (ADC_CTRLD_CTLCKDIV(dev_cfg->ctrl_clock_div) |
		    ADC_CTRLD_WKUPEXP(dev_cfg->adc_wkup_clock_count));
	adc_reg->ADC_CTRLD = reg_val;

	/* Clear Analog and bias circuitry for the ADC (ANLEN) */
	adc_reg->ADC_CTRLD = (adc_reg->ADC_CTRLD & ~(ADC_CTRLD_ANLEN_Msk | ADC_CTRLD_CHNEN_Msk));

	return 0;
}

static int adc_mchp_init(const struct device *dev)
{
	const struct adc_mchp_dev_config *const dev_cfg = dev->config;
	struct adc_mchp_dev_data *dev_data = dev->data;
	adc_registers_t *adc_reg = dev_cfg->regs;
	uint8_t core_id = dev_cfg->core_id;
	uint32_t reg_val;
	int ret = 0;

	dev_data->dev = dev;

	if (core_id >= dev_cfg->global_cfg->num_cores) {
		LOG_ERR("Invalid Core id : %d\n", core_id);
		return -EINVAL;
	}

	/* Get ADC Clock Frequency */
	ret = clock_control_get_rate(dev_cfg->global_cfg->adc_clock.clock_dev,
				     dev_cfg->global_cfg->adc_clock.gclk_sys, &dev_data->freq);
	if (ret != 0) {
		LOG_ERR("Failed to get the clock rate for ADC: %d", ret);
		return ret;
	}

	/* Calculate tad and store */
	dev_data->tad = ADC_CALC_TAD_NS(dev_data->freq, dev_cfg->global_cfg->ctrl_clock_div,
					dev_cfg->adc_div_ratio);

	/* Configure pins */
	pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);

	/* Configure and enable IRQ */
	dev_cfg->config_func(dev);

	/* Configure adc_div_ratio for ADCn */
	reg_val = adc_reg->CONFIG[dev_cfg->core_id].ADC_CORCTRL & ~(ADC_CORCTRL_ADCDIV_Msk);
	reg_val |= ADC_CORCTRL_ADCDIV(dev_cfg->adc_div_ratio);
	adc_reg->CONFIG[dev_cfg->core_id].ADC_CORCTRL = reg_val;

	/* Configure Analog and bias circuitry and
	 * digital interface (CHNEN) for the ADC SAR Core n (ANLEN)
	 */
	adc_reg->ADC_CTRLD |=
		ADC_CTRLD_ANLEN(1 << dev_cfg->core_id) | ADC_CTRLD_CHNEN(1 << dev_cfg->core_id);

	/* Enable the ADC controller. */
	adc_reg->ADC_CTRLA |= ADC_CTRLA_ENABLE_Msk;
	adc_wait_synchronization(adc_reg);

	/* Wait for WKUPEXP delay to expire after which ADC SAR Core n is Ready (CRDY) */
	if (WAIT_FOR(((adc_reg->ADC_CTLINTFLAG &
		       (1 << (ADC_CTLINTFLAG_CRRDY_Pos + dev_cfg->core_id))) != 0),
		     ADC_WKUPEXP_DELAY, k_busy_wait(DELAY_US)) == false) {
		LOG_ERR("Timeout waiting for ADC_WKUPEXP_DELAY");
	}

	/* Disable the ADC controller. */
	adc_reg->ADC_CTRLA &= ~ADC_CTRLA_ENABLE_Msk;
	adc_wait_synchronization(adc_reg);

	/* Initialize ADC context */
	adc_context_unlock_unconditionally(&dev_data->ctx);

	return 0;
}

static DEVICE_API(adc, adc_mchp_api) = {
	.channel_setup = adc_mchp_channel_setup,
	.read = adc_mchp_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_mchp_read_async,
#endif
};

/* clang-format off */
#define ADC_MCHP_DEFINE_CONFIG_FUNC(c)                                                             \
	static void adc_mchp_config_##c(const struct device *dev)                                  \
	{                                                                                          \
		/* Placeholder for IRQ and calibration configuration */                            \
		IRQ_CONNECT(DT_IRQ_BY_IDX(c, 0, irq), DT_IRQ_BY_IDX(c, 0, priority), adc_mchp_isr, \
			    DEVICE_DT_GET(c), 0);                                                  \
		irq_enable(DT_IRQ_BY_IDX(c, 0, irq));                                              \
		return;                                                                            \
	}

#define ADC_MCHP_DATA_DEFN(c)                                                                      \
	static struct adc_channel_cfg adc_channel_config_##c[DT_PROP(c, num_channels)];            \
	static struct adc_mchp_dev_data adc_mchp_data_##c = {                                      \
		ADC_CONTEXT_INIT_TIMER(adc_mchp_data_##c, ctx),                                    \
		ADC_CONTEXT_INIT_LOCK(adc_mchp_data_##c, ctx),                                     \
		ADC_CONTEXT_INIT_SYNC(adc_mchp_data_##c, ctx),                                     \
		.saved_cfg = adc_channel_config_##c,                                               \
		.freq = 0,                                                                         \
	}

#define ADC_MCHP_CONFIG_DEFN(c, n)                                                                 \
	static void adc_mchp_config_##c(const struct device *dev);                                 \
	static struct adc_mchp_dev_config adc_mchp_cfg_##c = {                                     \
		.regs = (adc_registers_t *)DT_REG_ADDR_BY_NAME(DT_PARENT(c), adc),                 \
		.fuses = (fuses_calotp_registers_t *)DT_REG_ADDR_BY_NAME(DT_PARENT(c), fuses),     \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(c),                                              \
		.adc_div_ratio = DT_PROP(c, adc_div_ratio),                                        \
		.core_id = DT_PROP(c, core_id),                                                    \
		.num_channels = DT_PROP(c, num_channels),                                          \
		.config_func = adc_mchp_config_##c,                                                \
		.global_cfg = &adc_global_mchp_cfg_##n}

#define ADC_MCHP_CHILD_INIT(c, n)                                                                  \
	PINCTRL_DT_DEFINE(c);                                                                      \
	ADC_MCHP_CONFIG_DEFN(c, n);                                                                \
	ADC_MCHP_DATA_DEFN(c);                                                                     \
	DEVICE_DT_DEFINE(c, adc_mchp_init, NULL, &adc_mchp_data_##c, &adc_mchp_cfg_##c,            \
			    POST_KERNEL, CONFIG_ADC_CORE_INIT_PRIORITY, &adc_mchp_api);            \
	ADC_MCHP_DEFINE_CONFIG_FUNC(c);

#define ADC_MCHP_GLOBAL_INIT(n)                                                                   \
	static const struct adc_global_mchp_config adc_global_mchp_cfg_##n = {                    \
		.regs = (adc_registers_t *)DT_INST_REG_ADDR_BY_NAME(n, adc),                      \
		.fuses = (fuses_calotp_registers_t *)DT_INST_REG_ADDR_BY_NAME(n, fuses),          \
		.adc_clock.clock_dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, clock_parent)),           \
		.adc_clock.mclk_sys = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, subsystem)),  \
		.adc_clock.gclk_sys = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, gclk, subsystem)),  \
		.num_cores = DT_INST_PROP(n, num_cores),                                          \
		.ctrl_clock_div = DT_INST_PROP(n, ctrl_clock_div),                                \
		.adc_wkup_clock_count = DT_INST_PROP(n, adc_wkup_clock_count),                    \
	};                                                                                        \
	DEVICE_DT_INST_DEFINE(n, adc_mchp_global_init, NULL, NULL, &adc_global_mchp_cfg_##n,      \
			      POST_KERNEL, CONFIG_ADC_INIT_PRIORITY, NULL);                       \
	DT_INST_FOREACH_CHILD_STATUS_OKAY_VARGS(n, ADC_MCHP_CHILD_INIT, n)
/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(ADC_MCHP_GLOBAL_INIT)
