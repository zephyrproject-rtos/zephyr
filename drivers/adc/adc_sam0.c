/*
 * Copyright (c) 2019 Derek Hageman <hageman@inthat.cloud>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam0_adc

#include <soc.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_sam0, CONFIG_ADC_LOG_LEVEL);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#if defined(CONFIG_SOC_SERIES_SAMD21) || defined(CONFIG_SOC_SERIES_SAMR21) || \
	defined(CONFIG_SOC_SERIES_SAMD20)
/*
 * SAMD21 Manual 33.6.2.1: The first conversion after changing the reference
 * is invalid, so we have to discard it.
 */
#define ADC_SAM0_REFERENCE_GLITCH 1
#endif

struct adc_sam0_data {
	struct adc_context ctx;
	const struct device *dev;

	uint16_t *buffer;

	/*
	 * Saved initial start, so we can reset the advances we've done
	 * if required
	 */
	uint16_t *repeat_buffer;

#ifdef ADC_SAM0_REFERENCE_GLITCH
	uint8_t reference_changed;
#endif
};

struct adc_sam0_cfg {
	Adc *regs;
	const struct pinctrl_dev_config *pcfg;

#ifdef MCLK
	uint32_t mclk_mask;
	uint32_t gclk_mask;
	uint16_t gclk_id;
#else
	uint32_t gclk;
#endif

	uint32_t freq;
	uint16_t prescaler;

	void (*config_func)(const struct device *dev);
};

static void wait_synchronization(Adc *const adc)
{
	while ((ADC_SYNC(adc) & ADC_SYNC_MASK) != 0) {
	}
}

static int adc_sam0_acquisition_to_clocks(const struct device *dev,
					  uint16_t acquisition_time)
{
	const struct adc_sam0_cfg *const cfg = dev->config;
	uint64_t scaled_acq;

	switch (ADC_ACQ_TIME_UNIT(acquisition_time)) {
	case ADC_ACQ_TIME_TICKS:
		if (ADC_ACQ_TIME_VALUE(acquisition_time) > 64U) {
			return -EINVAL;
		}

		return (int)ADC_ACQ_TIME_VALUE(acquisition_time) - 1;
	case ADC_ACQ_TIME_MICROSECONDS:
		scaled_acq = (uint64_t)ADC_ACQ_TIME_VALUE(acquisition_time) *
			     1000000U;
		break;
	case ADC_ACQ_TIME_NANOSECONDS:
		scaled_acq = (uint64_t)ADC_ACQ_TIME_VALUE(acquisition_time) *
			     1000U;
		break;
	default:
		return -EINVAL;
	}

	/*
	 * sample_time = (sample_length+1) * (clk_adc / 2)
	 * sample_length = sample_time * (2/clk_adc) - 1,
	 */

	scaled_acq *= 2U;
	scaled_acq += cfg->freq / 2U;
	scaled_acq /= cfg->freq;
	if (scaled_acq <= 1U) {
		return 0;
	}

	scaled_acq -= 1U;
	if (scaled_acq >= 64U) {
		return -EINVAL;
	}

	return (int)scaled_acq;
}

