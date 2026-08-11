/*
 * Copyright (c) 2021, Toby Firth.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT nxp_lpc_ctimer

#include <zephyr/drivers/counter.h>
#include <fsl_ctimer.h>
#ifdef CONFIG_COUNTER_CAPTURE
#include <fsl_inputmux.h>
#include <zephyr/drivers/pinctrl.h>
#endif /* CONFIG_COUNTER_CAPTURE */
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/mcux_lpc_syscon_clock.h>
#include <zephyr/irq.h>
#include <zephyr/pm/device.h>

LOG_MODULE_REGISTER(mcux_ctimer, CONFIG_COUNTER_LOG_LEVEL);

#ifdef CONFIG_COUNTER_MCUX_CTIMER_RESERVE_CHANNEL_FOR_SETTOP
/* One of the CTimer channels is reserved to implement set_top_value API */
#define NUM_CHANNELS 3
#else
#define NUM_CHANNELS 4
#endif

#ifdef CONFIG_COUNTER_CAPTURE
#define CTIMER_CAPTURE_INT_MASK(chan)    (CTIMER_CCR_CAP0I_MASK << ((uint32_t)(chan) * 3U))
#define CTIMER_CAPTURE_STATUS_MASK(chan) (CTIMER_IR_CR0INT_MASK << (uint32_t)(chan))
#define CTIMER_CAPTURE_VALID_FLAGS       (COUNTER_CAPTURE_BOTH_EDGES | COUNTER_CAPTURE_SINGLE_SHOT)
#endif /* CONFIG_COUNTER_CAPTURE */

struct mcux_lpc_ctimer_channel_data {
	counter_alarm_callback_t alarm_callback;
	void *alarm_user_data;
#ifdef CONFIG_COUNTER_CAPTURE
	counter_capture_cb_t capture_callback;
	void *capture_user_data;
	ctimer_capture_edge_t capture_edge;
	counter_capture_flags_t capture_flags;
	bool capture_single_shot;
#endif /* CONFIG_COUNTER_CAPTURE */
};

struct mcux_lpc_ctimer_data {
	struct mcux_lpc_ctimer_channel_data channels[NUM_CHANNELS];
	counter_top_callback_t top_callback;
	void *top_user_data;
};

#ifdef CONFIG_COUNTER_CAPTURE
struct mcux_lpc_ctimer_inputmux_entry {
	INPUTMUX_Type *base;
	uint16_t channel;
	uint32_t connection;
};
#endif /* CONFIG_COUNTER_CAPTURE */

