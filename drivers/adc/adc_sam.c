/*
 * Copyright (c) 2022, Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_adc

#include <soc.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(adc_sam, CONFIG_ADC_LOG_LEVEL);

#define SAM_ADC_NUM_CHANNELS 16
#define SAM_ADC_TEMP_CHANNEL 15

struct adc_sam_config {
	Adc *regs;
	const struct atmel_sam_pmc_config clock_cfg;

	uint8_t prescaler;
	uint8_t startup_time;
	uint8_t settling_time;
	uint8_t tracking_time;

	const struct pinctrl_dev_config *pcfg;
	void (*config_func)(const struct device *dev);
};

struct adc_sam_data {
	struct adc_context ctx;
	const struct device *dev;

	/* Pointer to the buffer in the sequence. */
	uint16_t *buffer;

	/* Pointer to the beginning of a sample. Consider the number of
	 * channels in the sequence: this buffer changes by that amount
	 * so all the channels would get repeated.
	 */
	uint16_t *repeat_buffer;

	/* Number of active channels to fill buffer */
	uint8_t num_active_channels;
};

static uint8_t count_bits(uint32_t val)
{
	uint8_t res = 0;

	while (val) {
		res += val & 1U;
		val >>= 1;
	}

	return res;
}

static int adc_sam_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	const struct adc_sam_config *const cfg = dev->config;
	Adc *const adc = cfg->regs;

	uint8_t channel_id = channel_cfg->channel_id;

	if (channel_cfg->differential) {
		if (channel_id != (channel_cfg->input_positive / 2U)
		    || channel_id != (channel_cfg->input_negative / 2U)) {
			LOG_ERR("Invalid ADC differential input for channel %u", channel_id);
			return -EINVAL;
		}
	} else {
		if (channel_id != channel_cfg->input_positive) {
			LOG_ERR("Invalid ADC single-ended input for channel %u", channel_id);
			return -EINVAL;
		}
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Invalid ADC channel acquisition time");
		return -EINVAL;
	}

	if (channel_cfg->reference != ADC_REF_EXTERNAL0) {
		LOG_ERR("Invalid ADC channel reference (%d)", channel_cfg->reference);
		return -EINVAL;
	}

	/* Enable internal temperature sensor (channel 15 / single-ended) */
	if (channel_cfg->channel_id == SAM_ADC_TEMP_CHANNEL) {
		adc->ADC_ACR |= ADC_ACR_TSON;
	}

	/* Set channel mode, always on both inputs */
	if (channel_cfg->differential) {
		adc->ADC_COR |= (ADC_COR_DIFF0 | ADC_COR_DIFF1) << (channel_id * 2U);
	} else {
		adc->ADC_COR &= ~((ADC_COR_DIFF0 | ADC_COR_DIFF1) << (channel_id * 2U));
	}

	/* Reset current gain */
	adc->ADC_CGR &= ~(ADC_CGR_GAIN0_Msk << (channel_id * 2U));

	switch (channel_cfg->gain) {
	case ADC_GAIN_1_2:
		if (!channel_cfg->differential) {
			LOG_ERR("ADC 1/2x gain only allowed for differential channel");
			return -EINVAL;
		}
		/* NOP */
		break;
	case ADC_GAIN_1:
		adc->ADC_CGR |= ADC_CGR_GAIN0(1) << (channel_id * 2U);
		break;
	case ADC_GAIN_2:
		adc->ADC_CGR |= ADC_CGR_GAIN0(2) << (channel_id * 2U);
		break;
	case ADC_GAIN_4:
		if (channel_cfg->differential) {
			LOG_ERR("ADC 4x gain only allowed for single-ended channel");
			return -EINVAL;
		}
		adc->ADC_CGR |= ADC_CGR_GAIN0(3) << (channel_id * 2U);
		break;
	default:
		LOG_ERR("Invalid ADC channel gain (%d)", channel_cfg->gain);
		return -EINVAL;
	}

	return 0;
}

