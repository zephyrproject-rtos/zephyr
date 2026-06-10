/*
 * (c) Meta Platforms, Inc. and affiliates.
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_ctimer_pwm

#include <errno.h>
#include <fsl_ctimer.h>
#include <fsl_clock.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/mcux_lpc_syscon_clock.h>
#ifdef CONFIG_PWM_CAPTURE
#include <zephyr/irq.h>
#include <fsl_inputmux.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pwm_mcux_ctimer, CONFIG_PWM_LOG_LEVEL);

#define CHANNEL_COUNT kCTIMER_Match_3 + 1

enum pwm_ctimer_channel_role {
	PWM_CTIMER_CHANNEL_ROLE_NONE = 0,
	PWM_CTIMER_CHANNEL_ROLE_PULSE,
	PWM_CTIMER_CHANNEL_ROLE_PERIOD,
};

struct pwm_ctimer_channel_state {
	enum pwm_ctimer_channel_role role;
	uint32_t cycles;
};

#ifdef CONFIG_PWM_CAPTURE
struct pwm_ctimer_capture_data {
	pwm_capture_callback_handler_t callback;
	void *user_data;
	uint32_t capture_channel;
	bool pulse_capture;
	bool inverted;
	bool continuous;
	bool active;
	bool first_edge_seen;
};

struct pwm_ctimer_inputmux_entry {
	INPUTMUX_Type *base;
	uint16_t channel;
	uint32_t connection;
};
#endif /* CONFIG_PWM_CAPTURE */

struct pwm_mcux_ctimer_data {
	struct pwm_ctimer_channel_state channel_states[CHANNEL_COUNT];
	ctimer_match_t current_period_channel;
	bool is_period_channel_set;
	uint32_t num_active_pulse_chans;
#ifdef CONFIG_PWM_CAPTURE
	struct pwm_ctimer_capture_data capture;
#endif
};

struct pwm_mcux_ctimer_config {
	CTIMER_Type *base;
	uint32_t prescale;
	uint32_t period_channel;
	const struct device *clock_control;
	clock_control_subsys_t clock_id;
	const struct pinctrl_dev_config *pincfg;
#ifdef CONFIG_PWM_CAPTURE
	const struct pwm_ctimer_inputmux_entry *inputmux_entries;
	uint8_t inputmux_entries_count;
	void (*irq_config_func)(const struct device *dev);
#endif
};

/*
 * All pwm signals generated from the same ctimer must have same period. To avoid this, we check
 * if the new parameters will NOT change the period for a ctimer with active pulse channels
 */
static bool mcux_ctimer_pwm_is_period_valid(struct pwm_mcux_ctimer_data *data,
					    uint32_t new_pulse_channel, uint32_t new_period_cycles,
					    uint32_t current_period_channel)
{
	/* if we aren't changing the period, we're ok */
	if (data->channel_states[current_period_channel].cycles == new_period_cycles) {
		return true;
	}

	/*
	 * if we are changing it but there aren't any pulse channels that depend on it, then we're
	 * ok too
	 */
	if (data->num_active_pulse_chans == 0) {
		return true;
	}

	if (data->num_active_pulse_chans > 1) {
		return false;
	}

	/*
	 * there is exactly one pulse channel that depends on existing period and its not the
	 * one we're changing now
	 */
	if (data->channel_states[new_pulse_channel].role != PWM_CTIMER_CHANNEL_ROLE_PULSE) {
		return false;
	}

	return true;
}

/*
 * Each ctimer channel can either be used as a pulse or period channel.  Each channel has a counter.
 * The duty cycle is counted by the pulse channel. When the period channel counts down, it resets
 * the pulse channel (and all counters in the ctimer instance).  The pwm api does not permit us to
 * specify a period channel (only pulse channel). So we need to figure out an acceptable period
 * channel in the driver (if that's even possible)
 */
