/*
 * Copyright (c) 2019, Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/counter.h>
#include <fsl_gpt.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(mcux_gpt, CONFIG_COUNTER_LOG_LEVEL);

struct mcux_gpt_config {
	/* info must be first element */
	struct counter_config_info info;
	GPT_Type *base;
	clock_name_t clock_source;
};

struct mcux_gpt_data {
	counter_alarm_callback_t alarm_callback;
	counter_top_callback_t top_callback;
	void *alarm_user_data;
	void *top_user_data;
};

static int mcux_gpt_start(struct device *dev)
{
	const struct mcux_gpt_config *config = dev->config->config_info;

	GPT_StartTimer(config->base);

	return 0;
}

static int mcux_gpt_stop(struct device *dev)
{
	const struct mcux_gpt_config *config = dev->config->config_info;

	GPT_StopTimer(config->base);

	return 0;
}

static int mcux_gpt_get_value(struct device *dev, u32_t *ticks)
{
	const struct mcux_gpt_config *config = dev->config->config_info;

	*ticks = GPT_GetCurrentTimerCount(config->base);
	return 0;
}

static int mcux_gpt_set_alarm(struct device *dev, u8_t chan_id,
			      const struct counter_alarm_cfg *alarm_cfg)
{
	const struct mcux_gpt_config *config = dev->config->config_info;
	struct mcux_gpt_data *data = dev->driver_data;

	u32_t current = GPT_GetCurrentTimerCount(config->base);
	u32_t ticks = alarm_cfg->ticks;

	if (chan_id != 0) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) == 0) {
		ticks += current;
	}

	if (data->alarm_callback) {
		return -EBUSY;
	}

	data->alarm_callback = alarm_cfg->callback;
	data->alarm_user_data = alarm_cfg->user_data;

	GPT_SetOutputCompareValue(config->base, kGPT_OutputCompare_Channel1,
				  ticks);
	GPT_EnableInterrupts(config->base, kGPT_OutputCompare1InterruptEnable);

	return 0;
}

static int mcux_gpt_cancel_alarm(struct device *dev, u8_t chan_id)
{
	const struct mcux_gpt_config *config = dev->config->config_info;
	struct mcux_gpt_data *data = dev->driver_data;

	if (chan_id != 0) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	GPT_DisableInterrupts(config->base, kGPT_OutputCompare1InterruptEnable);
	data->alarm_callback = NULL;

	return 0;
}

void mcux_gpt_isr(void *p)
{
	struct device *dev = p;
	const struct mcux_gpt_config *config = dev->config->config_info;
	struct mcux_gpt_data *data = dev->driver_data;
	u32_t current = GPT_GetCurrentTimerCount(config->base);
	u32_t status;

	status =  GPT_GetStatusFlags(config->base, kGPT_OutputCompare1Flag |
				     kGPT_RollOverFlag);
	GPT_ClearStatusFlags(config->base, status);
	__DSB();

	if ((status & kGPT_OutputCompare1Flag) && data->alarm_callback) {
		GPT_DisableInterrupts(config->base,
				      kGPT_OutputCompare1InterruptEnable);
		counter_alarm_callback_t alarm_cb = data->alarm_callback;
		data->alarm_callback = NULL;
		alarm_cb(dev, 0, current, data->alarm_user_data);
	}

	if ((status & kGPT_RollOverFlag) && data->top_callback) {
		data->top_callback(dev, data->top_user_data);
	}
}

static u32_t mcux_gpt_get_pending_int(struct device *dev)
{
	const struct mcux_gpt_config *config = dev->config->config_info;

	return GPT_GetStatusFlags(config->base, kGPT_OutputCompare1Flag);
}

