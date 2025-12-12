/*
 * Copyright (c) 2021, 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#define DT_DRV_COMPAT nxp_sctimer_pwm

#include <errno.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/pwm.h>
#include <fsl_sctimer.h>
#include <fsl_clock.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#ifdef CONFIG_PWM_CAPTURE
#include <zephyr/irq.h>
#include <fsl_inputmux.h>
#endif
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_mcux_sctimer, CONFIG_PWM_LOG_LEVEL);

#define CHANNEL_COUNT FSL_FEATURE_SCT_NUMBER_OF_OUTPUTS

#define CAPTURE_CHANNEL_COUNT FSL_FEATURE_SCT_NUMBER_OF_MATCH_CAPTURE

/* Constant identifying that no event number has been set */
#define EVENT_NOT_SET FSL_FEATURE_SCT_NUMBER_OF_EVENTS

#ifdef CONFIG_PM_DEVICE
typedef struct pwm_channel_config {
	uint32_t period_cycles;
	uint32_t duty_cycles;
	pwm_flags_t flags;
} pwm_channel_config_t;
#endif /* CONFIG_PM_DEVICE */

struct pwm_mcux_sctimer_config {
	SCT_Type *base;
	uint32_t prescale;
	const struct pinctrl_dev_config *pincfg;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
#ifdef CONFIG_PWM_CAPTURE
	const uint32_t *input_channels;
	const uint32_t *inputmux_connections;
	uint8_t input_channel_count;
	void (*irq_config_func)(const struct device *dev);
#endif /* CONFIG_PWM_CAPTURE */
};

#ifdef CONFIG_PWM_CAPTURE
struct pwm_mcux_sctimer_capture_data {
	pwm_capture_callback_handler_t callback;
	void *user_data;
	uint32_t overflow_count;
	uint32_t first_capture_event;
	uint32_t second_capture_event;
	uint32_t first_limit_count;
	uint32_t second_limit_count;
	uint32_t first_capture_value;
	uint32_t second_capture_value;

	bool continuous : 1;
	bool overflowed : 1;
	bool pulse_capture : 1;
	bool capture_ready : 1;
	bool channel_used: 1;
	bool first_edge_captured : 1;
};
#endif /* CONFIG_PWM_CAPTURE */

struct pwm_mcux_sctimer_data {
	uint32_t event_number[CHANNEL_COUNT];
	sctimer_pwm_signal_param_t channel[CHANNEL_COUNT];
#ifdef CONFIG_PM_DEVICE
	pwm_channel_config_t pwm_channel_config[CHANNEL_COUNT];
#endif /* CONFIG_PM_DEVICE */
	uint32_t match_period;
	uint32_t configured_chan;
#ifdef CONFIG_PWM_CAPTURE
	uint32_t match_event;
	struct pwm_mcux_sctimer_capture_data capture_data[CAPTURE_CHANNEL_COUNT];
#endif /* CONFIG_PWM_CAPTURE */
	bool pwm_channel_active;
};

#ifdef CONFIG_PM_DEVICE
static void mcux_sctimer_restore_chn_config(const struct device *dev);
#endif /* CONFIG_PM_DEVICE */

/* Helper to setup channel that has not previously been configured for PWM */
static int mcux_sctimer_new_channel(const struct device *dev,
				    uint32_t channel, uint32_t period_cycles,
				    uint32_t duty_cycle)
{
	const struct pwm_mcux_sctimer_config *config = dev->config;
	struct pwm_mcux_sctimer_data *data = dev->data;
	uint32_t clock_freq;
	uint32_t pwm_freq;

	data->match_period = period_cycles;

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
				&clock_freq)) {
		return -EINVAL;
	}

	pwm_freq = (clock_freq / config->prescale) / period_cycles;
	if (pwm_freq == 0) {
		LOG_ERR("Could not set up pwm_freq=%d", pwm_freq);
		return -EINVAL;
	}

	SCTIMER_StopTimer(config->base, kSCTIMER_Counter_U);

	LOG_DBG("SETUP dutycycle to %u\n", duty_cycle);
	data->channel[channel].dutyCyclePercent = duty_cycle;
	if (SCTIMER_SetupPwm(config->base, &data->channel[channel],
			     kSCTIMER_EdgeAlignedPwm, pwm_freq,
			     clock_freq, &data->event_number[channel]) == kStatus_Fail) {
		LOG_ERR("Could not set up pwm");
		return -ENOTSUP;
	}