static int mcux_ctimer_pwm_select_period_channel(struct pwm_mcux_ctimer_data *data,
						 uint32_t new_pulse_channel,
						 uint32_t new_period_cycles,
						 uint32_t *ret_period_channel)
{
	if (data->is_period_channel_set) {
		if (!mcux_ctimer_pwm_is_period_valid(data, new_pulse_channel, new_period_cycles,
						     data->current_period_channel)) {
			LOG_ERR("Cannot set channel %u to %u as period channel",
				*ret_period_channel, new_period_cycles);
			return -EINVAL;
		}

		*ret_period_channel = data->current_period_channel;
		if (new_pulse_channel != *ret_period_channel) {
			/* the existing period channel will not conflict with new pulse_channel */
			return 0;
		}
	}

	/* we need to find an unused channel to use as period_channel */
	*ret_period_channel = new_pulse_channel + 1;
	*ret_period_channel %= CHANNEL_COUNT;
	while (data->channel_states[*ret_period_channel].role != PWM_CTIMER_CHANNEL_ROLE_NONE) {
		if (new_pulse_channel == *ret_period_channel) {
			LOG_ERR("no available channel for period counter");
			return -EBUSY;
		}
		(*ret_period_channel)++;
		*ret_period_channel %= CHANNEL_COUNT;
	}

	return 0;
}

static void mcux_ctimer_pwm_update_state(struct pwm_mcux_ctimer_data *data, uint32_t pulse_channel,
					 uint32_t pulse_cycles, uint32_t period_channel,
					 uint32_t period_cycles)
{
	if (data->channel_states[pulse_channel].role != PWM_CTIMER_CHANNEL_ROLE_PULSE) {
		data->num_active_pulse_chans++;
	}

	data->channel_states[pulse_channel].role = PWM_CTIMER_CHANNEL_ROLE_PULSE;
	data->channel_states[pulse_channel].cycles = pulse_cycles;

	data->is_period_channel_set = true;
	data->current_period_channel = period_channel;
	data->channel_states[period_channel].role = PWM_CTIMER_CHANNEL_ROLE_PERIOD;
	data->channel_states[period_channel].cycles = period_cycles;
}

static int mcux_ctimer_pwm_set_cycles(const struct device *dev, uint32_t pulse_channel,
				      uint32_t period_cycles, uint32_t pulse_cycles,
				      pwm_flags_t flags)
{
	const struct pwm_mcux_ctimer_config *config = dev->config;
	struct pwm_mcux_ctimer_data *data = dev->data;
	uint32_t period_channel = data->current_period_channel;
	int ret = 0;
	status_t status;

	if (pulse_channel >= CHANNEL_COUNT) {
		LOG_ERR("Invalid channel %u. muse be less than %u", pulse_channel, CHANNEL_COUNT);
		return -EINVAL;
	}

	if (period_cycles == 0) {
		LOG_ERR("Channel can not be set to zero");
		return -ENOTSUP;
	}

#ifdef CONFIG_PWM_CAPTURE
	if (data->capture.active) {
		LOG_ERR("Cannot set PWM cycles while capture is active");
		return -EBUSY;
	}
#endif

	ret = mcux_ctimer_pwm_select_period_channel(data, pulse_channel, period_cycles,
						    &period_channel);
	if (ret != 0) {
		LOG_ERR("could not select valid period channel. ret=%d", ret);
		return ret;
	}

	/*
	 * CTIMER PWM-mode output: after each TC reset the pin starts LOW; it
	 * transitions HIGH when TC matches MR[pulse_channel] and stays HIGH until
	 * the next TC reset (MR[period_channel] match). So in hardware:
	 *     LOW  duration = MR[pulse_channel]
	 *     HIGH duration = period - MR[pulse_channel]
	 * Boundary handling (symmetric across polarity):
	 *   - MR > period  → match never fires → pin stays LOW the whole period
	 *   - MR == 0      → match fires at TC=0 → pin stays HIGH (approx) the whole period
	 */
	if (flags & PWM_POLARITY_INVERTED) {
		if (pulse_cycles == period_cycles) {
			/* always active(LOW): keep pin LOW the entire period */
			pulse_cycles = period_cycles + 1;
		}
		/* else: pulse_cycles passes through unchanged as MR[pulse] */
	} else {
		if (pulse_cycles == 0) {
			/* always inactive(LOW): keep pin LOW the entire period */
			pulse_cycles = period_cycles + 1;
		} else {
			pulse_cycles = period_cycles - pulse_cycles;
		}
	}

	status = CTIMER_SetupPwmPeriod(config->base, period_channel, pulse_channel, period_cycles,
				       pulse_cycles, false);
	if (kStatus_Success != status) {
		LOG_ERR("failed setup pwm period. status=%d", status);
		return -EIO;
	}
	mcux_ctimer_pwm_update_state(data, pulse_channel, pulse_cycles, period_channel,
				     period_cycles);

	CTIMER_StartTimer(config->base);
	return 0;
}

