/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_gecko_adc

#include <zephyr/drivers/adc.h>

#include <em_adc.h>
#include <em_cmu.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_gecko, CONFIG_ADC_LOG_LEVEL);

/* Number of channels available. */
#define GECKO_CHANNEL_COUNT	16

struct adc_gecko_channel_config {
	bool initialized;
	ADC_Ref_TypeDef reference;
	ADC_PosSel_TypeDef input_select;
};

struct adc_gecko_data {
	const struct device *dev;
	struct adc_context ctx;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint32_t channels;
	uint8_t channel_id;
	ADC_Res_TypeDef resolution;
	struct adc_gecko_channel_config channel_config[GECKO_CHANNEL_COUNT];
};

struct adc_gecko_config {
	ADC_TypeDef *base;
	void (*irq_cfg_func)(void);
	uint32_t frequency;
};

static void adc_gecko_set_config(const struct device *dev)
{
	struct adc_gecko_data *data = dev->data;
	struct adc_gecko_channel_config *channel_config = NULL;
	const struct adc_gecko_config *config = dev->config;
	ADC_TypeDef *adc_base = (ADC_TypeDef *)config->base;

	ADC_Init_TypeDef init = ADC_INIT_DEFAULT;
	ADC_InitSingle_TypeDef initSingle = ADC_INITSINGLE_DEFAULT;

	channel_config = &data->channel_config[data->channel_id];

	init.prescale = ADC_PrescaleCalc(config->frequency, 0);
	init.timebase = ADC_TimebaseCalc(0);

	initSingle.diff = false;
	initSingle.reference = channel_config->reference;
	initSingle.resolution = data->resolution;
	initSingle.acqTime = adcAcqTime4;

	initSingle.posSel = channel_config->input_select;

	ADC_Init(adc_base, &init);
	ADC_InitSingle(adc_base, &initSingle);
}

static int adc_gecko_check_buffer_size(const struct adc_sequence *sequence,
					uint8_t active_channels)
{
	size_t needed_buffer_size;

	needed_buffer_size = active_channels * sizeof(uint16_t);

	if (sequence->options) {
		needed_buffer_size *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed_buffer_size) {
		LOG_DBG("Provided buffer is too small (%u/%u)",
			sequence->buffer_size, needed_buffer_size);
		return -ENOMEM;
	}

	return 0;
}

static int start_read(const struct device *dev, const struct adc_sequence *sequence)
{

	struct adc_gecko_data *data = dev->data;
	uint32_t channels;
	uint8_t channel_count;
	uint8_t index;
	int res;

	/* Check if at least 1 channel is requested */
	if (sequence->channels == 0) {
		LOG_DBG("No channel requested");
		return -EINVAL;
	}

	if (sequence->oversampling) {
		LOG_ERR("Oversampling is not supported");
		return -ENOTSUP;
	}

	/* Verify all requested channels are initialized and store resolution */
	channels = sequence->channels;
	channel_count = 0;
	while (channels) {
		/* Iterate through all channels and check if they are initialized */
		index = find_lsb_set(channels) - 1;
		if (index >= GECKO_CHANNEL_COUNT) {
			LOG_DBG("Requested channel index not available: %d", index);
			return -EINVAL;
		}

		if (!data->channel_config[index].initialized) {
			LOG_DBG("Channel not initialized");
			return -EINVAL;
		}
		channel_count++;
		channels &= ~BIT(index);
	}

	res = adc_gecko_check_buffer_size(sequence, channel_count);
	if (res < 0) {
		return res;
	}

	data->buffer = sequence->buffer;

	adc_context_start_read(&data->ctx, sequence);

	res = adc_context_wait_for_completion(&data->ctx);

	return res;
}