static int adc_sam0_channel_setup(const struct device *dev,
				  const struct adc_channel_cfg *channel_cfg)
{
	const struct adc_sam0_cfg *const cfg = dev->config;
	Adc *const adc = cfg->regs;
	int retval;
	uint8_t sampctrl = 0;

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		retval = adc_sam0_acquisition_to_clocks(dev,
							channel_cfg->acquisition_time);
		if (retval < 0) {
			LOG_ERR("Selected ADC acquisition time is not valid");
			return retval;
		}

		sampctrl |= ADC_SAMPCTRL_SAMPLEN(retval);
	}

	adc->SAMPCTRL.reg = sampctrl;
	wait_synchronization(adc);

	uint8_t refctrl;

	switch (channel_cfg->reference) {
	case ADC_REF_INTERNAL:
		refctrl = ADC_REFCTRL_REFSEL_INTERNAL | ADC_REFCTRL_REFCOMP;
		/* Enable the internal bandgap reference */
		ADC_BGEN = 1;
		break;
#ifdef ADC_REFCTRL_REFSEL_VDD_1
	case ADC_REF_VDD_1:
		refctrl = ADC_REFCTRL_REFSEL_VDD_1 | ADC_REFCTRL_REFCOMP;
		break;
#endif
	case ADC_REF_VDD_1_2:
		refctrl = ADC_REFCTRL_REFSEL_VDD_1_2 | ADC_REFCTRL_REFCOMP;
		break;
	case ADC_REF_EXTERNAL0:
		refctrl = ADC_REFCTRL_REFSEL_AREFA;
		break;
	case ADC_REF_EXTERNAL1:
		refctrl = ADC_REFCTRL_REFSEL_AREFB;
		break;
	default:
		LOG_ERR("Selected reference is not valid");
		return -EINVAL;
	}
	if (adc->REFCTRL.reg != refctrl) {
#ifdef ADC_SAM0_REFERENCE_ENABLE_PROTECTED
		adc->CTRLA.bit.ENABLE = 0;
		wait_synchronization(adc);
#endif
		adc->REFCTRL.reg = refctrl;
		wait_synchronization(adc);
#ifdef ADC_SAM0_REFERENCE_ENABLE_PROTECTED
		adc->CTRLA.bit.ENABLE = 1;
		wait_synchronization(adc);
#endif
#ifdef ADC_SAM0_REFERENCE_GLITCH
		struct adc_sam0_data *data = dev->data;

		data->reference_changed = 1;
#endif
	}


	uint32_t inputctrl = 0;

	switch (channel_cfg->gain) {
	case ADC_GAIN_1:
#ifdef ADC_INPUTCTRL_GAIN_1X
		inputctrl = ADC_INPUTCTRL_GAIN_1X;
#endif
		break;
#ifdef ADC_INPUTCTRL_GAIN_DIV2
	case ADC_GAIN_1_2:
		inputctrl = ADC_INPUTCTRL_GAIN_DIV2;
		break;
#endif
#ifdef ADC_INPUTCTRL_GAIN_2X
	case ADC_GAIN_2:
		inputctrl = ADC_INPUTCTRL_GAIN_2X;
		break;
#endif
#ifdef ADC_INPUTCTRL_GAIN_4X
	case ADC_GAIN_4:
		inputctrl = ADC_INPUTCTRL_GAIN_4X;
		break;
#endif
#ifdef ADC_INPUTCTRL_GAIN_8X
	case ADC_GAIN_8:
		inputctrl = ADC_INPUTCTRL_GAIN_8X;
		break;
#endif
#ifdef ADC_INPUTCTRL_GAIN_16X
	case ADC_GAIN_16:
		inputctrl = ADC_INPUTCTRL_GAIN_16X;
		break;
#endif
	default:
		LOG_ERR("Selected ADC gain is not valid");
		return -EINVAL;
	}

	inputctrl |= ADC_INPUTCTRL_MUXPOS(channel_cfg->input_positive);
	if (channel_cfg->differential) {
		inputctrl |= ADC_INPUTCTRL_MUXNEG(channel_cfg->input_negative);

		ADC_DIFF(adc) |= ADC_DIFF_MASK;
	} else {
		inputctrl |= ADC_INPUTCTRL_MUXNEG_GND;

		ADC_DIFF(adc) &= ~ADC_DIFF_MASK;
	}
	wait_synchronization(adc);

	adc->INPUTCTRL.reg = inputctrl;
	wait_synchronization(adc);

	/* Enable references if they're selected */
	switch (channel_cfg->input_positive) {
#ifdef ADC_INPUTCTRL_MUXPOS_TEMP_Val
	case ADC_INPUTCTRL_MUXPOS_TEMP_Val:
		ADC_TSEN = 1;
		break;
#endif
#ifdef ADC_INPUTCTRL_MUXPOS_PTAT_Val
	case ADC_INPUTCTRL_MUXPOS_PTAT_Val:
		ADC_TSEN = 1;
		break;
#endif
#ifdef ADC_INPUTCTRL_MUXPOS_CTAT_Val
	case ADC_INPUTCTRL_MUXPOS_CTAT_Val:
		ADC_TSEN = 1;
		break;
#endif
	case ADC_INPUTCTRL_MUXPOS_BANDGAP_Val:
		ADC_BGEN = 1;
		break;
	default:
		break;
	}


	return 0;
}