static int mcux_ctimer_pwm_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					      uint64_t *cycles)
{
	const struct pwm_mcux_ctimer_config *config = dev->config;
	int err = 0;


	/* clean up upper word of return parameter */
	*cycles &= 0xFFFFFFFF;

	err = clock_control_get_rate(config->clock_control, config->clock_id, (uint32_t *)cycles);
	if (err != 0) {
		LOG_ERR("could not get clock rate");
		return err;
	}

	*cycles /= (uint64_t)config->prescale + 1;
	__ASSERT((*cycles) > 0, "Invalid PWM frequency: cycles per second is 0(check clock rate "
				"and prescaler)");

	return err;
}

#ifdef CONFIG_PWM_CAPTURE

static void mcux_ctimer_pwm_setup_inputmux(const struct pwm_mcux_ctimer_config *config)
{
	for (uint8_t i = 0; i < config->inputmux_entries_count; i++) {
		const struct pwm_ctimer_inputmux_entry *entry = &config->inputmux_entries[i];

		INPUTMUX_Init(entry->base);
		INPUTMUX_AttachSignal(entry->base, entry->channel,
				      (inputmux_connection_t)entry->connection);
		INPUTMUX_Deinit(entry->base);
	}
}

static int mcux_ctimer_pwm_configure_capture(const struct device *dev, uint32_t channel,
					     pwm_flags_t flags,
					     pwm_capture_callback_handler_t cb, void *user_data)
{
	struct pwm_mcux_ctimer_data *data = dev->data;
	struct pwm_ctimer_capture_data *capture = &data->capture;

	if (channel >= CHANNEL_COUNT) {
		LOG_ERR("invalid capture channel %u", channel);
		return -EINVAL;
	}

	if ((flags & PWM_CAPTURE_TYPE_MASK) == 0U) {
		LOG_ERR("no capture type specified");
		return -EINVAL;
	}

	if ((flags & PWM_CAPTURE_TYPE_MASK) == PWM_CAPTURE_TYPE_BOTH) {
		LOG_ERR("simultaneous period and pulse capture not supported");
		return -ENOTSUP;
	}

	if (cb == NULL) {
		LOG_ERR("PWM capture callback is not configured");
		return -EINVAL;
	}

	if (capture->active) {
		LOG_ERR("capture already active on this ctimer");
		return -EBUSY;
	}

	if (data->num_active_pulse_chans != 0U) {
		LOG_ERR("cannot capture while PWM output active on same ctimer");
		return -EBUSY;
	}

	capture->callback = cb;
	capture->user_data = user_data;
	capture->capture_channel = channel;
	capture->pulse_capture = ((flags & PWM_CAPTURE_TYPE_MASK) == PWM_CAPTURE_TYPE_PULSE);
	capture->inverted = ((flags & PWM_POLARITY_MASK) == PWM_POLARITY_INVERTED);
	capture->continuous = ((flags & PWM_CAPTURE_MODE_MASK) == PWM_CAPTURE_MODE_CONTINUOUS);
	capture->first_edge_seen = false;

	return 0;
}