#ifdef CONFIG_PWM_CAPTURE
	/* All channel share same period, so we can use one of pwm period
	 * as capture period.
	 */
	data->match_event = data->event_number[channel];
#endif /* CONFIG_PWM_CAPTURE */

	SCTIMER_StartTimer(config->base, kSCTIMER_Counter_U);
	data->configured_chan++;
	return 0;
}

static void mcux_sctimer_pwm_update_polarity(const struct device *dev, uint32_t channel)
{
	const struct pwm_mcux_sctimer_config *config = dev->config;
	struct pwm_mcux_sctimer_data *data = dev->data;
	uint32_t period_event = data->event_number[channel];
	uint32_t pulse_event = period_event + 1;

	/* clear polarity setting*/
	config->base->OUTPUT &= ~(1UL << channel);
	config->base->OUT[channel].SET &= ~((1UL << pulse_event) | (1UL << period_event));
	config->base->OUT[channel].CLR &= ~((1UL << pulse_event) | (1UL << period_event));

	/* Set polarity based on channel level configuration */
	if (data->channel[channel].level == kSCTIMER_HighTrue) {
		/* Set the initial output level to low which is the inactive state */
		config->base->OUTPUT &= ~(1UL << channel);
		/* Set the output when we reach the PWM period */
		SCTIMER_SetupOutputSetAction(config->base, channel, period_event);
		/* Clear the output when we reach the PWM pulse value */
		SCTIMER_SetupOutputClearAction(config->base, channel, pulse_event);
	} else {
		/* Set the initial output level to high which is the inactive state */
		config->base->OUTPUT |= (1UL << channel);
		 /* Clear the output when we reach the PWM period */
		SCTIMER_SetupOutputClearAction(config->base, channel, period_event);
		/* Set the output when we reach the PWM pulse value */
		SCTIMER_SetupOutputSetAction(config->base, channel, pulse_event);
	}
}