static void adc_sam0_start_conversion(const struct device *dev)
{
	const struct adc_sam0_cfg *const cfg = dev->config;
	Adc *const adc = cfg->regs;

	LOG_DBG("Starting conversion");

	adc->SWTRIG.reg = ADC_SWTRIG_START;
	/*
	 * Should be safe to not synchronize here because the only things
	 * that might access the ADC after this will wait for it to complete
	 * (synchronize finished implicitly)
	 */
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_sam0_data *data =
		CONTAINER_OF(ctx, struct adc_sam0_data, ctx);

	adc_sam0_start_conversion(data->dev);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat_sampling)
{
	struct adc_sam0_data *data =
		CONTAINER_OF(ctx, struct adc_sam0_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static int check_buffer_size(const struct adc_sequence *sequence,
			     uint8_t active_channels)
{
	size_t needed_buffer_size;

	needed_buffer_size = active_channels * sizeof(uint16_t);
	if (sequence->options) {
		needed_buffer_size *= (1U + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed_buffer_size) {
		LOG_ERR("Provided buffer is too small (%u/%u)",
			sequence->buffer_size, needed_buffer_size);
		return -ENOMEM;
	}
	return 0;
}

static int start_read(const struct device *dev,
		      const struct adc_sequence *sequence)
{
	const struct adc_sam0_cfg *const cfg = dev->config;
	struct adc_sam0_data *data = dev->data;
	Adc *const adc = cfg->regs;
	int error;

	if (sequence->oversampling > 10U) {
		LOG_ERR("Invalid oversampling");
		return -EINVAL;
	}

	adc->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM(sequence->oversampling);
	/* AVGCTRL is not synchronized */

#ifdef CONFIG_SOC_SERIES_SAMD20
	/*
	 * Errata: silicon revisions B and C do not perform the automatic right
	 * shifts in accumulation
	 */
	if (sequence->oversampling > 4U && DSU->DID.bit.REVISION < 3) {
		adc->AVGCTRL.bit.ADJRES = sequence->oversampling - 4U;
	}
#endif

	switch (sequence->resolution) {
	case 8:
		if (sequence->oversampling) {
			LOG_ERR("Oversampling requires 12 bit resolution");
			return -EINVAL;
		}

		ADC_RESSEL(adc) = ADC_RESSEL_8BIT;
		break;
	case 10:
		if (sequence->oversampling) {
			LOG_ERR("Oversampling requires 12 bit resolution");
			return -EINVAL;
		}

		ADC_RESSEL(adc) = ADC_RESSEL_10BIT;
		break;
	case 12:
		if (sequence->oversampling) {
			ADC_RESSEL(adc) = ADC_RESSEL_16BIT;
		} else {
			ADC_RESSEL(adc) = ADC_RESSEL_12BIT;
		}
		break;
	default:
		LOG_ERR("ADC resolution value %d is not valid",
			sequence->resolution);
		return -EINVAL;
	}

	wait_synchronization(adc);

	if ((sequence->channels == 0)
		|| ((sequence->channels & (sequence->channels - 1)) != 0)) {
		/* The caller is expected to identify a single input channel, which will
		 * typically be the positive input, though no check is made for this...
		 *
		 * While ensuring that the channels bitfield matches the positive input
		 * might be sensible, this will likely break users before this revision
		 * was put in place.
		 */
		LOG_ERR("Channel scanning is not supported");
		return -ENOTSUP;
	}

	error = check_buffer_size(sequence, 1);
	if (error) {
		return error;
	}

	data->buffer = sequence->buffer;
	data->repeat_buffer = sequence->buffer;

	/* At this point we allow the scheduler to do other things while
	 * we wait for the conversions to complete. This is provided by the
	 * adc_context functions. However, the caller of this function is
	 * blocked until the results are in.
	 */
	adc_context_start_read(&data->ctx, sequence);

	error = adc_context_wait_for_completion(&data->ctx);
	return error;
}

static int adc_sam0_read(const struct device *dev,
			 const struct adc_sequence *sequence)
{
	struct adc_sam0_data *data = dev->data;
	int error;

	adc_context_lock(&data->ctx, false, NULL);
	error = start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}

static void adc_sam0_isr(const struct device *dev)
{
	struct adc_sam0_data *data = dev->data;
	const struct adc_sam0_cfg *const cfg = dev->config;
	Adc *const adc = cfg->regs;
	uint16_t result;

	adc->INTFLAG.reg = ADC_INTFLAG_MASK;

	result = (uint16_t)(adc->RESULT.reg);

#ifdef ADC_SAM0_REFERENCE_GLITCH
	if (data->reference_changed) {
		data->reference_changed = 0;
		LOG_DBG("Discarded initial conversion due to reference change");

		adc_sam0_start_conversion(dev);
		return;
	}
#endif

	*data->buffer++ = result;
	adc_context_on_sampling_done(&data->ctx, dev);
}

static int adc_sam0_init(const struct device *dev)
{
	const struct adc_sam0_cfg *const cfg = dev->config;
	struct adc_sam0_data *data = dev->data;
	Adc *const adc = cfg->regs;
	int retval;

#ifdef MCLK
	GCLK->PCHCTRL[cfg->gclk_id].reg = cfg->gclk_mask | GCLK_PCHCTRL_CHEN;

	MCLK->APBDMASK.reg |= cfg->mclk_mask;
#else
	PM->APBCMASK.bit.ADC_ = 1;

	GCLK->CLKCTRL.reg = cfg->gclk | GCLK_CLKCTRL_CLKEN;
#endif

	retval = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (retval < 0) {
		return retval;
	}

	ADC_PRESCALER(adc) = cfg->prescaler;
	wait_synchronization(adc);

	adc->INTENCLR.reg = ADC_INTENCLR_MASK;
	adc->INTFLAG.reg = ADC_INTFLAG_MASK;

	cfg->config_func(dev);

	adc->INTENSET.reg = ADC_INTENSET_RESRDY;

	data->dev = dev;
#ifdef ADC_SAM0_REFERENCE_GLITCH
	data->reference_changed = 1;
#endif

	adc->CTRLA.bit.ENABLE = 1;
	wait_synchronization(adc);

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#ifdef CONFIG_ADC_ASYNC
static int adc_sam0_read_async(const struct device *dev,
			       const struct adc_sequence *sequence,
			       struct k_poll_signal *async)
{
	struct adc_sam0_data *data = dev->data;
	int error;

	adc_context_lock(&data->ctx, true, async);
	error = start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}
#endif

static const struct adc_driver_api adc_sam0_api = {
	.channel_setup = adc_sam0_channel_setup,
	.read = adc_sam0_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_sam0_read_async,
#endif
};


#ifdef MCLK

#define ADC_SAM0_CLOCK_CONTROL(n)					\
	.mclk_mask = BIT(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, bit)),	\
	.gclk_mask = UTIL_CAT(GCLK_PCHCTRL_GEN_GCLK,			\
			      DT_INST_PROP(n, gclk)),			\
	.gclk_id = DT_INST_CLOCKS_CELL_BY_NAME(n, gclk, periph_ch),	\
	.prescaler = UTIL_CAT(ADC_CTRLx_PRESCALER_DIV,			\
			      UTIL_CAT(DT_INST_PROP(n, prescaler), _Val)),

#define ADC_SAM0_CONFIGURE(n)						\
do {									\
	const struct adc_sam0_cfg *const cfg = dev->config;		\
	Adc * const adc = cfg->regs;					\
	adc->CALIB.reg = ADC_SAM0_BIASCOMP(n)				\
			 | ADC_SAM0_BIASR2R(n)				\
			 | ADC_SAM0_BIASREFBUF(n);			\
} while (0)

#else

#define ADC_SAM0_CLOCK_CONTROL(n)					\
	.gclk = UTIL_CAT(GCLK_CLKCTRL_GEN_GCLK,	DT_INST_PROP(n, gclk)) |\
			 GCLK_CLKCTRL_ID_ADC,				\
	.prescaler = UTIL_CAT(ADC_CTRLx_PRESCALER_DIV,			\
			      UTIL_CAT(DT_INST_PROP(n, prescaler), _Val)),

#define ADC_SAM0_CONFIGURE(n)						\
do {									\
	const struct adc_sam0_cfg *const cfg = dev->config;		\
	Adc * const adc = cfg->regs;					\
	/* Linearity is split across two words */			\
	uint32_t lin = ((*(uint32_t *)ADC_FUSES_LINEARITY_0_ADDR) &		\
		     ADC_FUSES_LINEARITY_0_Msk) >>			\
		     ADC_FUSES_LINEARITY_0_Pos;				\
	lin |= (((*(uint32_t *)ADC_FUSES_LINEARITY_1_ADDR) &		\
		 ADC_FUSES_LINEARITY_1_Msk) >>				\
		 ADC_FUSES_LINEARITY_1_Pos) << 4;			\
	uint32_t bias = ((*(uint32_t *)ADC_FUSES_BIASCAL_ADDR) &		\
		      ADC_FUSES_BIASCAL_Msk) >> ADC_FUSES_BIASCAL_Pos;	\
	adc->CALIB.reg = ADC_CALIB_BIAS_CAL(bias) |			\
			 ADC_CALIB_LINEARITY_CAL(lin);			\
} while (0)

#endif

#define ADC_SAM0_DEVICE(n)						\
	PINCTRL_DT_INST_DEFINE(n);					\
	static void adc_sam0_config_##n(const struct device *dev);	\
	static const struct adc_sam0_cfg adc_sam_cfg_##n = {		\
		.regs = (Adc *)DT_INST_REG_ADDR(n),			\
		ADC_SAM0_CLOCK_CONTROL(n)				\
		.freq = UTIL_CAT(UTIL_CAT(SOC_ATMEL_SAM0_GCLK,		\
					  DT_INST_PROP(n, gclk)),	\
					  _FREQ_HZ) /			\
			DT_INST_PROP(n, prescaler),			\
		.config_func = &adc_sam0_config_##n,			\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
	};								\
	static struct adc_sam0_data adc_sam_data_##n = {		\
		ADC_CONTEXT_INIT_TIMER(adc_sam_data_##n, ctx),		\
		ADC_CONTEXT_INIT_LOCK(adc_sam_data_##n, ctx),		\
		ADC_CONTEXT_INIT_SYNC(adc_sam_data_##n, ctx),		\
	};								\
	DEVICE_DT_INST_DEFINE(n, adc_sam0_init, NULL,			\
			    &adc_sam_data_##n,				\
			    &adc_sam_cfg_##n, POST_KERNEL,		\
			    CONFIG_ADC_INIT_PRIORITY,			\
			    &adc_sam0_api);				\
	static void adc_sam0_config_##n(const struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, resrdy, irq),	\
			    DT_INST_IRQ_BY_NAME(n, resrdy, priority),	\
			    adc_sam0_isr,				\
			    DEVICE_DT_INST_GET(n), 0);			\
		irq_enable(DT_INST_IRQ_BY_NAME(n, resrdy, irq));	\
		ADC_SAM0_CONFIGURE(n);					\
	}

DT_INST_FOREACH_STATUS_OKAY(ADC_SAM0_DEVICE)