static int mcux_ctimer_pwm_enable_capture(const struct device *dev, uint32_t channel)
{
	const struct pwm_mcux_ctimer_config *config = dev->config;
	struct pwm_mcux_ctimer_data *data = dev->data;
	struct pwm_ctimer_capture_data *capture = &data->capture;
	CTIMER_Type *base = config->base;
	ctimer_capture_channel_t ch;
	ctimer_capture_edge_t irq_edge;
	uint32_t selcc_value;
	bool clear_on_rising;

	if (channel >= CHANNEL_COUNT) {
		return -EINVAL;
	}

	if (capture->callback == NULL) {
		LOG_ERR("capture not configured");
		return -EINVAL;
	}

	if (capture->active) {
		LOG_ERR("capture already active");
		return -EBUSY;
	}

	if (channel != capture->capture_channel) {
		LOG_ERR("channel %u does not match configured channel %u", channel,
			capture->capture_channel);
		return -EINVAL;
	}

	ch = (ctimer_capture_channel_t)capture->capture_channel;
	clear_on_rising = !capture->inverted;
	if (capture->pulse_capture) {
		irq_edge = capture->inverted ? kCTIMER_Capture_RiseEdge : kCTIMER_Capture_FallEdge;
	} else {
		irq_edge = capture->inverted ? kCTIMER_Capture_FallEdge : kCTIMER_Capture_RiseEdge;
	}

	/* stop timer and reset TC before arming so the first edge starts fresh */
	CTIMER_StopTimer(base);
	CTIMER_Reset(base);
	CTIMER_ClearStatusFlags(base, CTIMER_IR_CR0INT_MASK << capture->capture_channel);

	capture->first_edge_seen = false;
	capture->active = true;

	/* CCR: select capture edge for this channel, enable capture interrupt */
	CTIMER_SetupCapture(base, ch, irq_edge, true);

	/*
	 * Enable the ctimer "capture clears timer" feature: the chosen edge on
	 * the selected capture channel automatically resets TC and PC in
	 * hardware. The next capture event therefore latches CR equal to the
	 * time elapsed since that reset edge, i.e. one period or one pulse
	 * width directly, with no software subtraction and no TC wrap-around
	 * handling.
	 */
	selcc_value = ((uint32_t)ch * 2U) + (clear_on_rising ? 0U : 1U);
	base->CTCR = (base->CTCR & ~(CTIMER_CTCR_SELCC_MASK | CTIMER_CTCR_ENCC_MASK)) |
		     CTIMER_CTCR_ENCC_MASK | CTIMER_CTCR_SELCC(selcc_value);

	CTIMER_StartTimer(base);

	return 0;
}

static int mcux_ctimer_pwm_disable_capture(const struct device *dev, uint32_t channel)
{
	const struct pwm_mcux_ctimer_config *config = dev->config;
	struct pwm_mcux_ctimer_data *data = dev->data;
	struct pwm_ctimer_capture_data *capture = &data->capture;

	if (channel >= CHANNEL_COUNT) {
		return -EINVAL;
	}

	if (!capture->active) {
		return 0;
	}

	if (channel != capture->capture_channel) {
		LOG_ERR("channel %u does not match configured channel %u",
			channel, capture->capture_channel);
		return -EINVAL;
	}

	/*
	 * Tear down the capture channel: disable the capture interrupt, clear
	 * both rising/falling edge enables, and turn off the capture-clears-TC
	 * feature in CTCR.
	 */
	ctimer_capture_channel_t ch = (ctimer_capture_channel_t)capture->capture_channel;

	CTIMER_DisableInterrupts(config->base, CTIMER_CCR_CAP0I_MASK << ((uint32_t)ch * 3U));
	CTIMER_EnableRisingEdgeCapture(config->base, ch, false);
	CTIMER_EnableFallingEdgeCapture(config->base, ch, false);
	config->base->CTCR &= ~(CTIMER_CTCR_ENCC_MASK | CTIMER_CTCR_SELCC_MASK);

	CTIMER_StopTimer(config->base);
	CTIMER_ClearStatusFlags(config->base, CTIMER_IR_CR0INT_MASK << capture->capture_channel);

	capture->active = false;
	capture->first_edge_seen = false;

	return 0;
}

