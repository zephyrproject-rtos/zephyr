/*
 * Copyright (c) 2021, Toby Firth.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT nxp_lpc_ctimer

#include <zephyr/drivers/counter.h>
#include <fsl_ctimer.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/mcux_lpc_syscon_clock.h>
#include <zephyr/irq.h>

LOG_MODULE_REGISTER(mcux_ctimer, CONFIG_COUNTER_LOG_LEVEL);

#ifdef CONFIG_COUNTER_MCUX_CTIMER_RESERVE_CHANNEL_FOR_SETTOP
/* One of the CTimer channels is reserved to implement set_top_value API */
#define NUM_CHANNELS 3
#else
#define NUM_CHANNELS 4
#endif

struct mcux_lpc_ctimer_channel_data {
	counter_alarm_callback_t alarm_callback;
	void *alarm_user_data;
};

struct mcux_lpc_ctimer_data {
	struct mcux_lpc_ctimer_channel_data channels[NUM_CHANNELS];
	counter_top_callback_t top_callback;
	void *top_user_data;
};

struct mcux_lpc_ctimer_config {
	struct counter_config_info info;
	CTIMER_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	ctimer_timer_mode_t mode;
	ctimer_capture_channel_t input;
	uint32_t prescale;
	void (*irq_config_func)(const struct device *dev);
};

static int mcux_lpc_ctimer_start(const struct device *dev)
{
	const struct mcux_lpc_ctimer_config *config = dev->config;

	CTIMER_StartTimer(config->base);

	return 0;
}

static int mcux_lpc_ctimer_stop(const struct device *dev)
{
	const struct mcux_lpc_ctimer_config *config = dev->config;

	CTIMER_StopTimer(config->base);

	return 0;
}

static uint32_t mcux_lpc_ctimer_read(CTIMER_Type *base)
{
	return CTIMER_GetTimerCountValue(base);
}

static int mcux_lpc_ctimer_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct mcux_lpc_ctimer_config *config = dev->config;
	*ticks = mcux_lpc_ctimer_read(config->base);
	return 0;
}

static uint32_t mcux_lpc_ctimer_get_top_value(const struct device *dev)
{
	const struct mcux_lpc_ctimer_config *config = dev->config;

#ifdef CONFIG_COUNTER_MCUX_CTIMER_RESERVE_CHANNEL_FOR_SETTOP
	CTIMER_Type *base = config->base;

	/* Return the top value if it has been set, else return the max top value */
	if (base->MR[NUM_CHANNELS] != 0) {
		return base->MR[NUM_CHANNELS];
	} else {
		return config->info.max_top_value;
	}
#else
	return config->info.max_top_value;
#endif
}

static int mcux_lpc_ctimer_set_alarm(const struct device *dev, uint8_t chan_id,
				     const struct counter_alarm_cfg *alarm_cfg)
{
	const struct mcux_lpc_ctimer_config *config = dev->config;
	struct mcux_lpc_ctimer_data *data = dev->data;
	uint32_t ticks = alarm_cfg->ticks;
	uint32_t current = mcux_lpc_ctimer_read(config->base);
	uint32_t top = mcux_lpc_ctimer_get_top_value(dev);

	if (alarm_cfg->ticks > top) {
		return -EINVAL;
	}

	if (data->channels[chan_id].alarm_callback != NULL) {
		LOG_ERR("channel already in use");
		return -EBUSY;
	}

	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) == 0) {
		ticks += current;
		if (ticks > top) {
			ticks %= top;
		}
	}

	data->channels[chan_id].alarm_callback = alarm_cfg->callback;
	data->channels[chan_id].alarm_user_data = alarm_cfg->user_data;

	ctimer_match_config_t match_config = { .matchValue = ticks,
					       .enableCounterReset = false,
					       .enableCounterStop = false,
					       .outControl = kCTIMER_Output_NoAction,
					       .outPinInitState = false,
					       .enableInterrupt = true };

	CTIMER_SetupMatch(config->base, chan_id, &match_config);

	return 0;
}

static int mcux_lpc_ctimer_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct mcux_lpc_ctimer_config *config = dev->config;
	struct mcux_lpc_ctimer_data *data = dev->data;

	CTIMER_DisableInterrupts(config->base, (1 << chan_id));

	data->channels[chan_id].alarm_callback = NULL;
	data->channels[chan_id].alarm_user_data = NULL;

	return 0;
}

static int mcux_lpc_ctimer_set_top_value(const struct device *dev,
					 const struct counter_top_cfg *cfg)
{
	const struct mcux_lpc_ctimer_config *config = dev->config;
	struct mcux_lpc_ctimer_data *data = dev->data;

#ifndef CONFIG_COUNTER_MCUX_CTIMER_RESERVE_CHANNEL_FOR_SETTOP
	/* Only allow max value when we do not reserve a ctimer channel for setting top value */
	if (cfg->ticks != config->info.max_top_value) {
		LOG_ERR("Wrap can only be set to 0x%x",
			config->info.max_top_value);
		return -ENOTSUP;
	}
#endif

	data->top_callback = cfg->callback;
	data->top_user_data = cfg->user_data;

	if (!(cfg->flags & COUNTER_TOP_CFG_DONT_RESET)) {
		CTIMER_Reset(config->base);
	} else if (mcux_lpc_ctimer_read(config->base) >= cfg->ticks) {
		if (cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) {
			CTIMER_Reset(config->base);
		}
		return -ETIME;
	}

#ifdef CONFIG_COUNTER_MCUX_CTIMER_RESERVE_CHANNEL_FOR_SETTOP
	ctimer_match_config_t match_config = { .matchValue = cfg->ticks,
					       .enableCounterReset = true,
					       .enableCounterStop = false,
					       .outControl = kCTIMER_Output_NoAction,
					       .outPinInitState = false,
					       .enableInterrupt = true };

	CTIMER_SetupMatch(config->base, NUM_CHANNELS, &match_config);
#endif

	return 0;
}