static int mcux_sctimer_pwm_set_cycles(const struct device *dev,
				       uint32_t channel, uint32_t period_cycles,
				       uint32_t pulse_cycles, pwm_flags_t flags)
{
	const struct pwm_mcux_sctimer_config *config = dev->config;
	struct pwm_mcux_sctimer_data *data = dev->data;
	uint8_t duty_cycle;
	int ret;

	if (channel >= CHANNEL_COUNT) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	if (period_cycles == 0) {
		LOG_ERR("Channel can not be set to inactive level");
		return -ENOTSUP;
	}

	#ifdef CONFIG_PM_DEVICE
	/* Save channel configuration when CONFIG_PM_DEVICE is defined
	 * to recover it when exiting from PM3
	 */
	data->pwm_channel_config[channel].period_cycles = period_cycles;
	data->pwm_channel_config[channel].duty_cycles = pulse_cycles;
	data->pwm_channel_config[channel].flags = flags;
	data->pwm_channel_active = true;
	#endif /* CONFIG_PM_DEVICE */

	if ((flags & PWM_POLARITY_INVERTED) == 0) {
		data->channel[channel].level = kSCTIMER_HighTrue;
	} else {
		data->channel[channel].level = kSCTIMER_LowTrue;
	}

	duty_cycle = 100 * pulse_cycles / period_cycles;

	if (duty_cycle == 0 && data->configured_chan == 1) {
		/* Only one channel is active. We can turn off the SCTimer
		 * global counter.
		 */
		SCT_Type *base = config->base;

		/* Stop timer so we can set output directly */
		SCTIMER_StopTimer(base, kSCTIMER_Counter_U);

		/* Set the output to inactive State */
		if (data->channel[channel].level == kSCTIMER_HighTrue) {
			base->OUTPUT &= ~(1UL << channel);
		} else {
			base->OUTPUT |= (1UL << channel);
		}

		return 0;
	}

	/* SCTimer has some unique restrictions when operation as a PWM output.
	 * The peripheral is based around a single counter, with a block of
	 * match registers that can trigger corresponding events. When used
	 * as a PWM peripheral, MCUX SDK sets up the SCTimer as follows:
	 * - one match register is used to set PWM output high, and reset
	 *   SCtimer counter. This sets the PWM period
	 * - one match register is used to set PWM output low. This sets the
	 *   pulse length
	 *
	 * This means that when configured, multiple channels must have the
	 * same PWM period, since they all share the same SCTimer counter.
	 */
	if (period_cycles != data->match_period &&
	    data->event_number[channel] == EVENT_NOT_SET &&
	    data->match_period == 0U) {
		/* No PWM signals have been configured. We can set up the first
		 * PWM output using the MCUX SDK.
		 */
		ret = mcux_sctimer_new_channel(dev, channel, period_cycles,
					       duty_cycle);
		if (ret < 0) {
			return ret;
		}
	} else if (data->event_number[channel] == EVENT_NOT_SET) {
		/* We have already configured a PWM signal, but this channel
		 * has not been setup. We can only support this channel
		 * if the period matches that of other PWM signals.
		 */
		if (period_cycles != data->match_period) {
			LOG_ERR("Only one PWM period is supported between "
				"multiple channels");
			return -ENOTSUP;
		}
		/* Setup PWM output using MCUX SDK */
		ret = mcux_sctimer_new_channel(dev, channel, period_cycles,
					       duty_cycle);
	} else if (period_cycles != data->match_period) {
		uint32_t period_event = data->event_number[channel];
		/* We are reconfiguring the period of a configured channel
		 * MCUX SDK does not provide support for this feature, and
		 * we cannot do this safely if multiple channels are setup.
		 */
		if (data->configured_chan != 1) {
			LOG_ERR("Cannot change PWM period when multiple "
				"channels active");
			return -ENOTSUP;
		}

		/* To make this change, we can simply set the MATCHREL
		 * registers for the period match, and the next match
		 * (which the SDK will setup as the pulse match event)
		 */
		SCTIMER_StopTimer(config->base, kSCTIMER_Counter_U);
		/* Update polarity */
		mcux_sctimer_pwm_update_polarity(dev, channel);
		config->base->MATCH[period_event] = period_cycles - 1U;
		config->base->MATCHREL[period_event] = period_cycles - 1U;
		config->base->MATCH[period_event + 1] = pulse_cycles - 1U;
		config->base->MATCHREL[period_event + 1] = pulse_cycles - 1U;
		SCTIMER_StartTimer(config->base, kSCTIMER_Counter_U);
		data->match_period = period_cycles;
	} else {
		SCTIMER_StopTimer(config->base, kSCTIMER_Counter_U);
		/* Update polarity */
		mcux_sctimer_pwm_update_polarity(dev, channel);
		/* Only duty cycle needs to be updated */
		SCTIMER_UpdatePwmDutycycle(config->base, channel, duty_cycle,
					   data->event_number[channel]);
	}

	return 0;
}

static int mcux_sctimer_pwm_get_cycles_per_sec(const struct device *dev,
					       uint32_t channel,
					       uint64_t *cycles)
{
	const struct pwm_mcux_sctimer_config *config = dev->config;
	uint32_t clock_freq;

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
				&clock_freq)) {
		return -EINVAL;
	}

	*cycles = clock_freq / config->prescale;

	return 0;
}


#ifdef CONFIG_PWM_CAPTURE
static void mcux_sctimer_get_edge_events(bool inverted, bool pulse_capture,
				sctimer_event_t *first_edge, sctimer_event_t *second_edge)
{
	if (inverted) {
		*first_edge = kSCTIMER_InputFallEvent;
		*second_edge = pulse_capture ? kSCTIMER_InputRiseEvent : kSCTIMER_InputFallEvent;
	} else {
		*first_edge = kSCTIMER_InputRiseEvent;
		*second_edge = pulse_capture ? kSCTIMER_InputFallEvent : kSCTIMER_InputRiseEvent;
	}
}

