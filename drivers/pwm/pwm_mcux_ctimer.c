/*
 * (c) Meta Platforms, Inc. and affiliates.
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_ctimer_pwm

#include <errno.h>
#include <fsl_ctimer.h>
#include <fsl_inputmux.h>
#include <fsl_clock.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/mcux_lpc_syscon_clock.h>
#include <zephyr/irq.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pwm_mcux_ctimer, CONFIG_PWM_LOG_LEVEL);

#define MAX_MATCH_CHANNEL_COUNT (kCTIMER_Match_3 + 1)
#define MAX_CAPTURE_CHANNEL_COUNT (kCTIMER_Capture_3 + 1)

enum pwm_ctimer_channel_role {
	PWM_CTIMER_CHANNEL_ROLE_NONE = 0,
	PWM_CTIMER_CHANNEL_ROLE_PULSE,
	PWM_CTIMER_CHANNEL_ROLE_PERIOD,
	PWM_CTIMER_CHANNEL_ROLE_CAPTURE,
};

typedef struct pwm_ctimer_channel_state {
	enum pwm_ctimer_channel_role role;
	uint32_t cycles;
} pwm_ctimer_channel_state;

struct pwm_mcux_ctimer_data {
	uint8_t is_period_channel_set :1;

	uint8_t num_active_pulse_chans;
	ctimer_match_t current_period_channel;
	pwm_ctimer_channel_state channel_states[MAX_MATCH_CHANNEL_COUNT];

#ifdef CONFIG_PWM_CAPTURE
	ctimer_capture_channel_t operate_channel;
	ctimer_status_flags_t capture_status_flags;
	ctimer_interrupt_enable_t capture_interrupt_enable;

	void *user_data;
	inputmux_connection_t inputmux_connection[MAX_CAPTURE_CHANNEL_COUNT];

	pwm_capture_callback_handler_t capture_callback;
#endif /* CONFIG_PWM_CAPTURE */
};

struct pwm_mcux_ctimer_config {
	CTIMER_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct pinctrl_dev_config *pincfg;

	uint8_t period_channel;
	ctimer_timer_mode_t mode;
	uint32_t prescale;

#ifdef CONFIG_PWM_CAPTURE
	uint8_t capture_channel_enable :1;
	uint8_t inputmux;
	ctimer_capture_channel_t channel;
    ctimer_capture_edge_t capture_edge;
	void (*irq_config_func)(const struct device *dev);
#endif /* CONFIG_PWM_CAPTURE */
};