static void mcux_ctimer_pwm_isr(const struct device *dev)
{
	const struct pwm_mcux_ctimer_config *config = dev->config;
	struct pwm_mcux_ctimer_data *data = dev->data;
	struct pwm_ctimer_capture_data *capture = &data->capture;
	uint32_t cap_flag;
	uint32_t cycles;

	if (!capture->active) {
		CTIMER_ClearStatusFlags(config->base, CTIMER_IR_CR0INT_MASK |
			CTIMER_IR_CR1INT_MASK | CTIMER_IR_CR2INT_MASK | CTIMER_IR_CR3INT_MASK);

		return;
	}

	cap_flag = CTIMER_IR_CR0INT_MASK << capture->capture_channel;
	if ((CTIMER_GetStatusFlags(config->base) & cap_flag) == 0U) {
		return;
	}

	cycles = config->base->CR[capture->capture_channel];
	CTIMER_ClearStatusFlags(config->base, cap_flag);

	if (!capture->first_edge_seen) {
		/*
		 * First edge after enable: TC was started at an unknown point in the
		 * input waveform, so the captured value is not meaningful. Skip it and
		 * use subsequent edges (TC has now been cleared by hardware on the
		 * configured edge, so the next capture is referenced from that edge).
		 */
		capture->first_edge_seen = true;
		return;
	}

	if (!capture->continuous) {
		mcux_ctimer_pwm_disable_capture(dev, capture->capture_channel);
	}

	if (capture->pulse_capture) {
		capture->callback(dev, capture->capture_channel, 0U, cycles, 0,
				  capture->user_data);
	} else {
		capture->callback(dev, capture->capture_channel, cycles, 0U, 0,
				  capture->user_data);
	}
}
#endif /* CONFIG_PWM_CAPTURE */

static int mcux_ctimer_pwm_init(const struct device *dev)
{
	const struct pwm_mcux_ctimer_config *config = dev->config;
	ctimer_config_t pwm_config;
	int err;

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	if (config->period_channel >= CHANNEL_COUNT) {
		LOG_ERR("invalid period_channel: %d. must be less than %d", config->period_channel,
			CHANNEL_COUNT);
		return -EINVAL;
	}

	CTIMER_GetDefaultConfig(&pwm_config);
	pwm_config.prescale = config->prescale;

	CTIMER_Init(config->base, &pwm_config);

#ifdef CONFIG_PWM_CAPTURE
	mcux_ctimer_pwm_setup_inputmux(config);
	if (config->irq_config_func != NULL) {
		config->irq_config_func(dev);
	}
#endif

	return 0;
}

static DEVICE_API(pwm, pwm_mcux_ctimer_driver_api) = {
	.set_cycles = mcux_ctimer_pwm_set_cycles,
	.get_cycles_per_sec = mcux_ctimer_pwm_get_cycles_per_sec,
#ifdef CONFIG_PWM_CAPTURE
	.configure_capture = mcux_ctimer_pwm_configure_capture,
	.enable_capture = mcux_ctimer_pwm_enable_capture,
	.disable_capture = mcux_ctimer_pwm_disable_capture,
#endif
};

#define PWM_MCUX_CTIMER_PINCTRL_DEFINE(n) PINCTRL_DT_INST_DEFINE(n);
#define PWM_MCUX_CTIMER_PINCTRL_INIT(n)   .pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),

#ifdef CONFIG_PWM_CAPTURE
#define PWM_MCUX_CTIMER_IRQ_FUNC_DECL(n)                                                           \
	static void pwm_mcux_ctimer_irq_config_##n(const struct device *dev);
#define PWM_MCUX_CTIMER_IRQ_FUNC_INIT(n) .irq_config_func = pwm_mcux_ctimer_irq_config_##n,
#define PWM_MCUX_CTIMER_IRQ_FUNC_DEFINE(n)                                                         \
	static void pwm_mcux_ctimer_irq_config_##n(const struct device *dev)                       \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), mcux_ctimer_pwm_isr,        \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}