static int mcux_sctimer_setup_capture_events(const struct device *dev, uint32_t channel,
				sctimer_event_t first_edge_event,
				sctimer_event_t second_edge_event,
				uint32_t *first_capture_event,
				uint32_t *second_capture_event)
{
	const struct pwm_mcux_sctimer_config *config = dev->config;
	struct pwm_mcux_sctimer_data *data = dev->data;
	uint32_t capture_reg;

	/* Create first edge capture event */
	if (SCTIMER_CreateAndScheduleEvent(config->base, first_edge_event,
			0, channel, kSCTIMER_Counter_U,
			first_capture_event) != kStatus_Success) {
		LOG_ERR("Failed to create first edge event");
		return -ENOTSUP;
	}

	/* Setup capture action for first edge */
	if (SCTIMER_SetupCaptureAction(config->base, kSCTIMER_Counter_U,
			&capture_reg, *first_capture_event) != kStatus_Success) {
		LOG_ERR("Failed to setup first edge capture");
		return -ENOTSUP;
	}

	/* Create second edge capture event */
	if (SCTIMER_CreateAndScheduleEvent(config->base, second_edge_event,
			0, channel, kSCTIMER_Counter_U,
			second_capture_event) != kStatus_Success) {
		LOG_ERR("Failed to create second edge event");
		return -ENOTSUP;
	}

	/* Setup capture action for second edge */
	if (SCTIMER_SetupCaptureAction(config->base, kSCTIMER_Counter_U,
			&capture_reg, *second_capture_event) != kStatus_Success) {
		LOG_ERR("Failed to setup second edge capture");
		return -ENOTSUP;
	}

	if (data->match_event == EVENT_NOT_SET) {
		/* Create limit event for overflow detection */
		if (SCTIMER_CreateAndScheduleEvent(config->base, kSCTIMER_MatchEventOnly,
				0xFFFF, 0, kSCTIMER_Counter_U,
				&data->match_event) != kStatus_Success) {
			LOG_ERR("Failed to create limit event");
			return -ENOTSUP;
		}
		/* Setup counter limit action */
		SCTIMER_SetupCounterLimitAction(config->base, kSCTIMER_Counter_U,
			data->match_event);
	}

	return 0;
}

static int mcux_sctimer_configure_capture(const struct device *dev,
					uint32_t channel, pwm_flags_t flags,
					pwm_capture_callback_handler_t cb,
					void *user_data)
{
	const struct pwm_mcux_sctimer_config *config = dev->config;
	struct pwm_mcux_sctimer_data *data = dev->data;
	bool inverted = (flags & PWM_POLARITY_INVERTED) != 0;
	bool pulse_capture = (flags & PWM_CAPTURE_TYPE_PERIOD) == 0;
	uint32_t first_capture_event = EVENT_NOT_SET;
	uint32_t second_capture_event = EVENT_NOT_SET;
	sctimer_event_t first_edge_event;
	sctimer_event_t second_edge_event;
	int ret;

	if (channel >= CAPTURE_CHANNEL_COUNT) {
		LOG_ERR("invalid channel %d", channel);
		return -EINVAL;
	}

	if (!(flags & PWM_CAPTURE_TYPE_MASK)) {
		LOG_ERR("No capture type specified");
		return -EINVAL;
	}

	if ((flags & PWM_CAPTURE_TYPE_MASK) == PWM_CAPTURE_TYPE_BOTH) {
		LOG_ERR("Cannot capture both period and pulse width");
		return -ENOTSUP;
	}

	if (data->capture_data[channel].channel_used) {
		LOG_ERR("pwm capture in progress");
		return -EBUSY;
	}

	mcux_sctimer_get_edge_events(inverted, pulse_capture, &first_edge_event,
		&second_edge_event);

	data->capture_data[channel].callback = cb;
	data->capture_data[channel].user_data = user_data;
	data->capture_data[channel].continuous =
		(flags & PWM_CAPTURE_MODE_MASK) == PWM_CAPTURE_MODE_CONTINUOUS;
	data->capture_data[channel].pulse_capture = pulse_capture;
	data->capture_data[channel].first_edge_captured = false;

	/* If capture is already configured, return success */
	if (data->capture_data[channel].first_capture_event == EVENT_NOT_SET &&
		data->capture_data[channel].second_capture_event == EVENT_NOT_SET) {
		ret = mcux_sctimer_setup_capture_events(dev, channel, first_edge_event,
				second_edge_event, &first_capture_event, &second_capture_event);
		if (ret != 0) {
			return ret;
		}
		/* Store capture configuration */
		data->capture_data[channel].first_capture_event = first_capture_event;
		data->capture_data[channel].second_capture_event = second_capture_event;
	} else {
		/* Capture already configured, update capture events */
		config->base->EV[data->capture_data[channel].first_capture_event].CTRL &=
			~(SCT_EV_CTRL_IOCOND_MASK | SCT_EV_CTRL_IOSEL_MASK);
		config->base->EV[data->capture_data[channel].first_capture_event].CTRL |=
			first_edge_event | SCT_EV_CTRL_IOSEL(channel);

		config->base->EV[data->capture_data[channel].second_capture_event].CTRL &=
			~(SCT_EV_CTRL_IOCOND_MASK | SCT_EV_CTRL_IOSEL_MASK);
		config->base->EV[data->capture_data[channel].second_capture_event].CTRL |=
			second_edge_event | SCT_EV_CTRL_IOSEL(channel);
	}

	return 0;
}

