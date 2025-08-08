/*
 * Copyright 2017, 2024-2025 NXP
 * Copyright (c) 2020-2021 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_ftm

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>

#include <fsl_ftm.h>

LOG_MODULE_REGISTER(mcux_ftm, CONFIG_COUNTER_LOG_LEVEL);

/* Each channel has a set of control registers, which indicates the number of channles*/
#define MAX_CHANNELS FTM_CONTROLS_COUNT

struct mcux_ftm_channel_data {
	counter_alarm_callback_t alarm_callback;
	void *alarm_user_data;
};

struct mcux_ftm_config {
	struct counter_config_info info;
	FTM_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;

	ftm_clock_source_t ftm_clock_source;
	ftm_clock_prescale_t prescale;
	uint8_t channel_count;
	void (*irq_config_func)(const struct device *dev);
};

struct mcux_ftm_data {
	uint32_t freq;
	struct mcux_ftm_channel_data channels[MAX_CHANNELS];
	counter_top_callback_t top_callback;
	void *top_user_data;
};

static int mcux_ftm_start(const struct device *dev)
{
	const struct mcux_ftm_config *config = dev->config;

	FTM_StartTimer(config->base, config->ftm_clock_source);

	return 0;
}

static int mcux_ftm_stop(const struct device *dev)
{
	const struct mcux_ftm_config *config = dev->config;

	FTM_StopTimer(config->base);

	return 0;
}

static int mcux_ftm_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct mcux_ftm_config *config = dev->config;

	*ticks = FTM_GetCurrentTimerCount(config->base);

	return 0;
}

static uint32_t mcux_ftm_get_top_value(const struct device *dev)
{
	const struct mcux_ftm_config *config = dev->config;

	return config->base->MOD;
}

static int mcux_ftm_set_alarm(const struct device *dev, uint8_t chan_id,
			      const struct counter_alarm_cfg *alarm_cfg)
{
	const struct mcux_ftm_config *config = dev->config;
	struct mcux_ftm_data *data = dev->data;
	uint32_t current = FTM_GetCurrentTimerCount(config->base);
	uint32_t top_value = mcux_ftm_get_top_value(dev);
	uint32_t ticks = alarm_cfg->ticks;
	uint32_t reg;

	if (chan_id >= config->channel_count) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	if (data->channels[chan_id].alarm_callback) {
		LOG_ERR("channel already in use");
		return -EBUSY;
	}

	if (ticks > (top_value)) {
		return -EINVAL;
	}

	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) == 0) {
		if (top_value - current >= ticks) {
			ticks += current;
		} else {
			ticks -= top_value - current;
		}
	}

	data->channels[chan_id].alarm_callback = alarm_cfg->callback;
	data->channels[chan_id].alarm_user_data = alarm_cfg->user_data;

	/* Compared value can only be updated when clock stops*/
	FTM_StopTimer(config->base);
	/* Set compare mode for the channel */
	reg = config->base->CONTROLS[chan_id].CnSC;
	reg &= ~(FTM_CnSC_MSA_MASK | FTM_CnSC_MSB_MASK | FTM_CnSC_ELSA_MASK | FTM_CnSC_ELSB_MASK);
	reg |= FTM_CnSC_MSA(1U);
	config->base->CONTROLS[chan_id].CnSC = reg;
	/* Set compare value */
	config->base->CONTROLS[chan_id].CnV = ticks;

	/* To prevent the match event from triggering an interrupt immediately, causing the alarm
	 * to not occur at the expected time, Clear status bits before enabling interrupts
	 */
	FTM_ClearStatusFlags(config->base, BIT(chan_id));
	FTM_EnableInterrupts(config->base, BIT(chan_id));
	FTM_StartTimer(config->base, config->ftm_clock_source);

	return 0;
}

static int mcux_ftm_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct mcux_ftm_config *config = dev->config;
	struct mcux_ftm_data *data = dev->data;

	if (chan_id >= config->channel_count) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}
	data->channels[chan_id].alarm_callback = NULL;
	data->channels[chan_id].alarm_user_data = NULL;

	FTM_DisableInterrupts(config->base, BIT(chan_id));
	FTM_ClearStatusFlags(config->base, BIT(chan_id));

	return 0;
}

void mcux_ftm_isr(const struct device *dev)
{
	const struct mcux_ftm_config *config = dev->config;
	struct mcux_ftm_data *data = dev->data;
	uint32_t current = FTM_GetCurrentTimerCount(config->base);
	uint32_t status = FTM_GetStatusFlags(config->base);
	FTM_ClearStatusFlags(config->base, status);

	for(uint8_t chan = 0; chan < config->info.channels; chan++) {
		if ((status & BIT(chan)) != 0 && (data->channels[chan].alarm_callback != NULL)) {
			counter_alarm_callback_t alarm_callback =
				data->channels[chan].alarm_callback;
			void *alarm_user_data = data->channels[chan].alarm_user_data;

			data->channels[chan].alarm_callback = NULL;
			data->channels[chan].alarm_user_data = NULL;
			alarm_callback(dev, chan, current, alarm_user_data);
		}
	}

	if (((status & kFTM_TimeOverflowFlag) != 0) && (data->top_callback != NULL)) {
		data->top_callback(dev, data->top_user_data);
	}
}