static void adc_sam_start_conversion(const struct device *dev)
{
	const struct adc_sam_config *const cfg = dev->config;
	Adc *const adc = cfg->regs;

	adc->ADC_CR = ADC_CR_START;
}

/**
 * This is only called once at the beginning of all the conversions,
 * all channels as a group.
 */
static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_sam_data *data = CONTAINER_OF(ctx, struct adc_sam_data, ctx);
	const struct adc_sam_config *const cfg = data->dev->config;
	Adc *const adc = cfg->regs;

	data->num_active_channels = count_bits(ctx->sequence.channels);

	/* Disable all */
	adc->ADC_CHDR = 0xffff;

	/* Enable selected */
	adc->ADC_CHER = ctx->sequence.channels;

	LOG_DBG("Starting conversion for %u channels", data->num_active_channels);

	adc_sam_start_conversion(data->dev);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat)
{
	struct adc_sam_data *data = CONTAINER_OF(ctx, struct adc_sam_data, ctx);

	if (repeat) {
		data->buffer = data->repeat_buffer;
	}
}

static int check_buffer_size(const struct adc_sequence *sequence,
			     uint8_t active_channels)
{
	size_t needed_buffer_size = active_channels * sizeof(uint16_t);

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

static int start_read(const struct device *dev,
		      const struct adc_sequence *sequence)
{
	struct adc_sam_data *data = dev->data;
	uint32_t channels = sequence->channels;
	int error;

	/* Signal an error if the channel selection is invalid (no channels or
	 * a non-existing one is selected).
	 */
	if (channels == 0U ||
	    (channels & (~0UL << SAM_ADC_NUM_CHANNELS))) {
		LOG_ERR("Invalid selection of channels");
		return -EINVAL;
	}

	if (sequence->oversampling != 0U) {
		LOG_ERR("Oversampling is not supported");
		return -EINVAL;
	}

	if (sequence->resolution != 12U) {
		LOG_ERR("ADC resolution %d is not valid", sequence->resolution);
		return -EINVAL;
	}

	data->num_active_channels = count_bits(channels);

	error = check_buffer_size(sequence, data->num_active_channels);
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

	return adc_context_wait_for_completion(&data->ctx);
}

static int adc_sam_read(const struct device *dev,
			const struct adc_sequence *sequence)
{
	struct adc_sam_data *data = dev->data;
	int error;

	adc_context_lock(&data->ctx, false, NULL);
	error = start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}

static void adc_sam_isr(const struct device *dev)
{
	const struct adc_sam_config *const cfg = dev->config;
	struct adc_sam_data *data = dev->data;
	Adc *const adc = cfg->regs;

	uint16_t result;

	if (adc->ADC_ISR & ADC_ISR_DRDY) {
		result = adc->ADC_LCDR & ADC_LCDR_LDATA_Msk;

		*data->buffer++ = result;
		data->num_active_channels--;

		if (data->num_active_channels == 0) {
			/* Called once all conversions have completed.*/
			adc_context_on_sampling_done(&data->ctx, dev);
		} else {
			adc_sam_start_conversion(dev);
		}
	}
}

static int adc_sam_init(const struct device *dev)
{
	const struct adc_sam_config *const cfg = dev->config;
	struct adc_sam_data *data = dev->data;
	Adc *const adc = cfg->regs;

	int ret;
	uint32_t frequency, conv_periods;

	/* Get peripheral clock frequency */
	ret = clock_control_get_rate(SAM_DT_PMC_CONTROLLER,
				     (clock_control_subsys_t)&cfg->clock_cfg,
				     &frequency);
	if (ret < 0) {
		LOG_ERR("Failed to get ADC peripheral clock rate (%d)", ret);
		return -ENODEV;
	}

	/* Calculate ADC clock frequency */
	frequency = frequency / 2U / (cfg->prescaler + 1U);
	if (frequency < 1000000U || frequency > 22000000U) {
		LOG_ERR("Invalid ADC clock frequency %d (1MHz < freq < 22Mhz)", frequency);
		return -EINVAL;
	}

	/* The number of ADC pulses for conversion  */
	conv_periods = MAX(20U, cfg->tracking_time + 6U);

	/* Calculate the sampling frequency */
	frequency /= conv_periods;

	/* Reset ADC controller */
	adc->ADC_CR = ADC_CR_SWRST;

	/* Reset Mode */
	adc->ADC_MR = 0U;

	/* Reset PDC transfer */
	adc->ADC_PTCR = ADC_PTCR_RXTDIS | ADC_PTCR_TXTDIS;
	adc->ADC_RCR = 0U;
	adc->ADC_RNCR = 0U;

	/* Set prescaler, timings and allow different analog settings for each channel */
	adc->ADC_MR = ADC_MR_PRESCAL(cfg->prescaler)
		    | ADC_MR_STARTUP(cfg->startup_time)
		    | ADC_MR_SETTLING(cfg->settling_time)
		    | ADC_MR_TRACKTIM(cfg->tracking_time)
		    | ADC_MR_TRANSFER(2U) /* Should be 2 to guarantee the optimal hold time. */
		    | ADC_MR_ANACH_ALLOWED;

	/**
	 * Set bias current control
	 * IBCTL = 00 is the required value for a sampling frequency below 500 kHz,
	 * and IBCTL = 01 for a sampling frequency between 500 kHz and 1 MHz.
	 */
	adc->ADC_ACR = ADC_ACR_IBCTL(frequency < 500000U ? 0U : 1U);

	/* Enable ADC clock in PMC */
	ret = clock_control_on(SAM_DT_PMC_CONTROLLER,
			       (clock_control_subsys_t)&cfg->clock_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to enable ADC clock (%d)", ret);
		return -ENODEV;
	}

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	cfg->config_func(dev);

	/* Enable data ready interrupt */
	adc->ADC_IER = ADC_IER_DRDY;

	data->dev = dev;

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#ifdef CONFIG_ADC_ASYNC
static int adc_sam_read_async(const struct device *dev,
			      const struct adc_sequence *sequence,
			      struct k_poll_signal *async)
{
	struct adc_sam_data *data = dev->data;
	int error;

	adc_context_lock(&data->ctx, true, async);
	error = start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}
#endif

static DEVICE_API(adc, adc_sam_api) = {
	.channel_setup = adc_sam_channel_setup,
	.read = adc_sam_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_sam_read_async,
#endif
};

#define ADC_SAM_DEVICE(n)						\
	PINCTRL_DT_INST_DEFINE(n);					\
	static void adc_sam_irq_config_##n(const struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    adc_sam_isr,				\
			    DEVICE_DT_INST_GET(n), 0);			\
		irq_enable(DT_INST_IRQN(n));				\
	}								\
	static const struct adc_sam_config adc_sam_config_##n = {	\
		.regs = (Adc *)DT_INST_REG_ADDR(n),			\
		.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(n),		\
		.prescaler = DT_INST_PROP(n, prescaler),		\
		.startup_time = DT_INST_ENUM_IDX(n, startup_time),	\
		.settling_time = DT_INST_ENUM_IDX(n, settling_time),	\
		.tracking_time = DT_INST_ENUM_IDX(n, tracking_time),	\
		.config_func = &adc_sam_irq_config_##n,			\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
	};								\
	static struct adc_sam_data adc_sam_data_##n = {			\
		ADC_CONTEXT_INIT_TIMER(adc_sam_data_##n, ctx),		\
		ADC_CONTEXT_INIT_LOCK(adc_sam_data_##n, ctx),		\
		ADC_CONTEXT_INIT_SYNC(adc_sam_data_##n, ctx),		\
		.dev = DEVICE_DT_INST_GET(n),				\
	};								\
	DEVICE_DT_INST_DEFINE(n, adc_sam_init, NULL,			\
			      &adc_sam_data_##n,			\
			      &adc_sam_config_##n, POST_KERNEL,		\
			      CONFIG_ADC_INIT_PRIORITY,			\
			      &adc_sam_api);

DT_INST_FOREACH_STATUS_OKAY(ADC_SAM_DEVICE)