static int mcux_sctimer_enable_capture(const struct device *dev, uint32_t channel)
{
	const struct pwm_mcux_sctimer_config *config = dev->config;
	struct pwm_mcux_sctimer_data *data = dev->data;
	uint32_t status_flags;

	if (channel >= CAPTURE_CHANNEL_COUNT) {
		LOG_ERR("invalid channel %d", channel);
		return -EINVAL;
	}

	if (!data->capture_data[channel].callback) {
		LOG_ERR("PWM capture not configured");
		return -EINVAL;
	}

	if (data->capture_data[channel].channel_used) {
		LOG_ERR("pwm capture channel in progress");
		return -EBUSY;
	}

	/* Reset capture state */
	data->capture_data[channel].channel_used = true;

	/* The flag is set when match events happen even if interrupt is disabled.
	 * Clear status before enabling interrupt.
	 */
	status_flags = SCTIMER_GetStatusFlags(config->base);
	SCTIMER_ClearStatusFlags(config->base, status_flags);

	/* Enable interrupts for capture events and limit */
	SCTIMER_EnableInterrupts(config->base,
		(1 << data->capture_data[channel].first_capture_event) |
		(1 << data->capture_data[channel].second_capture_event) |
		(1 << data->match_event));

	return 0;
}

static int mcux_sctimer_disable_capture(const struct device *dev, uint32_t channel)
{
	const struct pwm_mcux_sctimer_config *config = dev->config;
	struct pwm_mcux_sctimer_data *data = dev->data;

	if (channel >= CAPTURE_CHANNEL_COUNT) {
		LOG_ERR("invalid channel %d", channel);
		return -EINVAL;
	}

	/* Disable interrupts */
	if (data->capture_data[channel].channel_used) {
		data->capture_data[channel].channel_used = false;
		SCTIMER_DisableInterrupts(config->base,
			(1 << data->capture_data[channel].first_capture_event) |
			(1 << data->capture_data[channel].second_capture_event) |
			(1 << data->match_event));
	}

	/* Stop timer */
	SCTIMER_StopTimer(config->base, kSCTIMER_Counter_U);

	return 0;
}

static int mcux_sctimer_calc_ticks(uint32_t period, uint32_t first_limit, uint32_t second_limit,
				   uint32_t first_capture, uint32_t second_capture,
				   uint32_t *result)
{
	uint32_t ticks;
	uint32_t overflow_ticks;

	/* Calculate overflow ticks */
	if (u32_mul_overflow(second_limit - first_limit, period, &overflow_ticks)) {
		return -ERANGE;
	}

	/* Add capture difference */
	if (second_capture >= first_capture) {
		if (u32_add_overflow(overflow_ticks, second_capture - first_capture, &ticks)) {
			return -ERANGE;
		}
	} else {
		/* Handle counter wrap */
		if (u32_add_overflow(overflow_ticks, period - first_capture + second_capture,
				     &ticks)) {
			return -ERANGE;
		}
	}

	*result = ticks;

	return 0;
}