static uint32_t mcux_ftm_get_pending_int(const struct device *dev)
{
	const struct mcux_ftm_config *config = dev->config;

	return FTM_GetStatusFlags(config->base);
}

static int mcux_ftm_set_top_value(const struct device *dev,
				  const struct counter_top_cfg *cfg)
{
	const struct mcux_ftm_config *config = dev->config;
	struct mcux_ftm_data *data = dev->data;

	if (cfg->ticks > config->info.max_top_value) {
		return -ENOTSUP;
	}

	/* Check if timer already enabled. */
	if ((config->base->SC & FTM_SC_CLKS_MASK) != 0) {
		/* Timer already enabled, check flags before resetting */
		if ((cfg->flags & COUNTER_TOP_CFG_DONT_RESET) != 0) {
			return -ENOTSUP;
		}

		FTM_StopTimer(config->base);
		config->base->CNT = 0;
		FTM_SetTimerPeriod(config->base, cfg->ticks);
		FTM_StartTimer(config->base, config->ftm_clock_source);
	} else {
		config->base->CNT = 0;
		FTM_SetTimerPeriod(config->base, cfg->ticks);
	}

	data->top_callback = cfg->callback;
	data->top_user_data = cfg->user_data;

	FTM_EnableInterrupts(config->base, kFTM_TimeOverflowInterruptEnable);

	return 0;
}


static uint32_t mcux_ftm_get_freq(const struct device *dev)
{
	struct mcux_ftm_data *data = dev->data;

	return data->freq;
}

static int mcux_ftm_init(const struct device *dev)
{
	const struct mcux_ftm_config *config = dev->config;
	struct mcux_ftm_data *data = dev->data;
	ftm_config_t ftmConfig;
	uint32_t clk_freq;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	for (uint8_t chan = 0; chan < config->channel_count; chan++) {
		data->channels[chan].alarm_callback = NULL;
		data->channels[chan].alarm_user_data = NULL;
	}

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
				   &clk_freq)) {
		LOG_ERR("Could not get clock frequency");
		return -EINVAL;
	}

	data->freq = clk_freq / (1U << config->prescale);

	FTM_GetDefaultConfig(&ftmConfig);
	ftmConfig.prescale = config->prescale;
	FTM_Init(config->base, &ftmConfig);

	config->irq_config_func(dev);

	FTM_SetTimerPeriod(config->base, config->info.max_top_value);

	return 0;
}

static DEVICE_API(counter, mcux_ftm_driver_api) = {
	.start = mcux_ftm_start,
	.stop = mcux_ftm_stop,
	.get_value = mcux_ftm_get_value,
	.set_alarm = mcux_ftm_set_alarm,
	.cancel_alarm = mcux_ftm_cancel_alarm,
	.set_top_value = mcux_ftm_set_top_value,
	.get_pending_int = mcux_ftm_get_pending_int,
	.get_top_value = mcux_ftm_get_top_value,
	.get_freq = mcux_ftm_get_freq,
};

#define TO_FTM_PRESCALE_DIVIDE(val) _DO_CONCAT(kFTM_Prescale_Divide_, val)

#define COUNTER_MCUX_FTM_DEVICE_INIT(n)                                                            \
	static struct mcux_ftm_data mcux_ftm_data_##n;                                             \
	static void mcux_ftm_irq_config_##n(const struct device *dev);                             \
                                                                                                   \
	static const struct mcux_ftm_config mcux_ftm_config_##n = {                                \
		.info =                                                                            \
			{                                                                          \
				.max_top_value = 0xFFFFU,                                          \
				.freq = 0,                                                         \
				.channels = FSL_FEATURE_FTM_CHANNEL_COUNTn(                        \
					(FTM_Type *)DT_INST_REG_ADDR(n)),                          \
				.flags = COUNTER_CONFIG_INFO_COUNT_UP,                             \
			},                                                                         \
		.base = (FTM_Type *)DT_INST_REG_ADDR(n),                                           \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),              \
		.ftm_clock_source = (ftm_clock_source_t)(DT_INST_ENUM_IDX(n, clock_source) + 1U),  \
		.prescale = TO_FTM_PRESCALE_DIVIDE(DT_INST_PROP(n, prescaler)),                    \
		.channel_count = FSL_FEATURE_FTM_CHANNEL_COUNTn((FTM_Type *)DT_INST_REG_ADDR(n)),  \
		.irq_config_func = mcux_ftm_irq_config_##n,                                        \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, mcux_ftm_init, NULL, &mcux_ftm_data_##n, &mcux_ftm_config_##n,    \
			      POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY, &mcux_ftm_driver_api);    \
                                                                                                   \
	static void mcux_ftm_irq_config_##n(const struct device *dev)                              \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), mcux_ftm_isr,               \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

DT_INST_FOREACH_STATUS_OKAY(COUNTER_MCUX_FTM_DEVICE_INIT)
