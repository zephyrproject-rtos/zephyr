/*
 * Copyright (c) 2019 Derek Hageman <hageman@inthat.cloud>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <soc.h>
#include <drivers/adc.h>

#include <logging/log.h>
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
	struct device *dev;

	u16_t *buffer;

	/*
	 * Saved initial start, so we can reset the advances we've done
	 * if required
	 */
	u16_t *repeat_buffer;

#ifdef ADC_SAM0_REFERENCE_GLITCH
	u8_t reference_changed;
#endif
};

struct adc_sam0_cfg {
	Adc *regs;

#ifdef MCLK
	u32_t mclk_mask;
	u32_t gclk_mask;
	u16_t gclk_id;
#else
	u32_t gclk;
#endif

	u32_t freq;
	u16_t prescaler;

	void (*config_func)(struct device *dev);
};

#define DEV_CFG(dev) \
	((const struct adc_sam0_cfg *const)(dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct adc_sam0_data *)(dev)->driver_data)

static void wait_synchronization(Adc *const adc)
{
#if defined(ADC_SYNCBUSY_MASK)
	while ((adc->SYNCBUSY.reg & ADC_SYNCBUSY_MASK) != 0) {
	}
#else
	while ((adc->STATUS.reg & ADC_STATUS_SYNCBUSY) != 0) {
	}
#endif
}