static void mcux_sctimer_capture_first_edge(const struct device *dev, uint32_t channel)
{
	const struct pwm_mcux_sctimer_config *config = dev->config;
	struct pwm_mcux_sctimer_data *data = dev->data;

	data->capture_data[channel].first_capture_value =
		SCTIMER_GetCaptureValue(config->base, kSCTIMER_Counter_U,
			data->capture_data[channel].first_capture_event);
	data->capture_data[channel].first_limit_count =
		data->capture_data[channel].overflow_count;
	data->capture_data[channel].first_edge_captured = true;
}

static void mcux_sctimer_capture_second_edge(const struct device *dev, uint32_t channel)
{
	const struct pwm_mcux_sctimer_config *config = dev->config;
	struct pwm_mcux_sctimer_data *data = dev->data;

	data->capture_data[channel].second_capture_value =
		SCTIMER_GetCaptureValue(config->base, kSCTIMER_Counter_U,
			data->capture_data[channel].second_capture_event);
	data->capture_data[channel].second_limit_count =
		data->capture_data[channel].overflow_count;
	data->capture_data[channel].capture_ready = true;
	data->capture_data[channel].first_edge_captured = false;
}

static void prepare_next_capture(const struct device *dev, uint32_t channel)
{
	const struct pwm_mcux_sctimer_config *config = dev->config;
	struct pwm_mcux_sctimer_data *data = dev->data;

	data->capture_data[channel].capture_ready = false;
	data->capture_data[channel].overflowed = false;
	data->capture_data[channel].overflow_count = 0;

	/* Clear first capture setting */
	data->capture_data[channel].first_limit_count = 0;
	data->capture_data[channel].first_capture_value = 0;

	if (data->capture_data[channel].continuous) {
		if (!data->capture_data[channel].pulse_capture) {
			/* For period capture, current first edge is start of next period */
			data->capture_data[channel].first_capture_value =
				data->capture_data[channel].second_capture_value;
			data->capture_data[channel].first_limit_count =
				data->capture_data[channel].second_limit_count;
			data->capture_data[channel].first_edge_captured = true;
		} else {
			/* No actions required. */
		}
	} else {
		/* Single capture mode: Disable interrupts */
		SCTIMER_DisableInterrupts(config->base,
			(1 << data->capture_data[channel].first_capture_event) |
			(1 << data->capture_data[channel].second_capture_event) |
			(1 << data->match_event));
	}

	/* Clear second capture setting */
	data->capture_data[channel].second_limit_count = 0;
	data->capture_data[channel].second_capture_value = 0;
}

static void mcux_sctimer_process_channel_events(const struct device *dev, uint32_t channel,
				uint32_t status_flags)
{
	struct pwm_mcux_sctimer_data *data = dev->data;
	uint32_t ticks = 0;
	int err;

	/* Handle limit/overflow interrupt */
	if (status_flags & (1 << data->match_event)) {
		data->capture_data[channel].overflowed |= u32_add_overflow(1,
			data->capture_data[channel].overflow_count,
			&data->capture_data[channel].overflow_count);
	}

	/* Handle first edge capture */
	if (status_flags & (1 << data->capture_data[channel].first_capture_event)) {
		if (!data->capture_data[channel].first_edge_captured) {
			mcux_sctimer_capture_first_edge(dev, channel);
			return;
		}
	}

	/* Handle second edge capture */
	if (status_flags & (1 << data->capture_data[channel].second_capture_event)) {
		if (data->capture_data[channel].first_edge_captured) {
			mcux_sctimer_capture_second_edge(dev, channel);
		}
	}

	/* Process capture if ready */
	if (data->capture_data[channel].capture_ready) {

		err = mcux_sctimer_calc_ticks(data->match_period,
				data->capture_data[channel].first_limit_count,
				data->capture_data[channel].second_limit_count,
				data->capture_data[channel].first_capture_value,
				data->capture_data[channel].second_capture_value,
				&ticks);

		if (!data->capture_data[channel].callback) {
			return;
		}

		if (data->capture_data[channel].pulse_capture) {
			data->capture_data[channel].callback(dev, channel,
				0, ticks, err, data->capture_data[channel].user_data);
		} else {
			data->capture_data[channel].callback(dev, channel,
				ticks, 0, err, data->capture_data[channel].user_data);
		}

		prepare_next_capture(dev, channel);
	}
}

