/*
 * Copyright (c) 2019, Linaro Limited.
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_gpt

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>
#include <fsl_gpt.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/barrier.h>

LOG_MODULE_REGISTER(mcux_gpt, CONFIG_COUNTER_LOG_LEVEL);

#define DEV_CFG(_dev) ((const struct mcux_gpt_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct mcux_gpt_data *)(_dev)->data)

struct mcux_gpt_config {
	/* info must be first element */
	struct counter_config_info info;

	DEVICE_MMIO_NAMED_ROM(gpt_mmio);

	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	clock_name_t clock_source;
};

struct mcux_gpt_data {
	DEVICE_MMIO_NAMED_RAM(gpt_mmio);
	counter_alarm_callback_t alarm_callback;
	counter_top_callback_t top_callback;
	void *alarm_user_data;
	void *top_user_data;
};

static GPT_Type *get_base(const struct device *dev)
{
	return (GPT_Type *)DEVICE_MMIO_NAMED_GET(dev, gpt_mmio);
}

static int mcux_gpt_start(const struct device *dev)
{
	GPT_Type *base = get_base(dev);

	GPT_StartTimer(base);

	return 0;
}

static int mcux_gpt_stop(const struct device *dev)
{
	GPT_Type *base = get_base(dev);

	GPT_StopTimer(base);

	return 0;
}

static int mcux_gpt_get_value(const struct device *dev, uint32_t *ticks)
{
	GPT_Type *base = get_base(dev);

	*ticks = GPT_GetCurrentTimerCount(base);
	return 0;
}

static int mcux_gpt_set_alarm(const struct device *dev, uint8_t chan_id,
			      const struct counter_alarm_cfg *alarm_cfg)
{
	GPT_Type *base = get_base(dev);
	struct mcux_gpt_data *data = dev->data;

	uint32_t current = GPT_GetCurrentTimerCount(base);
	uint32_t ticks = alarm_cfg->ticks;

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

	GPT_SetOutputCompareValue(base, kGPT_OutputCompare_Channel1,
				  ticks);
	GPT_EnableInterrupts(base, kGPT_OutputCompare1InterruptEnable);

	return 0;
}

static int mcux_gpt_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	GPT_Type *base = get_base(dev);
	struct mcux_gpt_data *data = dev->data;

	if (chan_id != 0) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	GPT_DisableInterrupts(base, kGPT_OutputCompare1InterruptEnable);
	data->alarm_callback = NULL;

	return 0;
}

void mcux_gpt_isr(const struct device *dev)
{
	GPT_Type *base = get_base(dev);
	struct mcux_gpt_data *data = dev->data;
	uint32_t current = GPT_GetCurrentTimerCount(base);
	uint32_t status;

	status =  GPT_GetStatusFlags(base, kGPT_OutputCompare1Flag |
				     kGPT_RollOverFlag);
	GPT_ClearStatusFlags(base, status);
	barrier_dsync_fence_full();

	if ((status & kGPT_OutputCompare1Flag) && data->alarm_callback) {
		GPT_DisableInterrupts(base,
				      kGPT_OutputCompare1InterruptEnable);
		counter_alarm_callback_t alarm_cb = data->alarm_callback;
		data->alarm_callback = NULL;
		alarm_cb(dev, 0, current, data->alarm_user_data);
	}

	if ((status & kGPT_RollOverFlag) && data->top_callback) {
		data->top_callback(dev, data->top_user_data);
	}
}

static uint32_t mcux_gpt_get_pending_int(const struct device *dev)
{
	GPT_Type *base = get_base(dev);

	return GPT_GetStatusFlags(base, kGPT_OutputCompare1Flag);
}

static int mcux_gpt_set_top_value(const struct device *dev,
				  const struct counter_top_cfg *cfg)
{
	const struct mcux_gpt_config *config = dev->config;
	GPT_Type *base = get_base(dev);
	struct mcux_gpt_data *data = dev->data;

	if (cfg->ticks != config->info.max_top_value) {
		LOG_ERR("Wrap can only be set to 0x%x",
			config->info.max_top_value);
		return -ENOTSUP;
	}

	data->top_callback = cfg->callback;
	data->top_user_data = cfg->user_data;

	GPT_EnableInterrupts(base, kGPT_RollOverFlagInterruptEnable);

	return 0;
}

static uint32_t mcux_gpt_get_top_value(const struct device *dev)
{
	const struct mcux_gpt_config *config = dev->config;

	return config->info.max_top_value;
}

static int mcux_gpt_init(const struct device *dev)
{
	const struct mcux_gpt_config *config = dev->config;
	gpt_config_t gptConfig;
	uint32_t clock_freq;
	GPT_Type *base;

	DEVICE_MMIO_NAMED_MAP(dev, gpt_mmio, K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
				   &clock_freq)) {
		return -EINVAL;
	}

	/* Adjust divider to match expected freq */
	if (clock_freq % config->info.freq) {
		LOG_ERR("Cannot Adjust GPT freq to %u\n", config->info.freq);
		LOG_ERR("clock src is %u\n", clock_freq);
		return -EINVAL;
	}

	GPT_GetDefaultConfig(&gptConfig);
	gptConfig.enableFreeRun = true; /* Do not reset on compare */
	gptConfig.clockSource = kGPT_ClockSource_Periph;
	gptConfig.divider = clock_freq / config->info.freq;
	base = get_base(dev);
	GPT_Init(base, &gptConfig);

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
};

#define GPT_DEVICE_INIT_MCUX(n)						\
	static struct mcux_gpt_data mcux_gpt_data_ ## n;		\
									\
	static const struct mcux_gpt_config mcux_gpt_config_ ## n = {	\
		DEVICE_MMIO_NAMED_ROM_INIT(gpt_mmio, DT_DRV_INST(n)),	\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),	\
		.clock_subsys =						\
			(clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),\
		.info = {						\
			.max_top_value = UINT32_MAX,			\
			.freq = DT_INST_PROP(n, gptfreq),           \
			.channels = 1,					\
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,		\
		},							\
	};								\
									\
	static int mcux_gpt_## n ##_init(const struct device *dev);	\
	DEVICE_DT_INST_DEFINE(n,					\
			    mcux_gpt_## n ##_init,			\
			    NULL,					\
			    &mcux_gpt_data_ ## n,			\
			    &mcux_gpt_config_ ## n,			\
			    POST_KERNEL,				\
			    CONFIG_COUNTER_INIT_PRIORITY,		\
			    &mcux_gpt_driver_api);			\
									\
	static int mcux_gpt_## n ##_init(const struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    mcux_gpt_isr, DEVICE_DT_INST_GET(n), 0);	\
		irq_enable(DT_INST_IRQN(n));				\
		return mcux_gpt_init(dev);				\
	}								\

DT_INST_FOREACH_STATUS_OKAY(GPT_DEVICE_INIT_MCUX)