#define PWM_MCUX_CTIMER_INPUTMUX_ENTRY(node_id, prop, idx)                                         \
	{                                                                                          \
		.base = (INPUTMUX_Type *)DT_REG_ADDR(DT_PHANDLE_BY_IDX(node_id, prop, idx)),       \
		.channel = (uint16_t)DT_PHA_BY_IDX(node_id, prop, idx, channel),                   \
		.connection = (uint32_t)DT_PHA_BY_IDX(node_id, prop, idx, connection),             \
	}
#define PWM_MCUX_CTIMER_INPUTMUX_DEFINE(n)                                                         \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, inputmux_connections),                                \
		    (static const struct pwm_ctimer_inputmux_entry                                 \
			    pwm_mcux_ctimer_inputmux_entries_##n[] = {                             \
			    DT_INST_FOREACH_PROP_ELEM_SEP(n, inputmux_connections,                 \
							  PWM_MCUX_CTIMER_INPUTMUX_ENTRY, (,))     \
		    };), ())
#define PWM_MCUX_CTIMER_INPUTMUX_INIT(n)                                                           \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, inputmux_connections),                                \
		    (.inputmux_entries = pwm_mcux_ctimer_inputmux_entries_##n,                     \
		     .inputmux_entries_count = DT_INST_PROP_LEN(n, inputmux_connections),), ())
#else
#define PWM_MCUX_CTIMER_IRQ_FUNC_DECL(n)
#define PWM_MCUX_CTIMER_IRQ_FUNC_INIT(n)
#define PWM_MCUX_CTIMER_IRQ_FUNC_DEFINE(n)
#define PWM_MCUX_CTIMER_INPUTMUX_DEFINE(n)
#define PWM_MCUX_CTIMER_INPUTMUX_INIT(n)
#endif

#define PWM_MCUX_CTIMER_DEVICE_INIT_MCUX(n)                                                        \
	static struct pwm_mcux_ctimer_data pwm_mcux_ctimer_data_##n = {                            \
		.channel_states =                                                                  \
			{                                                                          \
				[kCTIMER_Match_0] = {.role = PWM_CTIMER_CHANNEL_ROLE_NONE,         \
						     .cycles = 0},                                 \
				[kCTIMER_Match_1] = {.role = PWM_CTIMER_CHANNEL_ROLE_NONE,         \
						     .cycles = 0},                                 \
				[kCTIMER_Match_2] = {.role = PWM_CTIMER_CHANNEL_ROLE_NONE,         \
						     .cycles = 0},                                 \
				[kCTIMER_Match_3] = {.role = PWM_CTIMER_CHANNEL_ROLE_NONE,         \
						     .cycles = 0},                                 \
			},                                                                         \
		.current_period_channel = kCTIMER_Match_0,                                         \
		.is_period_channel_set = false,                                                    \
	};                                                                                         \
	PWM_MCUX_CTIMER_PINCTRL_DEFINE(n)                                                          \
	PWM_MCUX_CTIMER_IRQ_FUNC_DECL(n)                                                           \
	PWM_MCUX_CTIMER_INPUTMUX_DEFINE(n)                                                         \
	static const struct pwm_mcux_ctimer_config pwm_mcux_ctimer_config_##n = {                  \
		.base = (CTIMER_Type *)DT_INST_REG_ADDR(n),                                        \
		.prescale = DT_INST_PROP(n, prescaler),                                            \
		.clock_control = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                            \
		.clock_id = (clock_control_subsys_t)(DT_INST_CLOCKS_CELL(n, name)),                \
		PWM_MCUX_CTIMER_PINCTRL_INIT(n)                                                    \
		PWM_MCUX_CTIMER_INPUTMUX_INIT(n)                                                   \
		PWM_MCUX_CTIMER_IRQ_FUNC_INIT(n)                                                   \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, mcux_ctimer_pwm_init, NULL, &pwm_mcux_ctimer_data_##n,            \
			      &pwm_mcux_ctimer_config_##n, POST_KERNEL,                            \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &pwm_mcux_ctimer_driver_api);    \
	PWM_MCUX_CTIMER_IRQ_FUNC_DEFINE(n)

DT_INST_FOREACH_STATUS_OKAY(PWM_MCUX_CTIMER_DEVICE_INIT_MCUX)