static void mcux_sctimer_isr(const struct device *dev)
{
	const struct pwm_mcux_sctimer_config *config = dev->config;
	struct pwm_mcux_sctimer_data *data = dev->data;
	uint32_t status_flags;

	status_flags = SCTIMER_GetStatusFlags(config->base);
	SCTIMER_ClearStatusFlags(config->base, status_flags);

	for (uint32_t channel = 0; channel < CAPTURE_CHANNEL_COUNT; channel++) {
		if (!data->capture_data[channel].channel_used) {
			continue;
		}
		mcux_sctimer_process_channel_events(dev, channel, status_flags);
	}
}
#endif /* CONFIG_PWM_CAPTURE */

#ifdef CONFIG_PM_DEVICE
static void mcux_sctimer_restore_chn_config(const struct device *dev)
{
	const struct pwm_mcux_sctimer_data *data = dev->data;
	uint8_t channel;

	for (channel = 0; channel < CHANNEL_COUNT; channel++) {
		/* Only restore the channels configured
		 * before entering into a low power mode
		 */
		if (data->pwm_channel_config[channel].period_cycles != 0) {
			mcux_sctimer_pwm_set_cycles(dev, channel,
			data->pwm_channel_config[channel].period_cycles,
			data->pwm_channel_config[channel].duty_cycles,
			data->pwm_channel_config[channel].flags);
		}
	}
}
#endif /* CONFIG_PM_DEVICE */

static int mcux_sctimer_pwm_init_common(const struct device *dev)
{
	const struct pwm_mcux_sctimer_config *config = dev->config;
	struct pwm_mcux_sctimer_data *data = dev->data;
	sctimer_config_t pwm_config;
	status_t status;
	int i;
	int err;

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	SCTIMER_GetDefaultConfig(&pwm_config);

	pwm_config.prescale_l = config->prescale - 1;

	status = SCTIMER_Init(config->base, &pwm_config);
	if (status != kStatus_Success) {
		LOG_ERR("Unable to init PWM");
		return -EIO;
	}

	for (i = 0; i < CHANNEL_COUNT; i++) {
		data->channel[i].output = i;
		data->channel[i].level = kSCTIMER_HighTrue;
		data->channel[i].dutyCyclePercent = 0;
		data->event_number[i] = EVENT_NOT_SET;
	}
	data->match_period = 0;
	data->configured_chan = 0;

#ifdef CONFIG_PWM_CAPTURE
	/* set inputmux connections */
	INPUTMUX_Init(INPUTMUX);
	for (i = 0; i < config->input_channel_count; i++) {
		INPUTMUX_AttachSignal(INPUTMUX, config->input_channels[i],
			config->inputmux_connections[i]);
	}
	/* Initialize capture data */
	data->match_event = EVENT_NOT_SET;
	for (i = 0; i < CAPTURE_CHANNEL_COUNT; i++) {
		/* Reset capture state */
		memset(&data->capture_data[i], 0, sizeof(struct pwm_mcux_sctimer_capture_data));
		data->capture_data[i].first_capture_event = EVENT_NOT_SET;
		data->capture_data[i].second_capture_event = EVENT_NOT_SET;
	}
	/* Configure IRQ */
	config->irq_config_func(dev);
#endif /* CONFIG_PWM_CAPTURE */

	return 0;
}