struct mcux_lpc_ctimer_config {
	struct counter_config_info info;
	CTIMER_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	ctimer_timer_mode_t mode;
	ctimer_capture_channel_t input;
	uint32_t prescale;
	void (*irq_config_func)(const struct device *dev);
#ifdef CONFIG_COUNTER_CAPTURE
	const struct pinctrl_dev_config *pincfg;
	const struct mcux_lpc_ctimer_inputmux_entry *inputmux_entries;
	uint8_t inputmux_entries_count;
#endif /* CONFIG_COUNTER_CAPTURE */
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

#ifdef CONFIG_COUNTER_CAPTURE
	if (data->channels[chan_id].capture_callback != NULL) {
		LOG_ERR("channel already configured for capture");
		return -EBUSY;
	}
#endif /* CONFIG_COUNTER_CAPTURE */

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

static int mcux_lpc_ctimer_reset(const struct device *dev)
{
	const struct mcux_lpc_ctimer_config *config = dev->config;

	CTIMER_Reset(config->base);

	return 0;
}

static uint32_t mcux_lpc_ctimer_get_pending_int(const struct device *dev)
{
	const struct mcux_lpc_ctimer_config *config = dev->config;
	uint32_t mask = 0x0FU;

#ifdef CONFIG_COUNTER_CAPTURE
	mask |= 0xF0U;
#endif /* CONFIG_COUNTER_CAPTURE */

	return (CTIMER_GetStatusFlags(config->base) & mask) != 0;
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

#ifdef CONFIG_COUNTER_CAPTURE
static void mcux_lpc_ctimer_setup_inputmux(const struct mcux_lpc_ctimer_config *config)
{
	for (uint8_t i = 0; i < config->inputmux_entries_count; i++) {
		const struct mcux_lpc_ctimer_inputmux_entry *entry = &config->inputmux_entries[i];

		INPUTMUX_Init(entry->base);
		INPUTMUX_AttachSignal(entry->base, entry->channel,
				      (inputmux_connection_t)entry->connection);
		INPUTMUX_Deinit(entry->base);
	}
}

static int mcux_lpc_ctimer_capture_edge(counter_capture_flags_t flags,
						ctimer_capture_edge_t *edge)
{
	if ((flags & ~CTIMER_CAPTURE_VALID_FLAGS) != 0U) {
		return -EINVAL;
	}

	if ((flags & COUNTER_CAPTURE_BOTH_EDGES) == COUNTER_CAPTURE_BOTH_EDGES) {
		*edge = kCTIMER_Capture_BothEdge;
	} else if ((flags & COUNTER_CAPTURE_FALLING_EDGE) != 0U) {
		*edge = kCTIMER_Capture_FallEdge;
	} else if ((flags & COUNTER_CAPTURE_RISING_EDGE) != 0U) {
		*edge = kCTIMER_Capture_RiseEdge;
	} else {
		return -EINVAL;
	}

	return 0;
}

static int mcux_lpc_ctimer_capture_configure(const struct device *dev, uint8_t chan_id,
					     counter_capture_flags_t flags,
					     counter_capture_cb_t cb, void *user_data)
{
	const struct mcux_lpc_ctimer_config *config = dev->config;
	struct mcux_lpc_ctimer_data *data = dev->data;
	ctimer_capture_edge_t edge;
	int ret;

	if (chan_id >= NUM_CHANNELS) {
		return -EINVAL;
	}

	if (cb == NULL) {
		return -EINVAL;
	}

	if (data->channels[chan_id].alarm_callback != NULL) {
		LOG_ERR("channel %u already configured for alarm", chan_id);
		return -EBUSY;
	}

	if ((config->base->CCR & CTIMER_CAPTURE_INT_MASK(chan_id)) != 0U) {
		LOG_ERR("capture channel %u is enabled", chan_id);
		return -EBUSY;
	}

	ret = mcux_lpc_ctimer_capture_edge(flags, &edge);
	if (ret != 0) {
		return ret;
	}

	data->channels[chan_id].capture_callback = cb;
	data->channels[chan_id].capture_user_data = user_data;
	data->channels[chan_id].capture_edge = edge;
	data->channels[chan_id].capture_flags = flags & COUNTER_CAPTURE_BOTH_EDGES;
	data->channels[chan_id].capture_single_shot = (flags & COUNTER_CAPTURE_SINGLE_SHOT) != 0U;

	return 0;
}

static int mcux_lpc_ctimer_enable_capture(const struct device *dev, uint8_t chan_id)
{
	const struct mcux_lpc_ctimer_config *config = dev->config;
	struct mcux_lpc_ctimer_data *data = dev->data;
	struct mcux_lpc_ctimer_channel_data *channel;

	if (chan_id >= NUM_CHANNELS) {
		return -EINVAL;
	}

	channel = &data->channels[chan_id];
	if (channel->alarm_callback != NULL) {
		LOG_ERR("channel %u already configured for alarm", chan_id);
		return -EBUSY;
	}

	if (channel->capture_callback == NULL) {
		LOG_ERR("capture callback not configured for channel %u", chan_id);
		return -EINVAL;
	}

	if ((config->base->CCR & CTIMER_CAPTURE_INT_MASK(chan_id)) != 0U) {
		return -EBUSY;
	}

	CTIMER_ClearStatusFlags(config->base, CTIMER_CAPTURE_STATUS_MASK(chan_id));
	CTIMER_SetupCapture(config->base, (ctimer_capture_channel_t)chan_id,
			   channel->capture_edge, true);

	return 0;
}

static int mcux_lpc_ctimer_disable_capture(const struct device *dev, uint8_t chan_id)
{
	const struct mcux_lpc_ctimer_config *config = dev->config;
	struct mcux_lpc_ctimer_data *data = dev->data;
	ctimer_capture_channel_t channel;

	if (chan_id >= NUM_CHANNELS) {
		return -EINVAL;
	}

	channel = (ctimer_capture_channel_t)chan_id;
	CTIMER_DisableInterrupts(config->base, CTIMER_CAPTURE_INT_MASK(chan_id));
	CTIMER_EnableRisingEdgeCapture(config->base, channel, false);
	CTIMER_EnableFallingEdgeCapture(config->base, channel, false);
	CTIMER_ClearStatusFlags(config->base, CTIMER_CAPTURE_STATUS_MASK(chan_id));

	data->channels[chan_id].capture_callback = NULL;
	data->channels[chan_id].capture_user_data = NULL;
	data->channels[chan_id].capture_flags = 0U;
	data->channels[chan_id].capture_single_shot = false;

	return 0;
}
#endif /* CONFIG_COUNTER_CAPTURE */

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

#ifdef CONFIG_COUNTER_CAPTURE
	for (uint8_t chan = 0; chan < NUM_CHANNELS; chan++) {
		uint32_t capture_mask = CTIMER_CAPTURE_STATUS_MASK(chan);
		counter_capture_cb_t capture_callback;
		counter_capture_flags_t capture_flags;
		void *capture_user_data;
		uint32_t capture_ticks;

		if ((interrupt_stat & capture_mask) == 0U) {
			continue;
		}

		capture_callback = data->channels[chan].capture_callback;
		if (capture_callback == NULL) {
			continue;
		}

		capture_ticks = CTIMER_GetCaptureValue(config->base,
					       (ctimer_capture_channel_t)chan);
		capture_user_data = data->channels[chan].capture_user_data;
		capture_flags = data->channels[chan].capture_flags;

		if (data->channels[chan].capture_single_shot) {
			capture_flags |= COUNTER_CAPTURE_SINGLE_SHOT;
			(void)mcux_lpc_ctimer_disable_capture(dev, chan);
		} else {
			capture_flags |= COUNTER_CAPTURE_CONTINUOUS;
		}

		capture_callback(dev, chan, capture_flags, capture_ticks, capture_user_data);
	}
#endif /* CONFIG_COUNTER_CAPTURE */

#ifdef CONFIG_COUNTER_MCUX_CTIMER_RESERVE_CHANNEL_FOR_SETTOP
	if (((interrupt_stat & (0x01 << NUM_CHANNELS)) != 0) && data->top_callback) {
		data->top_callback(dev, data->top_user_data);
	}
#endif
}

static int mcux_lpc_ctimer_init_common(const struct device *dev)
{
	const struct mcux_lpc_ctimer_config *config = dev->config;
	struct mcux_lpc_ctimer_data *data = dev->data;
	ctimer_config_t ctimer_config;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

#ifdef CONFIG_COUNTER_CAPTURE
	int ret;

	if (config->pincfg != NULL) {
		ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
		if (ret != 0) {
			return ret;
		}
	}

	mcux_lpc_ctimer_setup_inputmux(config);
#endif /* CONFIG_COUNTER_CAPTURE */

	for (uint8_t chan = 0; chan < NUM_CHANNELS; chan++) {
		data->channels[chan].alarm_callback = NULL;
		data->channels[chan].alarm_user_data = NULL;
#ifdef CONFIG_COUNTER_CAPTURE
		data->channels[chan].capture_callback = NULL;
		data->channels[chan].capture_user_data = NULL;
		data->channels[chan].capture_flags = 0U;
		data->channels[chan].capture_single_shot = false;
#endif /* CONFIG_COUNTER_CAPTURE */
	}

	CTIMER_GetDefaultConfig(&ctimer_config);

	ctimer_config.mode = config->mode;
	ctimer_config.input = config->input;
	ctimer_config.prescale = config->prescale;

	CTIMER_Init(config->base, &ctimer_config);

	config->irq_config_func(dev);

	return 0;
}

static int mcux_lpc_ctimer_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		break;
	case PM_DEVICE_ACTION_TURN_ON:
		mcux_lpc_ctimer_init_common(dev);
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

static int mcux_lpc_ctimer_init(const struct device *dev)
{
	/* Rest of the init is done from the PM_DEVICE_TURN_ON action
	 * which is invoked by pm_device_driver_init().
	 */
	return pm_device_driver_init(dev, mcux_lpc_ctimer_pm_action);
}

static DEVICE_API(counter, mcux_ctimer_driver_api) = {
	.start = mcux_lpc_ctimer_start,
	.stop = mcux_lpc_ctimer_stop,
	.reset = mcux_lpc_ctimer_reset,
	.get_value = mcux_lpc_ctimer_get_value,
	.set_alarm = mcux_lpc_ctimer_set_alarm,
	.cancel_alarm = mcux_lpc_ctimer_cancel_alarm,
	.set_top_value = mcux_lpc_ctimer_set_top_value,
	.get_pending_int = mcux_lpc_ctimer_get_pending_int,
	.get_top_value = mcux_lpc_ctimer_get_top_value,
	.get_freq = mcux_lpc_ctimer_get_freq,
#ifdef CONFIG_COUNTER_CAPTURE
	.capture_configure = mcux_lpc_ctimer_capture_configure,
	.enable_capture = mcux_lpc_ctimer_enable_capture,
	.disable_capture = mcux_lpc_ctimer_disable_capture,
#endif /* CONFIG_COUNTER_CAPTURE */
};

#ifdef CONFIG_COUNTER_CAPTURE
#define COUNTER_LPC_CTIMER_PINCTRL_DEFINE(id) \
	IF_ENABLED(DT_INST_PINCTRL_HAS_NAME(id, default), (PINCTRL_DT_INST_DEFINE(id);))
#define COUNTER_LPC_CTIMER_PINCTRL_INIT(id) \
	.pincfg = COND_CODE_1(DT_INST_NODE_HAS_PROP(id, pinctrl_0), \
				 (PINCTRL_DT_INST_DEV_CONFIG_GET(id)), (NULL)),
#define COUNTER_LPC_CTIMER_INPUTMUX_ENTRY(node_id, prop, idx) \
	{ \
		.base = (INPUTMUX_Type *)DT_REG_ADDR(DT_PHANDLE_BY_IDX(node_id, prop, idx)), \
		.channel = (uint16_t)DT_PHA_BY_IDX(node_id, prop, idx, channel), \
		.connection = (uint32_t)DT_PHA_BY_IDX(node_id, prop, idx, connection), \
	}
#define COUNTER_LPC_CTIMER_INPUTMUX_DEFINE(id) \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(id, inputmux_connections), \
		    (static const struct mcux_lpc_ctimer_inputmux_entry \
			    mcux_lpc_ctimer_inputmux_entries_##id[] = { \
			    DT_INST_FOREACH_PROP_ELEM_SEP(id, inputmux_connections, \
							 COUNTER_LPC_CTIMER_INPUTMUX_ENTRY, (,)) \
		    };), ())