static uint32_t mcux_lpc_ctimer_get_pending_int(const struct device *dev)
{
	const struct mcux_lpc_ctimer_config *config = dev->config;

	return (CTIMER_GetStatusFlags(config->base) & 0xF) != 0;
}

static uint32_t mcux_lpc_ctimer_get_freq(const struct device *dev)
{
	/*
	 * The frequency of the timer is not known at compile time so we need to
	 * calculate at runtime when the frequency is known.
	 */
	const struct mcux_lpc_ctimer_config *config = dev->config;

	uint32_t clk_freq = 0;

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
					&clk_freq)) {
		LOG_ERR("unable to get clock frequency");
		return 0;
	}

	/* prescale increments when the prescale counter is 0 so if prescale is 1
	 * the counter is incremented every 2 cycles of the clock so will actually
	 * divide by 2 hence the addition of 1 to the value here.
	 */
	return (clk_freq / (config->prescale + 1));
}

static void mcux_lpc_ctimer_isr(const struct device *dev)
{
	const struct mcux_lpc_ctimer_config *config = dev->config;
	struct mcux_lpc_ctimer_data *data = dev->data;

	uint32_t interrupt_stat = CTIMER_GetStatusFlags(config->base);

	CTIMER_ClearStatusFlags(config->base, interrupt_stat);

	uint32_t ticks = mcux_lpc_ctimer_read(config->base);

	for (uint8_t chan = 0; chan < NUM_CHANNELS; chan++) {
		uint8_t channel_mask = 0x01 << chan;

		if (((interrupt_stat & channel_mask) != 0) &&
		    (data->channels[chan].alarm_callback != NULL)) {
			counter_alarm_callback_t alarm_callback =
				data->channels[chan].alarm_callback;
			void *alarm_user_data = data->channels[chan].alarm_user_data;

			data->channels[chan].alarm_callback = NULL;
			data->channels[chan].alarm_user_data = NULL;
			alarm_callback(dev, chan, ticks, alarm_user_data);
		}
	}

#ifdef CONFIG_COUNTER_MCUX_CTIMER_RESERVE_CHANNEL_FOR_SETTOP
	if (((interrupt_stat & (0x01 << NUM_CHANNELS)) != 0) && data->top_callback) {
		data->top_callback(dev, data->top_user_data);
	}
#endif
}

static int mcux_lpc_ctimer_init(const struct device *dev)
{
	const struct mcux_lpc_ctimer_config *config = dev->config;
	struct mcux_lpc_ctimer_data *data = dev->data;
	ctimer_config_t ctimer_config;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	for (uint8_t chan = 0; chan < NUM_CHANNELS; chan++) {
		data->channels[chan].alarm_callback = NULL;
		data->channels[chan].alarm_user_data = NULL;
	}

	CTIMER_GetDefaultConfig(&ctimer_config);

	ctimer_config.mode = config->mode;
	ctimer_config.input = config->input;
	ctimer_config.prescale = config->prescale;

	CTIMER_Init(config->base, &ctimer_config);

	config->irq_config_func(dev);

	return 0;
}

static const struct counter_driver_api mcux_ctimer_driver_api = {
	.start = mcux_lpc_ctimer_start,
	.stop = mcux_lpc_ctimer_stop,
	.get_value = mcux_lpc_ctimer_get_value,
	.set_alarm = mcux_lpc_ctimer_set_alarm,
	.cancel_alarm = mcux_lpc_ctimer_cancel_alarm,
	.set_top_value = mcux_lpc_ctimer_set_top_value,
	.get_pending_int = mcux_lpc_ctimer_get_pending_int,
	.get_top_value = mcux_lpc_ctimer_get_top_value,
	.get_freq = mcux_lpc_ctimer_get_freq,
};

#define COUNTER_LPC_CTIMER_DEVICE(id)                                                              \
	static void mcux_lpc_ctimer_irq_config_##id(const struct device *dev);                     \
	static struct mcux_lpc_ctimer_config mcux_lpc_ctimer_config_##id = { \
		.info = {						\
			.max_top_value = UINT32_MAX,			\
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,		\
			.channels = NUM_CHANNELS,					\
		},\
		.base = (CTIMER_Type *)DT_INST_REG_ADDR(id),		\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(id)),	\
		.clock_subsys =				\
		(clock_control_subsys_t)(DT_INST_CLOCKS_CELL(id, name) + MCUX_CTIMER_CLK_OFFSET),\
		.mode = DT_INST_PROP(id, mode),						\
		.input = DT_INST_PROP(id, input),					\
		.prescale = DT_INST_PROP(id, prescale),				\
		.irq_config_func = mcux_lpc_ctimer_irq_config_##id,	\
	};                     \
	static struct mcux_lpc_ctimer_data mcux_lpc_ctimer_data_##id;                              \
	DEVICE_DT_INST_DEFINE(id, &mcux_lpc_ctimer_init, NULL, &mcux_lpc_ctimer_data_##id,         \
			      &mcux_lpc_ctimer_config_##id, POST_KERNEL,                           \
			      CONFIG_COUNTER_INIT_PRIORITY, &mcux_ctimer_driver_api);              \
	static void mcux_lpc_ctimer_irq_config_##id(const struct device *dev)                      \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(id), DT_INST_IRQ(id, priority), mcux_lpc_ctimer_isr,      \
			    DEVICE_DT_INST_GET(id), 0);                                            \
		irq_enable(DT_INST_IRQN(id));                                                      \
	}

DT_INST_FOREACH_STATUS_OKAY(COUNTER_LPC_CTIMER_DEVICE)