static int mcux_sctimer_pwm_pm_action(const struct device *dev, enum pm_device_action action)
{
#ifdef CONFIG_PM_DEVICE
	const struct pwm_mcux_sctimer_config *config = dev->config;
	const struct pwm_mcux_sctimer_data *data = dev->data;
	uint8_t channel;
#endif /* CONFIG_PM_DEVICE */

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
#ifdef CONFIG_PM_DEVICE
		SCTIMER_StartTimer(config->base, kSCTIMER_Counter_U);
#endif /* CONFIG_PM_DEVICE */
		break;
	case PM_DEVICE_ACTION_SUSPEND:
#ifdef CONFIG_PM_DEVICE
		/* Halt the timer counters */
		SCTIMER_StopTimer(config->base, kSCTIMER_Counter_U);
		/* Force the configured channels to inactive state */
		for (channel = 0; channel < CHANNEL_COUNT; channel++) {
			if (data->pwm_channel_config[channel].period_cycles != 0) {
				if (data->channel[channel].level == kSCTIMER_HighTrue) {
					config->base->OUTPUT &= ~(1U << channel);
				} else {
					config->base->OUTPUT |= (1U << channel);
				}
			}
		}
#endif /* CONFIG_PM_DEVICE */
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		break;
	case PM_DEVICE_ACTION_TURN_ON:
		mcux_sctimer_pwm_init_common(dev);
#ifdef CONFIG_PM_DEVICE
		if (data->pwm_channel_active == true) {
			mcux_sctimer_restore_chn_config(dev);
		}
#endif /* CONFIG_PM_DEVICE */
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

static int mcux_sctimer_pwm_init(const struct device *dev)
{
	/* Rest of the init is done from the PM_DEVICE_TURN_ON action
	 * which is invoked by pm_device_driver_init().
	 */
	return pm_device_driver_init(dev, mcux_sctimer_pwm_pm_action);
}

static DEVICE_API(pwm, pwm_mcux_sctimer_driver_api) = {
	.set_cycles = mcux_sctimer_pwm_set_cycles,
	.get_cycles_per_sec = mcux_sctimer_pwm_get_cycles_per_sec,
#ifdef CONFIG_PWM_CAPTURE
	.configure_capture = mcux_sctimer_configure_capture,
	.enable_capture = mcux_sctimer_enable_capture,
	.disable_capture = mcux_sctimer_disable_capture,
#endif
};

#define SCTIMER_DECLARE_CFG(n, CAPTURE_INIT) \
	static const struct pwm_mcux_sctimer_config pwm_mcux_sctimer_config_##n = { \
		.base = (SCT_Type *)DT_INST_REG_ADDR(n),				\
		.prescale = DT_INST_PROP(n, prescaler),					\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),				\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),			\
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),	\
		CAPTURE_INIT								\
	}

#ifdef CONFIG_PWM_CAPTURE
#define SCTIMER_CONFIG_FUNC(n) \
static void mcux_sctimer_config_func_##n(const struct device *dev) \
{ \
	IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), \
		    mcux_sctimer_isr, DEVICE_DT_INST_GET(n), 0); \
	irq_enable(DT_INST_IRQN(n)); \
}
#define SCTIMER_CFG_CAPTURE_INIT(n) \
	.input_channels = (const uint32_t[]) DT_INST_PROP(n, input_channels), \
	.inputmux_connections = (const uint32_t[]) DT_INST_PROP(n, inputmux_connections), \
	.input_channel_count = DT_INST_PROP_LEN(n, input_channels), \
	.irq_config_func = mcux_sctimer_config_func_##n
#define SCTIMER_INIT_CFG(n)	SCTIMER_DECLARE_CFG(n, SCTIMER_CFG_CAPTURE_INIT(n))
#else /* !CONFIG_PWM_CAPTURE */
#define SCTIMER_CONFIG_FUNC(n)
#define SCTIMER_CFG_CAPTURE_INIT
#define SCTIMER_INIT_CFG(n)	SCTIMER_DECLARE_CFG(n, SCTIMER_CFG_CAPTURE_INIT)
#endif /* !CONFIG_PWM_CAPTURE */

#define PWM_MCUX_SCTIMER_DEVICE_INIT_MCUX(n)						\
	PINCTRL_DT_INST_DEFINE(n);							\
	static struct pwm_mcux_sctimer_data pwm_mcux_sctimer_data_##n;			\
											\
	SCTIMER_CONFIG_FUNC(n)								\
	SCTIMER_INIT_CFG(n);								\
	PM_DEVICE_DT_INST_DEFINE(n, mcux_sctimer_pwm_pm_action);			\
	DEVICE_DT_INST_DEFINE(n,							\
			      mcux_sctimer_pwm_init,					\
			      PM_DEVICE_DT_INST_GET(n),					\
			      &pwm_mcux_sctimer_data_##n,				\
			      &pwm_mcux_sctimer_config_##n,				\
			      POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,			\
			      &pwm_mcux_sctimer_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_MCUX_SCTIMER_DEVICE_INIT_MCUX)