static int adc_sam0_acquisition_to_clocks(struct device *dev,
					  u16_t acquisition_time)
{
	const struct adc_sam0_cfg *const cfg = DEV_CFG(dev);
	u64_t scaled_acq;

	switch (ADC_ACQ_TIME_UNIT(acquisition_time)) {
	case ADC_ACQ_TIME_TICKS:
		if (ADC_ACQ_TIME_VALUE(acquisition_time) > 64U) {
			return -EINVAL;
		}

		return (int)ADC_ACQ_TIME_VALUE(acquisition_time) - 1;
	case ADC_ACQ_TIME_MICROSECONDS:
		scaled_acq = (u64_t)ADC_ACQ_TIME_VALUE(acquisition_time) *
			     1000000U;
		break;
	case ADC_ACQ_TIME_NANOSECONDS:
		scaled_acq = (u64_t)ADC_ACQ_TIME_VALUE(acquisition_time) *
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

static int adc_sam0_channel_setup(struct device *dev,
				  const struct adc_channel_cfg *channel_cfg)
{
	const struct adc_sam0_cfg *const cfg = DEV_CFG(dev);
	Adc *const adc = cfg->regs;
	int retval;
	u8_t SAMPCTRL = 0;

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		retval = adc_sam0_acquisition_to_clocks(dev,
							channel_cfg->acquisition_time);
		if (retval < 0) {
			LOG_ERR("Selected ADC acquisition time is not valid");
			return retval;
		}

		SAMPCTRL |= ADC_SAMPCTRL_SAMPLEN(retval);
	}

	adc->SAMPCTRL.reg = SAMPCTRL;
	wait_synchronization(adc);


	u8_t REFCTRL;

	switch (channel_cfg->reference) {
	case ADC_REF_INTERNAL:
#ifdef ADC_REFCTRL_REFSEL_INTREF
		REFCTRL = ADC_REFCTRL_REFSEL_INTREF | ADC_REFCTRL_REFCOMP;
		/* Enable the internal reference, defaulting to 1V */
		SUPC->VREF.bit.VREFOE = 1;
#else
		REFCTRL = ADC_REFCTRL_REFSEL_INT1V | ADC_REFCTRL_REFCOMP;
		/* Enable the internal bandgap reference */
		SYSCTRL->VREF.bit.BGOUTEN = 1;
#endif
		break;
	case ADC_REF_VDD_1_2:
#ifdef ADC_REFCTRL_REFSEL_INTVCC0
		REFCTRL = ADC_REFCTRL_REFSEL_INTVCC0 | ADC_REFCTRL_REFCOMP;
#else
		REFCTRL = ADC_REFCTRL_REFSEL_INTVCC1 | ADC_REFCTRL_REFCOMP;
#endif
		break;
#ifdef ADC_REFCTRL_REFSEL_INTVCC1
	case ADC_REF_VDD_1:
		REFCTRL = ADC_REFCTRL_REFSEL_INTVCC1 | ADC_REFCTRL_REFCOMP;
		break;
#endif
	case ADC_REF_EXTERNAL0:
		REFCTRL = ADC_REFCTRL_REFSEL_AREFA;
		break;
	case ADC_REF_EXTERNAL1:
		REFCTRL = ADC_REFCTRL_REFSEL_AREFB;
		break;
	default:
		LOG_ERR("Selected reference is not valid");
		return -EINVAL;
	}
	if (adc->REFCTRL.reg != REFCTRL) {
		adc->REFCTRL.reg = REFCTRL;
		wait_synchronization(adc);
#ifdef ADC_SAM0_REFERENCE_GLITCH
		struct adc_sam0_data *data = DEV_DATA(dev);

		data->reference_changed = 1;
#endif
	}


	u32_t INPUTCTRL = 0;

	switch (channel_cfg->gain) {
	case ADC_GAIN_1:
#ifdef ADC_INPUTCTRL_GAIN_1X
		INPUTCTRL = ADC_INPUTCTRL_GAIN_1X;
#endif
		break;
#ifdef ADC_INPUTCTRL_GAIN_DIV2
	case ADC_GAIN_1_2:
		INPUTCTRL = ADC_INPUTCTRL_GAIN_DIV2;
		break;
#endif
#ifdef ADC_INPUTCTRL_GAIN_2X
	case ADC_GAIN_2:
		INPUTCTRL = ADC_INPUTCTRL_GAIN_2X;
		break;
#endif
#ifdef ADC_INPUTCTRL_GAIN_4X
	case ADC_GAIN_4:
		INPUTCTRL = ADC_INPUTCTRL_GAIN_4X;
		break;
#endif
#ifdef ADC_INPUTCTRL_GAIN_8X
	case ADC_GAIN_8:
		INPUTCTRL = ADC_INPUTCTRL_GAIN_8X;
		break;
#endif
#ifdef ADC_INPUTCTRL_GAIN_16X
	case ADC_GAIN_16:
		INPUTCTRL = ADC_INPUTCTRL_GAIN_16X;
		break;
#endif
	default:
		LOG_ERR("Selected ADC gain is not valid");
		return -EINVAL;
	}

	INPUTCTRL |= ADC_INPUTCTRL_MUXPOS(channel_cfg->input_positive);
	if (channel_cfg->differential) {
		INPUTCTRL |= ADC_INPUTCTRL_MUXNEG(channel_cfg->input_negative);

#ifdef ADC_INPUTCTRL_DIFFMODE
		INPUTCTRL |= ADC_INPUTCTRL_DIFFMODE;
#else
		adc->CTRLB.bit.DIFFMODE = 1;
		wait_synchronization(adc);
#endif
	} else {
		INPUTCTRL |= ADC_INPUTCTRL_MUXNEG_GND;

#ifndef ADC_INPUTCTRL_DIFFMODE
		adc->CTRLB.bit.DIFFMODE = 0;
		wait_synchronization(adc);
#endif
	}

	adc->INPUTCTRL.reg = INPUTCTRL;
	wait_synchronization(adc);

	/* Enable references if they're selected */
	switch (channel_cfg->input_positive) {
#ifdef ADC_INPUTCTRL_MUXPOS_TEMP_Val
	case ADC_INPUTCTRL_MUXPOS_TEMP_Val:
		SYSCTRL->VREF.bit.TSEN = 1;
		break;
#endif
#ifdef ADC_INPUTCTRL_MUXPOS_PTAT_Val
	case ADC_INPUTCTRL_MUXPOS_PTAT_Val:
		SUPC->VREF.bit.TSEN = 1;
		break;
#endif
#ifdef ADC_INPUTCTRL_MUXPOS_CTAT_Val
	case ADC_INPUTCTRL_MUXPOS_CTAT_Val:
		SUPC->VREF.bit.TSEN = 1;
		break;
#endif
	case ADC_INPUTCTRL_MUXPOS_BANDGAP_Val:
#ifdef ADC_REFCTRL_REFSEL_INTREF
		SUPC->VREF.bit.VREFOE = 1;
#else
		SYSCTRL->VREF.bit.BGOUTEN = 1;
#endif
		break;
	default:
		break;
	}


	return 0;
}

static void adc_sam0_start_conversion(struct device *dev)
{
	const struct adc_sam0_cfg *const cfg = DEV_CFG(dev);
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
			     u8_t active_channels)
{
	size_t needed_buffer_size;

	needed_buffer_size = active_channels * sizeof(u16_t);
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

static int start_read(struct device *dev, const struct adc_sequence *sequence)
{
	const struct adc_sam0_cfg *const cfg = DEV_CFG(dev);
	struct adc_sam0_data *data = DEV_DATA(dev);
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

		adc->CTRLB.bit.RESSEL = ADC_CTRLB_RESSEL_8BIT_Val;
		break;
	case 10:
		if (sequence->oversampling) {
			LOG_ERR("Oversampling requires 12 bit resolution");
			return -EINVAL;
		}

		adc->CTRLB.bit.RESSEL = ADC_CTRLB_RESSEL_10BIT_Val;
		break;
	case 12:
		if (sequence->oversampling) {
			adc->CTRLB.bit.RESSEL = ADC_CTRLB_RESSEL_16BIT_Val;
		} else {
			adc->CTRLB.bit.RESSEL = ADC_CTRLB_RESSEL_12BIT_Val;
		}

		break;
	default:
		LOG_ERR("ADC resolution value %d is not valid",
			sequence->resolution);
		return -EINVAL;
	}

	wait_synchronization(adc);

	if (sequence->channels != 1U) {
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

static int adc_sam0_read(struct device *dev,
			 const struct adc_sequence *sequence)
{
	struct adc_sam0_data *data = DEV_DATA(dev);
	int error;

	adc_context_lock(&data->ctx, false, NULL);
	error = start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}

static void adc_sam0_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct adc_sam0_data *data = DEV_DATA(dev);
	const struct adc_sam0_cfg *const cfg = DEV_CFG(dev);
	Adc *const adc = cfg->regs;
	u16_t result;

	adc->INTFLAG.reg = ADC_INTFLAG_MASK;

	result = (u16_t)(adc->RESULT.reg);

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

static int adc_sam0_init(struct device *dev)
{
	const struct adc_sam0_cfg *const cfg = DEV_CFG(dev);
	struct adc_sam0_data *data = DEV_DATA(dev);
	Adc *const adc = cfg->regs;

#ifdef MCLK
	GCLK->PCHCTRL[cfg->gclk_id].reg = cfg->gclk_mask | GCLK_PCHCTRL_CHEN;

	MCLK->APBDMASK.reg |= cfg->mclk_mask;
#else
	PM->APBCMASK.bit.ADC_ = 1;

	GCLK->CLKCTRL.reg = cfg->gclk | GCLK_CLKCTRL_CLKEN;
#endif

#ifdef ADC_CTRLA_PRESCALER_Pos
	adc->CTRLA.reg = cfg->prescaler;
#else
	adc->CTRLB.reg = cfg->prescaler;
#endif
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
static int adc_sam0_read_async(struct device *dev,
			       const struct adc_sequence *sequence,
			       struct k_poll_signal *async)
{
	struct adc_sam0_data *data = DEV_DATA(dev);
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

#define ADC_SAM0_CLOCK_CONTROL(n)					      \
	.mclk_mask = MCLK_APBDMASK_ADC##n,				      \
	.gclk_mask = UTIL_CAT(GCLK_PCHCTRL_GEN_GCLK,			      \
			      DT_ATMEL_SAM0_ADC_ADC_##n##_GCLK),	      \
	.gclk_id = ADC##n##_GCLK_ID,					      \
	.prescaler = UTIL_CAT(ADC_CTRLA_PRESCALER_DIV,			      \
			      DT_ATMEL_SAM0_ADC_ADC_##n##_PRESCALER),
#define ADC_SAM0_CONFIGURE(n) do {					      \
		const struct adc_sam0_cfg *const cfg = DEV_CFG(dev);	      \
		Adc *const adc = cfg->regs;				      \
		u32_t comp = ((*(u32_t *)ADC##n##_FUSES_BIASCOMP_ADDR) &      \
			      ADC##n##_FUSES_BIASCOMP_Msk) >>		      \
			     ADC##n##_FUSES_BIASCOMP_Pos;		      \
		u32_t r2r = ((*(u32_t *)ADC##n##_FUSES_BIASR2R_ADDR) &	      \
			     ADC##n##_FUSES_BIASR2R_Msk) >>		      \
			    ADC##n##_FUSES_BIASR2R_Pos;			      \
		u32_t rbuf = ((*(u32_t *)ADC##n##_FUSES_BIASREFBUF_ADDR) &    \
			      ADC##n##_FUSES_BIASREFBUF_Msk) >>		      \
			     ADC##n##_FUSES_BIASREFBUF_Pos;		      \
		adc->CALIB.reg = ADC_CALIB_BIASCOMP(comp) |		      \
				 ADC_CALIB_BIASR2R(r2r) |		      \
				 ADC_CALIB_BIASREFBUF(rbuf);		      \
} while (0)

#else

#define ADC_SAM0_CLOCK_CONTROL(n)					      \
	.gclk = UTIL_CAT(GCLK_CLKCTRL_GEN_GCLK,				      \
			 DT_ATMEL_SAM0_ADC_ADC_##n##_GCLK) |		      \
			 GCLK_CLKCTRL_ID_ADC,				      \
	.prescaler = UTIL_CAT(ADC_CTRLB_PRESCALER_DIV,			      \
			      DT_ATMEL_SAM0_ADC_ADC_##n##_PRESCALER),
#define ADC_SAM0_CONFIGURE(n) do {					      \
		const struct adc_sam0_cfg *const cfg = DEV_CFG(dev);	      \
		Adc *const adc = cfg->regs;				      \
		/* Linearity is split across two words */		      \
		u32_t lin = ((*(u32_t *)ADC_FUSES_LINEARITY_0_ADDR) &	      \
			     ADC_FUSES_LINEARITY_0_Msk) >>		      \
			     ADC_FUSES_LINEARITY_0_Pos;			      \
		lin |= (((*(u32_t *)ADC_FUSES_LINEARITY_1_ADDR) &	      \
			 ADC_FUSES_LINEARITY_1_Msk) >>			      \
			 ADC_FUSES_LINEARITY_1_Pos)			      \
			<< 4;						      \
		u32_t bias = ((*(u32_t *)ADC_FUSES_BIASCAL_ADDR) &	      \
			      ADC_FUSES_BIASCAL_Msk) >> ADC_FUSES_BIASCAL_Pos;\
		adc->CALIB.reg = ADC_CALIB_BIAS_CAL(bias) |		      \
				 ADC_CALIB_LINEARITY_CAL(lin);		      \
} while (0)

#endif


#define ADC_SAM0_DEVICE(n)						      \
	static void adc_sam0_config_##n(struct device *dev);		      \
	static const struct adc_sam0_cfg adc_sam_cfg_##n = {		      \
		.regs = (Adc *)DT_ATMEL_SAM0_ADC_ADC_##n##_BASE_ADDRESS,      \
		ADC_SAM0_CLOCK_CONTROL(n)				      \
		.freq = UTIL_CAT(UTIL_CAT(SOC_ATMEL_SAM0_GCLK,		      \
					  DT_ATMEL_SAM0_ADC_ADC_##n##_GCLK),  \
					  _FREQ_HZ) /			      \
			DT_ATMEL_SAM0_ADC_ADC_##n##_PRESCALER,		      \
		.config_func = &adc_sam0_config_##n,			      \
	};								      \
	static struct adc_sam0_data adc_sam_data_##n = {		      \
		ADC_CONTEXT_INIT_TIMER(adc_sam_data_##n, ctx),		      \
		ADC_CONTEXT_INIT_LOCK(adc_sam_data_##n, ctx),		      \
		ADC_CONTEXT_INIT_SYNC(adc_sam_data_##n, ctx),		      \
	};								      \
	DEVICE_AND_API_INIT(adc0_sam_##n, DT_ATMEL_SAM0_ADC_ADC_##n##_LABEL,  \
			    adc_sam0_init, &adc_sam_data_##n, &adc_sam_cfg_##n,\
			    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,  \
			    &adc_sam0_api);				      \
	static void adc_sam0_config_##n(struct device *dev)		      \
	{								      \
		IRQ_CONNECT(DT_ATMEL_SAM0_ADC_ADC_##n##_IRQ_0,		      \
			    DT_ATMEL_SAM0_ADC_ADC_##n##_IRQ_0_PRIORITY,	      \
			    adc_sam0_isr,				      \
			    DEVICE_GET(adc0_sam_##n),			      \
			    0);						      \
		irq_enable(DT_ATMEL_SAM0_ADC_ADC_##n##_IRQ_0);		      \
		ADC_SAM0_CONFIGURE(n);					      \
	}

#if DT_ATMEL_SAM0_ADC_ADC_0_BASE_ADDRESS
ADC_SAM0_DEVICE(0);
#endif

#if DT_ATMEL_SAM0_ADC_ADC_1_BASE_ADDRESS
ADC_SAM0_DEVICE(1);
#endif
