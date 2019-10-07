/*
 * Copyright (c) 2019 Interay Solutions B.V.
 * Copyright (c) 2019 Oane Kingma
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <drivers/adc.h>
#include <device.h>
#include <kernel.h>
#include <init.h>
#include <soc.h>
#include <em_adc.h>
#include <em_cmu.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(adc_gecko);

#define ADC_PREFIX cmuClock_ADC
#define CLOCK_ADC(id) _CONCAT(ADC_PREFIX, id)

/* Acquisition times */
static const u16_t acq_time_single_tbl[10] = {
	1, 2, 3, 4, 8, 16, 32, 64, 128, 256
};

/* Number of channels available. */
#define GECKO_CHANNEL_COUNT	32

struct adc_gecko_ch_cfg {
	ADC_Ref_TypeDef reference;
	ADC_AcqTime_TypeDef acq_time;
	ADC_PosSel_TypeDef input_positive;
	bool initialized;
};

struct adc_gecko_data {
	struct adc_context ctx;
	struct device *dev;
	u16_t *buffer;
	u16_t *repeat_buffer;
	u32_t channels;
	u8_t channel_id;
	int resolution;
	struct adc_gecko_ch_cfg ch_cfg[GECKO_CHANNEL_COUNT];
};

struct adc_gecko_cfg {
	ADC_TypeDef *base;
	CMU_Clock_TypeDef clock;
	u32_t prescaler;
	void (*irq_cfg_func)(void);
};

#define DEV_NAME(dev) ((dev)->config->name)
#define DEV_DATA(dev) \
	((struct adc_gecko_data *)(dev)->driver_data)
#define DEV_CFG(dev) \
	((struct adc_gecko_cfg *)(dev)->config->config_info)

