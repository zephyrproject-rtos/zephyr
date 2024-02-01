/*
 * Copyright 2020,2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_pit

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>
#include <fsl_pit.h>

#define LOG_MODULE_NAME counter_pit
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_COUNTER_LOG_LEVEL);

struct mcux_pit_config {
	struct counter_config_info info;
	PIT_Type *base;
	bool enableRunInDebug;
	pit_chnl_t pit_channel;
	uint32_t pit_period;
	void (*irq_config_func)(const struct device *dev);
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

struct mcux_pit_data {
	counter_top_callback_t top_callback;
	void *top_user_data;
};

static uint32_t mcux_pit_get_top_value(const struct device *dev)
{
	const struct mcux_pit_config *config = dev->config;
	pit_chnl_t channel = config->pit_channel;

	/*
	 * According to RM, the LDVAL trigger = clock ticks -1
	 * The underlying HAL driver function PIT_SetTimerPeriod()
	 * automatically subtracted 1 from the value that ends up in
	 * LDVAL so for reporting purposes we need to add it back in
	 * here to by consistent.
	 */
	return (config->base->CHANNEL[channel].LDVAL + 1);
}

static int mcux_pit_start(const struct device *dev)
{
	const struct mcux_pit_config *config = dev->config;

	LOG_DBG("period is %d", mcux_pit_get_top_value(dev));
	PIT_EnableInterrupts(config->base, config->pit_channel,
			     kPIT_TimerInterruptEnable);
	PIT_StartTimer(config->base, config->pit_channel);
	return 0;
}

static int mcux_pit_stop(const struct device *dev)
{
	const struct mcux_pit_config *config = dev->config;

	PIT_DisableInterrupts(config->base, config->pit_channel,
			      kPIT_TimerInterruptEnable);
	PIT_StopTimer(config->base, config->pit_channel);

	return 0;
}

static int mcux_pit_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct mcux_pit_config *config = dev->config;

	*ticks = PIT_GetCurrentTimerCount(config->base, config->pit_channel);

	return 0;
}

static int mcux_pit_set_top_value(const struct device *dev,
				  const struct counter_top_cfg *cfg)
{
	const struct mcux_pit_config *config = dev->config;
	struct mcux_pit_data *data = dev->data;
	pit_chnl_t channel = config->pit_channel;

	if (cfg->ticks == 0) {
		return -EINVAL;
	}

	data->top_callback = cfg->callback;
	data->top_user_data = cfg->user_data;

	if (config->base->CHANNEL[channel].TCTRL & PIT_TCTRL_TEN_MASK) {
		/* Timer already enabled, check flags before resetting */
		if (cfg->flags & COUNTER_TOP_CFG_DONT_RESET) {
			return -ENOTSUP;
		}
		PIT_StopTimer(config->base, channel);
		PIT_SetTimerPeriod(config->base, channel, cfg->ticks);
		PIT_StartTimer(config->base, channel);
	} else {
		PIT_SetTimerPeriod(config->base, channel, cfg->ticks);
	}

	return 0;
}

static uint32_t mcux_pit_get_pending_int(const struct device *dev)
{
	const struct mcux_pit_config *config = dev->config;
	uint32_t mask = PIT_TFLG_TIF_MASK;
	uint32_t flags;

	flags = PIT_GetStatusFlags(config->base, config->pit_channel);

	return ((flags & mask) == mask);
}

static uint32_t mcux_pit_get_frequency(const struct device *dev)
{
	const struct mcux_pit_config *config = dev->config;
	uint32_t clock_rate;

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys, &clock_rate)) {
		LOG_ERR("Failed to get clock rate");
		return 0;
	}

	return clock_rate;
}

static void mcux_pit_isr(const struct device *dev)
{
	const struct mcux_pit_config *config = dev->config;
	struct mcux_pit_data *data = dev->data;
	uint32_t flags;

	LOG_DBG("pit counter isr");
	flags = PIT_GetStatusFlags(config->base, config->pit_channel);
	PIT_ClearStatusFlags(config->base, config->pit_channel, flags);
	if (data->top_callback) {
		data->top_callback(dev, data->top_user_data);
	}
}

static int mcux_pit_init(const struct device *dev)
{
	const struct mcux_pit_config *config = dev->config;
	pit_config_t pit_config;
	uint32_t clock_rate;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("Clock control device not ready");
		return -ENODEV;
	}

	PIT_GetDefaultConfig(&pit_config);
	pit_config.enableRunInDebug = config->enableRunInDebug;

	PIT_Init(config->base, &pit_config);

	config->irq_config_func(dev);

	clock_rate = mcux_pit_get_frequency(dev);
	PIT_SetTimerPeriod(config->base, config->pit_channel,
			   USEC_TO_COUNT(config->pit_period, clock_rate));

	return 0;
}

static const struct counter_driver_api mcux_pit_driver_api = {
	.start = mcux_pit_start,
	.stop = mcux_pit_stop,
	.get_value = mcux_pit_get_value,
	.set_top_value = mcux_pit_set_top_value,
	.get_pending_int = mcux_pit_get_pending_int,
	.get_top_value = mcux_pit_get_top_value,
	.get_freq = mcux_pit_get_frequency,
};

#define COUNTER_MCUX_PIT_IRQ_CONFIG(idx, n)					\
	do {									\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, idx, irq),			\
			DT_INST_IRQ_BY_IDX(n, idx, priority),			\
			mcux_pit_isr, DEVICE_DT_INST_GET(n),			\
			COND_CODE_1(DT_INST_IRQ_HAS_NAME(n, flags),		\
				(DT_INST_IRQ_BY_IDX(n, idx, flags)), (0)));	\
		irq_enable(DT_INST_IRQ_BY_IDX(n, idx, irq));			\
	} while (0)

#define COUNTER_MCUX_PIT_DEVICE(n)						\
	static void mcux_pit_irq_config_##n(const struct device *dev);		\
	static struct mcux_pit_data mcux_pit_data_##n;				\
	static const struct mcux_pit_config mcux_pit_config_##n = {		\
		.info = {							\
			.max_top_value = DT_INST_PROP(n, max_load_value),	\
			.channels = 0,						\
		},								\
		.base = (PIT_Type *)DT_INST_REG_ADDR(n),			\
		.pit_channel = DT_INST_PROP(n, pit_channel),			\
		.pit_period = DT_INST_PROP(n, pit_period),			\
		.irq_config_func = mcux_pit_irq_config_##n,			\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),		\
		.clock_subsys = (clock_control_subsys_t)			\
				DT_INST_CLOCKS_CELL(n, name),			\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, &mcux_pit_init, NULL,				\
			&mcux_pit_data_##n, &mcux_pit_config_##n, POST_KERNEL,	\
			CONFIG_COUNTER_INIT_PRIORITY, &mcux_pit_driver_api);	\
										\
	static void mcux_pit_irq_config_##n(const struct device *dev)		\
	{									\
		LISTIFY(DT_NUM_IRQS(DT_DRV_INST(n)),				\
			COUNTER_MCUX_PIT_IRQ_CONFIG, (;), n);			\
	}

DT_INST_FOREACH_STATUS_OKAY(COUNTER_MCUX_PIT_DEVICE)
