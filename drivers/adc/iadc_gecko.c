/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_gecko_iadc

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_silabs.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/pm/device.h>

#include <em_iadc.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(iadc_gecko, CONFIG_ADC_LOG_LEVEL);

/* Number of channels available. */
#define GECKO_CHANNEL_COUNT	16
#define GECKO_INTERNAL_REFERENCE_mV	1210
#define GECKO_DATA_RES12BIT(DATA)	((DATA) & 0x0FFF)

struct adc_gecko_channel_config {
	IADC_CfgAnalogGain_t gain;
	IADC_CfgReference_t reference;
	IADC_PosInput_t input_positive;
	IADC_NegInput_t input_negative;
	bool initialized;
};

struct adc_gecko_data {
	const struct device *dev;
	struct adc_context ctx;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint32_t channels;
	uint8_t channel_id;
	struct adc_gecko_channel_config channel_config[GECKO_CHANNEL_COUNT];
};

struct adc_gecko_config {
	IADC_Config_t config;
	IADC_TypeDef *base;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	const struct silabs_clock_control_cmu_config clock_cfg;
	void (*irq_cfg_func)(void);
};

static void adc_gecko_set_config(const struct device *dev)
{
	struct adc_gecko_data *data = dev->data;
	struct adc_gecko_channel_config *channel_config = NULL;
	const struct adc_gecko_config *config = dev->config;

	IADC_TypeDef *iadc = (IADC_TypeDef *)config->base;
	IADC_InitSingle_t sInit = IADC_INITSINGLE_DEFAULT;
	IADC_SingleInput_t initSingleInput = IADC_SINGLEINPUT_DEFAULT;
	IADC_Init_t init = IADC_INIT_DEFAULT;
	IADC_AllConfigs_t initAllConfigs = IADC_ALLCONFIGS_DEFAULT;

	channel_config = &data->channel_config[data->channel_id];

	initSingleInput.posInput = channel_config->input_positive;
	initSingleInput.negInput = channel_config->input_negative;

	initAllConfigs.configs[0].analogGain = channel_config->gain;

	initAllConfigs.configs[0].reference = channel_config->reference;

	IADC_init(iadc, &init, &initAllConfigs);

	IADC_initSingle(iadc, &sInit, &initSingleInput);
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

static int adc_gecko_check_resolution(const struct adc_sequence *sequence)
{
	int value = sequence->resolution;

	/* Base resolution is on 12, it can be changed only up by oversampling */
	if (value != 12) {
		return -EINVAL;
	}

	return value;
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
		return -EINVAL;
	}

	/* Check resolution setting */
	res = adc_gecko_check_resolution(sequence);
	if (res < 0) {
		return -EINVAL;
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

static void adc_gecko_start_channel(const struct device *dev)
{
	const struct adc_gecko_config *config = dev->config;
	struct adc_gecko_data *data = dev->data;

	IADC_TypeDef *iadc = (IADC_TypeDef *)config->base;

	data->channel_id = find_lsb_set(data->channels) - 1;

	LOG_DBG("Starting channel %d", data->channel_id);

	adc_gecko_set_config(data->dev);

	/* Enable single conversion interrupt */
	IADC_enableInt(iadc, IADC_IEN_SINGLEDONE);

	/* Start single conversion */
	IADC_command(iadc, iadcCmdStartSingle);
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
	const struct device *dev = (const struct device *)arg;
	const struct adc_gecko_config *config = dev->config;
	struct adc_gecko_data *data = dev->data;
	IADC_TypeDef *iadc = config->base;
	IADC_Result_t sample;
	uint32_t flags, err;

	/*
	 * IRQ is enabled only for SINGLEDONE. However, other
	 * interrupt flags - the ones singaling an error - may be
	 * set simultaneously with SINGLEDONE. We read & clear them
	 * to determine if conversion is successful or not.
	 */
	flags = IADC_getInt(iadc);

	__ASSERT(flags & IADC_IF_SINGLEDONE,
		 "unexpected IADC IRQ (flags=0x%08x)!", flags);

	err = flags & (IADC_IF_PORTALLOCERR |
			IADC_IF_POLARITYERR |
			IADC_IF_EM23ABORTERROR);
	if (!err) {
		sample = IADC_readSingleResult(iadc);

		*data->buffer++ = GECKO_DATA_RES12BIT((uint16_t)sample.data);
		data->channels &= ~BIT(data->channel_id);

		if (data->channels) {
			adc_gecko_start_channel(dev);
		} else {
			adc_context_on_sampling_done(&data->ctx, dev);
		}
	} else {
		LOG_ERR("IADC conversion error, flags=%08x", err);
		adc_context_complete(&data->ctx, -EIO);
	}

	IADC_clearInt(iadc, IADC_IF_SINGLEDONE | err);
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

#ifdef CONFIG_ADC_ASYNC
static int adc_gecko_read_async(const struct device *dev,
				const struct adc_sequence *sequence,
				struct k_poll_signal *async)
{
	struct adc_gecko_data *data = dev->data;
	int error;

	adc_context_lock(&data->ctx, true, async);
	error = start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}
#endif

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

	channel_config->input_positive = channel_cfg->input_positive;

	if (channel_cfg->differential) {
		channel_config->input_negative = channel_cfg->input_negative;
	} else {
		channel_config->input_negative = iadcNegInputGnd;
	}

	/* Setup input */

	switch (channel_cfg->gain) {
#if defined(_IADC_CFG_ANALOGGAIN_ANAGAIN0P25)
	case ADC_GAIN_1_4:
		channel_config->gain = iadcCfgAnalogGain0P25x;
		break;
#endif
	case ADC_GAIN_1_2:
		channel_config->gain = iadcCfgAnalogGain0P5x;
		break;
	case ADC_GAIN_1:
		channel_config->gain = iadcCfgAnalogGain1x;
		break;
	case ADC_GAIN_2:
		channel_config->gain = iadcCfgAnalogGain2x;
		break;
	case ADC_GAIN_3:
		channel_config->gain = iadcCfgAnalogGain3x;
		break;
	case ADC_GAIN_4:
		channel_config->gain = iadcCfgAnalogGain4x;
		break;
	default:
		LOG_ERR("unsupported channel gain '%d'", channel_cfg->gain);
		return -EINVAL;
	}

	/* Setup reference */
	switch (channel_cfg->reference) {
	case ADC_REF_VDD_1:
		channel_config->reference = iadcCfgReferenceVddx;
		break;
	case ADC_REF_INTERNAL:
		channel_config->reference = iadcCfgReferenceInt1V2;
		break;
#if defined(_IADC_CFG_REFSEL_VREF2P5)
	case ADC_REF_EXTERNAL1:
		channel_config->reference = iadcCfgReferenceExt2V5;
		break;
#endif
	case ADC_REF_EXTERNAL0:
		channel_config->reference = iadcCfgReferenceExt1V25;
		break;
	default:
		LOG_ERR("unsupported channel reference type '%d'",
			channel_cfg->reference);
		return -EINVAL;
	}

	channel_config->initialized = true;
	LOG_DBG("Channel setup succeeded!");

	return 0;
}

static int adc_gecko_pm_action(const struct device *dev, enum pm_device_action action)
{
	int err;
	const struct adc_gecko_config *config = dev->config;

	if (action == PM_DEVICE_ACTION_RESUME) {
		err = clock_control_on(config->clock_dev,
				       (clock_control_subsys_t)&config->clock_cfg);
		if (err < 0 && err != -EALREADY) {
			return err;
		}

		err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
		if (err < 0 && err != -ENOENT) {
			return err;
		}
	} else if (IS_ENABLED(CONFIG_PM_DEVICE) && (action == PM_DEVICE_ACTION_SUSPEND)) {
		err = clock_control_off(config->clock_dev,
					(clock_control_subsys_t)&config->clock_cfg);
		if (err < 0) {
			return err;
		}

		err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_SLEEP);
		if (err < 0 && err != -ENOENT) {
			return err;
		}
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int adc_gecko_init(const struct device *dev)
{
	const struct adc_gecko_config *config = dev->config;
	struct adc_gecko_data *data = dev->data;

	data->dev = dev;

	config->irq_cfg_func();

	adc_context_unlock_unconditionally(&data->ctx);

	return pm_device_driver_init(dev, adc_gecko_pm_action);
}

static DEVICE_API(adc, api_gecko_adc_driver_api) = {
	.channel_setup = adc_gecko_channel_setup,
	.read = adc_gecko_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_gecko_read_async,
#endif
	.ref_internal = GECKO_INTERNAL_REFERENCE_mV,
};

#define GECKO_IADC_INIT(n)						\
	PINCTRL_DT_INST_DEFINE(n);					\
	PM_DEVICE_DT_INST_DEFINE(n, adc_gecko_pm_action);		\
									\
	static void adc_gecko_config_func_##n(void);			\
									\
	const static struct adc_gecko_config adc_gecko_config_##n = {	\
		.base = (IADC_TypeDef *)DT_INST_REG_ADDR(n),\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),	\
		.clock_cfg = SILABS_DT_INST_CLOCK_CFG(n),		\
		.irq_cfg_func = adc_gecko_config_func_##n,		\
	};								\
	static struct adc_gecko_data adc_gecko_data_##n = {		\
		ADC_CONTEXT_INIT_TIMER(adc_gecko_data_##n, ctx),	\
		ADC_CONTEXT_INIT_LOCK(adc_gecko_data_##n, ctx),	\
		ADC_CONTEXT_INIT_SYNC(adc_gecko_data_##n, ctx),	\
	};								\
	static void adc_gecko_config_func_##n(void)			\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),	\
			    DT_INST_IRQ(n, priority), \
			    adc_gecko_isr, DEVICE_DT_INST_GET(n), 0);	\
		irq_enable(DT_INST_IRQN(n));	\
	}; \
	DEVICE_DT_INST_DEFINE(n,					 \
			      &adc_gecko_init, PM_DEVICE_DT_INST_GET(n), \
			      &adc_gecko_data_##n, &adc_gecko_config_##n,\
			      POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,	 \
			      &api_gecko_adc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GECKO_IADC_INIT)