#ifdef CONFIG_PWM_CAPTURE
typedef struct user_data_t {
    // uint8_t capture_channel_enable :1;
	uint32_t unused;
} user_data_t;
#endif /* CONFIG_PWM_CAPTURE */

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
	*ret_period_channel %= MAX_MATCH_CHANNEL_COUNT;
	while (data->channel_states[*ret_period_channel].role != PWM_CTIMER_CHANNEL_ROLE_NONE) {
		if (new_pulse_channel == *ret_period_channel) {
			LOG_ERR("no available channel for period counter");
			return -EBUSY;
		}
		(*ret_period_channel)++;
		*ret_period_channel %= MAX_MATCH_CHANNEL_COUNT;
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

	if (pulse_channel >= MAX_MATCH_CHANNEL_COUNT) {
		LOG_ERR("Invalid channel %u. muse be less than %u", pulse_channel, MAX_MATCH_CHANNEL_COUNT);
		return -EINVAL;
	}

	if (period_cycles == 0) {
		LOG_ERR("Channel can not be set to zero");
		return -ENOTSUP;
	}

	ret = mcux_ctimer_pwm_select_period_channel(data, pulse_channel, period_cycles,
						    &period_channel);
	if (ret != 0) {
		LOG_ERR("could not select valid period channel. ret=%d", ret);
		return ret;
	}

	if (flags & PWM_POLARITY_INVERTED) {
		if (pulse_cycles == 0) {
			/* make pulse cycles greater than period so event never occurs */
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

#ifdef CONFIG_PWM_CAPTURE
static void mcux_ctimer_capture_isr(const struct device *dev)
{
	const struct pwm_mcux_ctimer_config *config = dev->config;
	struct pwm_mcux_ctimer_data *data = dev->data;
	uint32_t period = 0;	// ARG_UNUSED
	uint32_t pulse = 0;		// ARG_UNUSED
	int err = 0;
	uint8_t interrupt_flag = 0;
	
	if (CTIMER_GetStatusFlags(config->base) & kCTIMER_Capture0Flag) {
		interrupt_flag = 1;
		data->operate_channel = kCTIMER_Capture_0;
		data->capture_status_flags = kCTIMER_Capture0Flag;
	}
	else if (CTIMER_GetStatusFlags(config->base) & kCTIMER_Capture1Flag) {
		interrupt_flag = 1;
		data->operate_channel = kCTIMER_Capture_1;
		data->capture_status_flags = kCTIMER_Capture1Flag;
	}
	else if (CTIMER_GetStatusFlags(config->base) & kCTIMER_Capture2Flag) {
		interrupt_flag = 1;
		data->operate_channel = kCTIMER_Capture_2;
		data->capture_status_flags = kCTIMER_Capture2Flag;
	}
	else if (CTIMER_GetStatusFlags(config->base) & kCTIMER_Capture3Flag) {
		interrupt_flag = 1;
		data->operate_channel = kCTIMER_Capture_3;
		data->capture_status_flags = kCTIMER_Capture3Flag;
	}

	if (interrupt_flag) {
		interrupt_flag = 0;
		if (data->capture_callback != NULL) {
			data->capture_callback(dev, data->operate_channel,
										period, pulse, err,
										data->user_data);
		}
		
		// Clear Capture Interrupt Flag
		CTIMER_ClearStatusFlags(config->base, data->capture_status_flags);
	}
}

static int mcux_ctimer_configure_capture(const struct device *dev,
										 uint32_t channel, pwm_flags_t flags,
										 pwm_capture_callback_handler_t cb,
										 void *user_data)
{
	const struct pwm_mcux_ctimer_config *config = dev->config;
	struct pwm_mcux_ctimer_data *data = dev->data;
	uint32_t timer_captsel_base;

	// flags represent PWM_POLARITY_NORMAL and PWM_POLARITY_INVERTED
	// Future used for calculate duty cycle
	ARG_UNUSED(flags);

	if (channel >= MAX_CAPTURE_CHANNEL_COUNT) {
		LOG_ERR("Invalid channel %u. must be less than %u", channel, MAX_CAPTURE_CHANNEL_COUNT);
		return -EINVAL;
	}

	data->channel_states[channel].role = PWM_CTIMER_CHANNEL_ROLE_CAPTURE;

	// INPUT MUX Initialization
	INPUTMUX_Init(INPUTMUX); // Enable INPUTMUX Clock
	// INPUTMUX0->CTIMER0CAP0 = INPUTMUX_CTIMER0CAP0_INP(0);
	// CTIMERiCAPj => index equals to channel of CTIMERi => equals to j
	// inputmux_connection_t connection = kINPUTMUX_CtimerInp0ToTimer0Captsel

	// LOG_DBG("device name %s", dev->name);
	// Compare base address with CTIMER0, CTIMER1, CTIMER2, CTIMER3, CTIMER4
	timer_captsel_base = TIMER0CAPTSEL0;
	if (config->base == CTIMER0) {
		// LOG_DBG("Detect CTIMER0");
		timer_captsel_base = TIMER0CAPTSEL0;
	}
	else if (config->base == CTIMER1) {
		// LOG_DBG("Detect CTIMER1");
		timer_captsel_base = TIMER1CAPTSEL0;
	}
	else if (config->base == CTIMER2) {
		// LOG_DBG("Detect CTIMER2");
		timer_captsel_base = TIMER2CAPTSEL0;
	}
	else if (config->base == CTIMER3) {
		// LOG_DBG("Detect CTIMER3");
		timer_captsel_base = TIMER3CAPTSEL0;
	}
	else if (config->base == CTIMER4) {
		// LOG_DBG("Detect CTIMER4");
		timer_captsel_base = TIMER4CAPTSEL0;
	}

	switch (channel)
	{
	case 0:
		data->capture_interrupt_enable = kCTIMER_Capture0InterruptEnable;
		break;
	case 1:
		data->capture_interrupt_enable = kCTIMER_Capture1InterruptEnable;
		break;
	case 2:
		data->capture_interrupt_enable = kCTIMER_Capture2InterruptEnable;
		break;
	case 3:
		data->capture_interrupt_enable = kCTIMER_Capture3InterruptEnable;
		break;
	default:
		LOG_ERR("Invalid capture channel %u. must be less than %u", channel, MAX_CAPTURE_CHANNEL_COUNT);
		return -EINVAL;
	}

	data->inputmux_connection[channel] = (uint32_t)config->inputmux + (timer_captsel_base << PMUX_SHIFT);
	INPUTMUX_AttachSignal(INPUTMUX, channel, data->inputmux_connection[channel]);

	// Setup ctimer CCR register for capture
	CTIMER_SetupCapture(config->base, channel, config->capture_edge, true);
	config->base->CTCR &= ~CTIMER_CTCR_ENCC(1);	   // Clear Capture Channel Enable
	config->base->CTCR &= ~CTIMER_CTCR_SELCC_MASK; // clear SELCC in CTCR register

	data->capture_callback = cb;
	data->user_data = user_data;

	return 0;
}

static int mcux_ctimer_enable_capture(const struct device *dev, uint32_t channel)
{
	const struct pwm_mcux_ctimer_config *config = dev->config;
	struct pwm_mcux_ctimer_data *data = dev->data;

	if (data->channel_states[channel].role != PWM_CTIMER_CHANNEL_ROLE_CAPTURE) {
		LOG_ERR("Channel %u is not configured for capture", channel);
		return -EINVAL;
	}

	if (channel >= MAX_CAPTURE_CHANNEL_COUNT) {
		LOG_ERR("Invalid channel %u. must be less than %u", channel, MAX_CAPTURE_CHANNEL_COUNT);
		return -EINVAL;
	}
	
	// Writing 1 to this field enables clearing of the timer and the prescaler 
	// when the capture-edge event specified in SELCC occurs
	if (config->capture_channel_enable) {
		config->base->CTCR |= CTIMER_CTCR_ENCC(1);	// enable
	}
	else {
		config->base->CTCR &= ~CTIMER_CTCR_ENCC(1);	// disable
	}

	// SELCC take effect only when ENCC is 1
	// Setup CTCR count control register
	switch (config->capture_edge)
	{
	case kCTIMER_Capture_RiseEdge:
		config->base->CTCR |= CTIMER_CTCR_SELCC((channel << 1)); // clear TC/PR on rising edge
		// Enable edge capture with CCR register
		CTIMER_EnableRisingEdgeCapture(config->base, channel, true);
		break;
	case kCTIMER_Capture_FallEdge:
		config->base->CTCR |= CTIMER_CTCR_SELCC((channel << 1) | 1); // clear TC/PR on falling edge
		// Enable edge capture with CCR register
		CTIMER_EnableFallingEdgeCapture(config->base, channel, true);
		break;
	case kCTIMER_Capture_BothEdge:
		// CTimer supports both edges capture
		// But CTCR can only clear TC at either rising or falling edge
		config->base->CTCR |= CTIMER_CTCR_SELCC((channel << 1));	 // clear TC/PR on rising edge
		config->base->CTCR |= CTIMER_CTCR_SELCC((channel << 1) | 1); // clear TC/PR on falling edge
		// Enable edge capture with CCR register
		CTIMER_EnableRisingEdgeCapture(config->base, channel, true);
		CTIMER_EnableFallingEdgeCapture(config->base, channel, true);
		break;
	default:
		LOG_ERR("Invalid capture edge %d", config->capture_edge);
		return -EINVAL;
	}

	CTIMER_EnableInterrupts(config->base, data->capture_interrupt_enable);
	CTIMER_StartTimer(config->base);

	return 0;
}

static int mcux_ctimer_disable_capture(const struct device *dev, uint32_t channel)
{
	const struct pwm_mcux_ctimer_config *config = dev->config;
	struct pwm_mcux_ctimer_data *data = dev->data;

	if (data->channel_states[channel].role != PWM_CTIMER_CHANNEL_ROLE_CAPTURE) {
		LOG_ERR("Channel %u is not configured for capture", channel);
		return -EINVAL;
	}

	if (channel >= MAX_CAPTURE_CHANNEL_COUNT) {
		LOG_ERR("Invalid channel %u. must be less than %u", channel, MAX_CAPTURE_CHANNEL_COUNT);
		return -EINVAL;
	}

	CTIMER_DisableInterrupts(config->base, data->capture_interrupt_enable);
	CTIMER_StopTimer(config->base);

	return 0;
}
#endif /* CONFIG_PWM_CAPTURE */

static int mcux_ctimer_pwm_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					      uint64_t *cycles)
{
	const struct pwm_mcux_ctimer_config *config = dev->config;
	int err = 0;


	/* clean up upper word of return parameter */
	*cycles &= 0xFFFFFFFF;

	// err = clock_control_get_rate(config->clock_dev, config->clock_subsys, (uint32_t *)cycles);
	switch ((uint32_t)config->clock_subsys)
	{
	case MCUX_CTIMER0_CLK:
		*cycles = CLOCK_GetCTimerClkFreq(0);
		break;
	case MCUX_CTIMER1_CLK:
		*cycles = CLOCK_GetCTimerClkFreq(1);
		break;
	case MCUX_CTIMER2_CLK:
		*cycles = CLOCK_GetCTimerClkFreq(2);
		break;
	case MCUX_CTIMER3_CLK:
		*cycles = CLOCK_GetCTimerClkFreq(3);
		break;
	case MCUX_CTIMER4_CLK:
		*cycles = CLOCK_GetCTimerClkFreq(4);
		break;
	default:
		err = -EINVAL;
		break;
	}

	if (err != 0) {
		LOG_ERR("could not get clock rate");
		return err;
	}

	if (config->prescale > 0) {
		*cycles /= config->prescale;
	}

	__ASSERT((*cycles) > 0, "Invalid PWM frequency: cycles per second is 0(check clock rate "
				"and prescaler)");

	return err;
}

static int mcux_ctimer_pwm_init(const struct device *dev)
{
	const struct pwm_mcux_ctimer_config *config = dev->config;
	ctimer_config_t ctimer_config;
	int err;

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	if (config->period_channel >= MAX_MATCH_CHANNEL_COUNT) {
		LOG_ERR("invalid period_channel: %d. must be less than %d", config->period_channel,
				MAX_MATCH_CHANNEL_COUNT);
		return -EINVAL;
	}

	CTIMER_GetDefaultConfig(&ctimer_config);
	ctimer_config.mode = config->mode;
	ctimer_config.prescale = config->prescale;

#ifdef CONFIG_PWM_CAPTURE
	ctimer_config.input = config->channel;
#endif /* CONFIG_PWM_CAPTURE */

	// ctimer_config.input is useless when config->mode is 0
	CTIMER_Init(config->base, &ctimer_config);

	config->irq_config_func(dev);

	return 0;
}

static DEVICE_API(pwm, pwm_mcux_ctimer_driver_api) = {
	.set_cycles = mcux_ctimer_pwm_set_cycles,
	.get_cycles_per_sec = mcux_ctimer_pwm_get_cycles_per_sec,
#ifdef CONFIG_PWM_CAPTURE
	.configure_capture = mcux_ctimer_configure_capture,
	.enable_capture = mcux_ctimer_enable_capture,
	.disable_capture = mcux_ctimer_disable_capture,
#endif /* CONFIG_PWM_CAPTURE */
};

#define PWM_MCUX_CTIMER_DEVICE_INIT_MCUX(n)                                                        \
	static void mcux_ctimer_irq_config_func_##n(const struct device *dev);                  \
                                                                                            \
	PINCTRL_DT_INST_DEFINE(n);                                                              \
                                                                                            \
	static struct pwm_mcux_ctimer_data pwm_mcux_ctimer_data_##n = {                            \
		.channel_states =                                                                  \
			{                                                                          \
				[0] = {.role = PWM_CTIMER_CHANNEL_ROLE_NONE,                                \
						     .cycles = 0},                                 \
				[1] = {.role = PWM_CTIMER_CHANNEL_ROLE_NONE,                                \
						     .cycles = 0},                                 \
				[2] = {.role = PWM_CTIMER_CHANNEL_ROLE_NONE,                                \
						     .cycles = 0},                                 \
				[3] = {.role = PWM_CTIMER_CHANNEL_ROLE_NONE,                                \
						     .cycles = 0},                                 \
			},                                                                         \
		.current_period_channel = kCTIMER_Match_0,                                         \
		.is_period_channel_set = false,                                                    \
	};                                                                                         \
	static const struct pwm_mcux_ctimer_config pwm_mcux_ctimer_config_##n = {                  \
		.base = (CTIMER_Type *)DT_INST_REG_ADDR(n),                                        \
		.mode = DT_INST_PROP(n, mode),                                                      \
		.channel = DT_INST_PROP(n, channel),                                                \
		.capture_channel_enable = DT_INST_PROP(n, capture_channel_enable),                  \
		.capture_edge = DT_INST_PROP(n, capture_edge),                                      \
		.inputmux = DT_INST_PROP(n, inputmux),                                              \
		.prescale = DT_INST_PROP(n, prescale),                                              \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                 \
		.clock_subsys = (clock_control_subsys_t)(DT_INST_CLOCKS_CELL(n, name)),             \
		.irq_config_func = mcux_ctimer_irq_config_func_##n,                                 \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                        \
	};                                                                                      \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, mcux_ctimer_pwm_init, NULL, &pwm_mcux_ctimer_data_##n,            \
			      &pwm_mcux_ctimer_config_##n, POST_KERNEL,                            \
						  CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &pwm_mcux_ctimer_driver_api); \
                                                                                            \
	static void mcux_ctimer_irq_config_func_##n(const struct device *dev)                   \
	{                                                                                       \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),                              \
					mcux_ctimer_capture_isr, DEVICE_DT_INST_GET(n), 0);                     \
		irq_enable(DT_INST_IRQN(n));                                                        \
	}

DT_INST_FOREACH_STATUS_OKAY(PWM_MCUX_CTIMER_DEVICE_INIT_MCUX)