static int mcux_gpt_set_top_value(struct device *dev,
				  const struct counter_top_cfg *cfg)
{
	const struct mcux_gpt_config *config = dev->config->config_info;
	struct mcux_gpt_data *data = dev->driver_data;

	if (cfg->ticks != config->info.max_top_value) {
		LOG_ERR("Wrap can only be set to 0x%x",
			config->info.max_top_value);
		return -ENOTSUP;
	}

	data->top_callback = cfg->callback;
	data->top_user_data = cfg->user_data;

	GPT_EnableInterrupts(config->base, kGPT_RollOverFlagInterruptEnable);

	return 0;
}

static u32_t mcux_gpt_get_top_value(struct device *dev)
{
	const struct mcux_gpt_config *config = dev->config->config_info;

	return config->info.max_top_value;
}

static u32_t mcux_gpt_get_max_relative_alarm(struct device *dev)
{
	const struct mcux_gpt_config *config = dev->config->config_info;

	return config->info.max_top_value;
}

static int mcux_gpt_init(struct device *dev)
{
	const struct mcux_gpt_config *config = dev->config->config_info;
	gpt_config_t gptConfig;
	u32_t clock_freq;

	/* Adjust divider to match expected freq */
	clock_freq = CLOCK_GetFreq(config->clock_source);
	if (clock_freq % config->info.freq) {
		LOG_ERR("Cannot Adjust GPT freq to %u\n", config->info.freq);
		return -EINVAL;
	}

	GPT_GetDefaultConfig(&gptConfig);
	gptConfig.enableFreeRun = true; /* Do not reset on compare */
	gptConfig.clockSource = kGPT_ClockSource_Periph;
	gptConfig.divider = clock_freq / config->info.freq;
	GPT_Init(config->base, &gptConfig);

	return 0;
}

static const struct counter_driver_api mcux_gpt_driver_api = {
	.start = mcux_gpt_start,
	.stop = mcux_gpt_stop,
	.get_value = mcux_gpt_get_value,
	.set_alarm = mcux_gpt_set_alarm,
	.cancel_alarm = mcux_gpt_cancel_alarm,
	.set_top_value = mcux_gpt_set_top_value,
	.get_pending_int = mcux_gpt_get_pending_int,
	.get_top_value = mcux_gpt_get_top_value,
	.get_max_relative_alarm = mcux_gpt_get_max_relative_alarm,
};

#define GPT_DEVICE_INIT_MCUX(n)			  \
	static struct mcux_gpt_data mcux_gpt_data_ ## n;			\
										\
	static const struct mcux_gpt_config mcux_gpt_config_ ## n = {    	\
		.base = (void *)DT_ALIAS_COUNTER_ ## n ## _BASE_ADDRESS,	\
		.clock_source = kCLOCK_PerClk,					\
		.info = {							\
			.max_top_value = UINT32_MAX,				\
			.freq = 25000000,					\
			.channels = 1,						\
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,			\
		},								\
	};									\
										\
	static int mcux_gpt_## n ##_init(struct device *dev);			\
	DEVICE_AND_API_INIT(mcux_gpt ## n,					\
			    DT_ALIAS_COUNTER_ ## n ## _LABEL,			\
			    mcux_gpt_## n ##_init,				\
			    &mcux_gpt_data_ ## n,				\
			    &mcux_gpt_config_ ## n,				\
			    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
			    &mcux_gpt_driver_api);				\
										\
	static int mcux_gpt_## n ##_init(struct device *dev)			\
	{	 								\
		IRQ_CONNECT(DT_ALIAS_COUNTER_## n ##_IRQ_0,			\
			    DT_ALIAS_COUNTER_## n ##_IRQ_0_PRIORITY,		\
			    mcux_gpt_isr, DEVICE_GET(mcux_gpt ## n), 0);	\
		irq_enable(DT_ALIAS_COUNTER_## n ##_IRQ_0);			\
		return mcux_gpt_init(dev);					\
	}									\

#ifdef CONFIG_COUNTER_MCUX_GPT1
GPT_DEVICE_INIT_MCUX(1)
#endif

#ifdef CONFIG_COUNTER_MCUX_GPT2
GPT_DEVICE_INIT_MCUX(2)
#endif