static void adc_gecko_start_channel(const struct device *dev)
{
	const struct adc_gecko_config *config = dev->config;
	struct adc_gecko_data *data = dev->data;
	ADC_TypeDef *adc_base = (ADC_TypeDef *)config->base;

	data->channel_id = find_lsb_set(data->channels) - 1;
	adc_gecko_set_config(data->dev);

	ADC_IntEnable(adc_base, ADC_IEN_SINGLE);
	ADC_Start(adc_base, adcStartSingle);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_gecko_data *data = CONTAINER_OF(ctx, struct adc_gecko_data, ctx);

	data->channels = ctx->sequence.channels;
	adc_gecko_start_channel(data->dev);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct adc_gecko_data *data = CONTAINER_OF(ctx, struct adc_gecko_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static void adc_gecko_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	const struct adc_gecko_config *config = dev->config;
	struct adc_gecko_data *data = dev->data;
	ADC_TypeDef *adc_base = config->base;

	uint32_t sample = 0;
	uint32_t flags, err;

	flags = ADC_IntGet(adc_base);

	__ASSERT(flags & ADC_IF_SINGLE, "unexpected ADC IRQ (flags=0x%08x)!", flags);

	err = flags & (ADC_IF_EM23ERR | ADC_IF_PROGERR | ADC_IF_VREFOV | ADC_IF_SINGLEOF);

	if (!err) {
		sample = ADC_DataSingleGet(adc_base);
		*data->buffer++ = (uint16_t)sample;
		data->channels &= ~BIT(data->channel_id);

		if (data->channels) {
			adc_gecko_start_channel(dev);
		} else {
			adc_context_on_sampling_done(&data->ctx, dev);
		}
	} else {
		LOG_ERR("ADC conversion error, flags=%08x", err);
		adc_context_complete(&data->ctx, -EIO);
	}
	ADC_IntClear(adc_base, ADC_IF_SINGLE | err);
}

static int adc_gecko_read(const struct device *dev,
			  const struct adc_sequence *sequence)
{
	struct adc_gecko_data *data = dev->data;
	int error;

	adc_context_lock(&data->ctx, false, NULL);
	error = start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}

static int adc_gecko_channel_setup(const struct device *dev,
				const struct adc_channel_cfg *channel_cfg)
{
	struct adc_gecko_data *data = dev->data;
	struct adc_gecko_channel_config *channel_config = NULL;

	if (channel_cfg->channel_id < GECKO_CHANNEL_COUNT) {
		channel_config = &data->channel_config[channel_cfg->channel_id];
	} else {
		LOG_DBG("Requested channel index not available: %d", channel_cfg->channel_id);
		return -EINVAL;
	}

	channel_config->initialized = false;

	channel_config->input_select = channel_cfg->input_positive;

	switch (channel_cfg->gain) {
	case ADC_GAIN_1:
		break;
	default:
		LOG_ERR("unsupported channel gain '%d'", channel_cfg->gain);
		return -ENOTSUP;
	}

	switch (channel_cfg->reference) {
	case ADC_REF_VDD_1:
		channel_config->reference = adcRef5V;
		break;
	case ADC_REF_VDD_1_2:
		channel_config->reference = adcRef2V5;
		break;
	case ADC_REF_VDD_1_4:
		channel_config->reference = adcRef1V25;
		break;
	default:
		LOG_ERR("unsupported channel reference type '%d'", channel_cfg->reference);
		return -ENOTSUP;
	}

	channel_config->initialized = true;
	return 0;
}

static int adc_gecko_init(const struct device *dev)
{
	const struct adc_gecko_config *config = dev->config;
	struct adc_gecko_data *data = dev->data;

	CMU_ClockEnable(cmuClock_HFPER, true);
	CMU_ClockEnable(cmuClock_ADC0, true);

	data->dev = dev;
	data->resolution = adcRes12Bit;

	config->irq_cfg_func();

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(adc, api_gecko_adc_driver_api) = {
	.channel_setup = adc_gecko_channel_setup,
	.read = adc_gecko_read,
};

#define GECKO_ADC_INIT(n)						\
									\
	static void adc_gecko_config_func_##n(void);			\
									\
	const static struct adc_gecko_config adc_gecko_config_##n = {	\
		.base = (ADC_TypeDef *)DT_INST_REG_ADDR(n),		\
		.irq_cfg_func = adc_gecko_config_func_##n,		\
		.frequency = DT_INST_PROP(n, frequency),		\
	};								\
	static struct adc_gecko_data adc_gecko_data_##n = {		\
		ADC_CONTEXT_INIT_TIMER(adc_gecko_data_##n, ctx),	\
		ADC_CONTEXT_INIT_LOCK(adc_gecko_data_##n, ctx),		\
		ADC_CONTEXT_INIT_SYNC(adc_gecko_data_##n, ctx),		\
	};								\
	static void adc_gecko_config_func_##n(void)			\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),	\
			    DT_INST_IRQ(n, priority), \
			    adc_gecko_isr, DEVICE_DT_INST_GET(n), 0);	\
		irq_enable(DT_INST_IRQN(n));	\
	}; \
	DEVICE_DT_INST_DEFINE(n,					 \
			      &adc_gecko_init, NULL,			 \
			      &adc_gecko_data_##n, &adc_gecko_config_##n,\
			      POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,	 \
			      &api_gecko_adc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GECKO_ADC_INIT)