#define COUNTER_LPC_CTIMER_INPUTMUX_INIT(id) \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(id, inputmux_connections), \
		    (.inputmux_entries = mcux_lpc_ctimer_inputmux_entries_##id, \
		     .inputmux_entries_count = DT_INST_PROP_LEN(id, inputmux_connections),), ())
#else
#define COUNTER_LPC_CTIMER_PINCTRL_DEFINE(id)
#define COUNTER_LPC_CTIMER_PINCTRL_INIT(id)
#define COUNTER_LPC_CTIMER_INPUTMUX_DEFINE(id)
#define COUNTER_LPC_CTIMER_INPUTMUX_INIT(id)
#endif /* CONFIG_COUNTER_CAPTURE */

#define COUNTER_LPC_CTIMER_DEVICE(id) \
	COUNTER_LPC_CTIMER_PINCTRL_DEFINE(id) \
	COUNTER_LPC_CTIMER_INPUTMUX_DEFINE(id) \
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
		(clock_control_subsys_t)(DT_INST_CLOCKS_CELL(id, name)),\
		.mode = DT_INST_PROP(id, mode),						\
		.input = DT_INST_PROP(id, input),					\
		.prescale = DT_INST_PROP(id, prescale),				\
		COUNTER_LPC_CTIMER_PINCTRL_INIT(id) \
		COUNTER_LPC_CTIMER_INPUTMUX_INIT(id) \
		.irq_config_func = mcux_lpc_ctimer_irq_config_##id,	\
	};                     \
	PM_DEVICE_DT_INST_DEFINE(id, mcux_lpc_ctimer_pm_action);                                   \
	static struct mcux_lpc_ctimer_data mcux_lpc_ctimer_data_##id;                              \
	DEVICE_DT_INST_DEFINE(id, &mcux_lpc_ctimer_init, PM_DEVICE_DT_INST_GET(id),                \
			      &mcux_lpc_ctimer_data_##id,                                          \
			      &mcux_lpc_ctimer_config_##id, POST_KERNEL,                           \
			      CONFIG_COUNTER_INIT_PRIORITY, &mcux_ctimer_driver_api);              \
	static void mcux_lpc_ctimer_irq_config_##id(const struct device *dev)                      \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(id), DT_INST_IRQ(id, priority), mcux_lpc_ctimer_isr,      \
			    DEVICE_DT_INST_GET(id), 0);                                            \
		irq_enable(DT_INST_IRQN(id));                                                      \
	}

DT_INST_FOREACH_STATUS_OKAY(COUNTER_LPC_CTIMER_DEVICE)