static int adc_gecko_check_buffer_size(const struct adc_sequence *sequence,
					u8_t active_channels)
{
	size_t needed_buffer_size;

	needed_buffer_size = active_channels * sizeof(u16_t);

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

static int adc_gecko_check_resolution(const struct adc_sequence *sequence)
{
	int value = 0;

	switch (sequence->resolution) {
	case 6:
		value = adcRes6Bit;
		break;
	case 8:
		value = adcRes8Bit;
		break;
	case 12:
		value = adcRes12Bit;
		break;
	default:
		LOG_ERR("Invalid resolution");
		return -EINVAL;
	}

	return value;
}

static int start_read(struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_gecko_data *data = DEV_DATA(dev);
	u32_t channels;
	u8_t channel_count;
	u8_t index;
	int res;

	/* Check at least 1 channel is requested */
	if (sequence->channels == 0) {
		LOG_ERR("No channel requested");
		return -EINVAL;
	}

	/* Check oversampling setting */
	if (sequence->oversampling != 0) {
		LOG_ERR("Oversampling is not supported");
		return -ENOTSUP;
	}

	/* Check resolution setting */
	res = adc_gecko_check_resolution(sequence);
	if (res < 0) {
		return -EINVAL;
	}
	data->resolution = res;

	/* Verify all requested channels are initialized and store resolution */
	channels = sequence->channels;
	channel_count = 0;
	while (channels) {
		index = find_lsb_set(channels) - 1;
		if (!data->ch_cfg[index].initialized) {
			LOG_ERR("Channel not initialized");
			return -EINVAL;
		}
		channel_count++;
		channels &= ~BIT(index);
	}

	/* Check buffer size */
	res = adc_gecko_check_buffer_size(sequence, channel_count);
	if (res < 0) {
		return res;
	}

	data->buffer = sequence->buffer;

	adc_context_start_read(&data->ctx, sequence);

	res = adc_context_wait_for_completion(&data->ctx);
	return res;
}

static void adc_gecko_start_channel(struct device *dev)
{
	const struct adc_gecko_cfg *config = DEV_CFG(dev);
	struct adc_gecko_data *data = DEV_DATA(dev);
	ADC_TypeDef *adc = (ADC_TypeDef *)config->base;
	ADC_InitSingle_TypeDef sInit = ADC_INITSINGLE_DEFAULT;
	struct adc_gecko_ch_cfg *config_ch = NULL;

	data->channel_id = find_lsb_set(data->channels) - 1;

	LOG_DBG("Starting channel %d", data->channel_id);

	/* Apply previously set configuration options */
	config_ch = &data->ch_cfg[data->channel_id];
	sInit.resolution = (ADC_Res_TypeDef)data->resolution;
	sInit.reference = config_ch->reference;
	sInit.acqTime = config_ch->acq_time;
	sInit.posSel = config_ch->input_positive;
	sInit.negSel = adcNegSelVSS;
	ADC_InitSingle(adc, &sInit);

	/* Enable single conversion interrupt */
	ADC_IntEnable(adc, ADC_IEN_SINGLE);

	/* Start single conversion */
	ADC_Start(adc, adcStartSingle);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_gecko_data *data =
		CONTAINER_OF(ctx, struct adc_gecko_data, ctx);

	data->channels = ctx->sequence.channels;
	data->repeat_buffer = data->buffer;

	adc_gecko_start_channel(data->dev);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
						bool repeat_sampling)
{
	struct adc_gecko_data *data =
		CONTAINER_OF(ctx, struct adc_gecko_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static void adc_gecko_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct adc_gecko_cfg *config = DEV_CFG(dev);
	struct adc_gecko_data *data = DEV_DATA(dev);
	ADC_TypeDef *adc = config->base;

	*data->buffer++ = (u16_t)ADC_DataSingleGet(adc);
	data->channels &= ~BIT(data->channel_id);

	if (data->channels) {
		adc_gecko_start_channel(dev);
	} else {
		adc_context_on_sampling_done(&data->ctx, dev);
	}

	LOG_DBG("ISR triggered.");
}

static int adc_gecko_read(struct device *dev,
			  const struct adc_sequence *sequence)
{
	struct adc_gecko_data *data = DEV_DATA(dev);
	int error;

	adc_context_lock(&data->ctx, false, NULL);
	error = start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}

#ifdef CONFIG_ADC_ASYNC
static int adc_gecko_read_async(struct device *dev,
				const struct adc_sequence *sequence,
				struct k_poll_signal *async)
{
	struct adc_gecko_data *data = DEV_DATA(dev);
	int error;

	adc_context_lock(&data->ctx, true, async);
	error = start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}
#endif

static int adc_gecko_check_acq_time(u16_t acq_time)
{
	if (acq_time == ADC_ACQ_TIME_DEFAULT) {
		return 0;
	}

	for (int i = 0; i < ARRAY_SIZE(acq_time_single_tbl); i++) {
		if (acq_time == ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS,
						acq_time_single_tbl[i])) {
			return i;
		}
	}

	LOG_ERR("Conversion time not supported.");
	return -EINVAL;
}


static int adc_gecko_channel_setup(struct device *dev,
				const struct adc_channel_cfg *channel_cfg)
{
	struct adc_gecko_data *data = DEV_DATA(dev);
	struct adc_gecko_ch_cfg *config_ch = NULL;
	int acq_time_index;

	if (channel_cfg->channel_id >= GECKO_CHANNEL_COUNT) {
		LOG_ERR("Channel %d is not valid", channel_cfg->channel_id);
		return -EINVAL;
	}

	if (channel_cfg->differential) {
		LOG_ERR("Differential channels are not (yet) supported");
		return -ENOTSUP;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Invalid channel gain");
		return -EINVAL;
	}

	config_ch = &data->ch_cfg[channel_cfg->channel_id];
	config_ch->initialized = false;

	/* Setup input */
	config_ch->input_positive = channel_cfg->input_positive;

	/* Setup acquistion time */
	acq_time_index = adc_gecko_check_acq_time(
		channel_cfg->acquisition_time);
	if (acq_time_index < 0) {
		return -EINVAL;
	}
	config_ch->acq_time = acq_time_index;

	/* Setup reference */
	switch (channel_cfg->reference) {
	case ADC_REF_VDD_1:
	case ADC_REF_INTERNAL:
		config_ch->reference = adcRef2V5;
		break;

	case ADC_REF_VDD_1_2:
		config_ch->reference = adcRef1V25;
		break;

	default:
		LOG_ERR("Invalid channel reference");
		return -EINVAL;
	}

	config_ch->initialized = true;
	LOG_DBG("Channel setup succeeded!");

	return 0;
}

static int adc_gecko_init(struct device *dev)
{
	const struct adc_gecko_cfg *config = DEV_CFG(dev);
	struct adc_gecko_data *data = DEV_DATA(dev);
	struct adc_gecko_ch_cfg *config_ch = NULL;
	ADC_TypeDef *adc = (ADC_TypeDef *)config->base;
	int ch;

	LOG_DBG("Initializing....");

	data->dev = dev;

	for (ch = 0; ch < GECKO_CHANNEL_COUNT; ch++) {
		config_ch = &data->ch_cfg[ch];
		config_ch->initialized = false;
	}

	/* Enable ADC clock */
	CMU_ClockEnable(config->clock, true);

	/* Base the ADC configuration on the default setup. */
	ADC_Init_TypeDef init = ADC_INIT_DEFAULT;

	/* Initialize timebases */
	init.timebase = ADC_TimebaseCalc(0);
	init.prescale = (config->prescaler - 1);
	ADC_Init(adc, &init);

	config->irq_cfg_func();

	adc_context_unlock_unconditionally(&data->ctx);

	LOG_INF("Device %s initialized", DEV_NAME(dev));

	return 0;
}

static const struct adc_driver_api api_gecko_driver_api = {
	.channel_setup = adc_gecko_channel_setup,
	.read = adc_gecko_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_gecko_read_async,
#endif
	.ref_internal = 2500,
};

#define GECKO_ADC_INIT(index)						\
									\
	static void adc_gecko_cfg_func_##index(void);			\
									\
	static const struct adc_gecko_cfg adc_gecko_cfg_##index = {	\
		.base = (ADC_TypeDef *)					\
			DT_INST_##index##_SILABS_GECKO_ADC_BASE_ADDRESS,\
		.irq_cfg_func = adc_gecko_cfg_func_##index,		\
		.clock = CLOCK_ADC(					\
			DT_INST_##index##_SILABS_GECKO_ADC_PERIPHERAL_ID), \
		.prescaler = DT_INST_##index##_SILABS_GECKO_ADC_PRESCALER \
	};								\
	static struct adc_gecko_data adc_gecko_data_##index = {		\
		ADC_CONTEXT_INIT_TIMER(adc_gecko_data_##index, ctx),	\
		ADC_CONTEXT_INIT_LOCK(adc_gecko_data_##index, ctx),	\
		ADC_CONTEXT_INIT_SYNC(adc_gecko_data_##index, ctx),	\
	};								\
									\
	DEVICE_AND_API_INIT(adc_##index,				\
			    DT_INST_##index##_SILABS_GECKO_ADC_LABEL,	\
			    &adc_gecko_init, &adc_gecko_data_##index,	\
			    &adc_gecko_cfg_##index, POST_KERNEL,	\
			    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,	\
			    &api_gecko_driver_api);			\
									\
	static void adc_gecko_cfg_func_##index(void)			\
	{								\
		IRQ_CONNECT(DT_INST_##index##_SILABS_GECKO_ADC_IRQ_0,	\
			    DT_INST_##index##_SILABS_GECKO_ADC_IRQ_0_PRIORITY, \
			    adc_gecko_isr, DEVICE_GET(adc_##index), 0);	\
		irq_enable(DT_INST_##index##_SILABS_GECKO_ADC_IRQ_0);	\
	}

#ifdef DT_INST_0_SILABS_GECKO_ADC
GECKO_ADC_INIT(0)
#endif /* DT_INST_0_SILABS_GECKO_ADC */
