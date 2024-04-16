/*
 * (c) Meta Platforms, Inc. and affiliates.
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

struct pwm_mcux_ctimer_data {
	struct pwm_ctimer_channel_state channel_states[CHANNEL_COUNT];
	ctimer_match_t current_period_channel;
	bool is_period_channel_set;
	uint32_t num_active_pulse_chans;
};

struct pwm_mcux_ctimer_config {
	CTIMER_Type *base;
	uint32_t prescale;
	uint32_t period_channel;
	const struct device *clock_control;
	clock_control_subsys_t clock_id;
	const struct pinctrl_dev_config *pincfg;
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

	if (config->prescale > 0) {
		*cycles /= config->prescale;
	}

	return err;
}

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
	return 0;
}

static const struct pwm_driver_api pwm_mcux_ctimer_driver_api = {
	.set_cycles = mcux_ctimer_pwm_set_cycles,
	.get_cycles_per_sec = mcux_ctimer_pwm_get_cycles_per_sec,
};

#define PWM_MCUX_CTIMER_PINCTRL_DEFINE(n) PINCTRL_DT_INST_DEFINE(n);
#define PWM_MCUX_CTIMER_PINCTRL_INIT(n)   .pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),

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
	static const struct pwm_mcux_ctimer_config pwm_mcux_ctimer_config_##n = {                  \
		.base = (CTIMER_Type *)DT_INST_REG_ADDR(n),                                        \
		.prescale = DT_INST_PROP(n, prescaler),                                            \
		.clock_control = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                            \
		.clock_id = (clock_control_subsys_t)(DT_INST_CLOCKS_CELL(n, name) +                \
						     MCUX_CTIMER_CLK_OFFSET),                      \
		PWM_MCUX_CTIMER_PINCTRL_INIT(n)};                                                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, mcux_ctimer_pwm_init, NULL, &pwm_mcux_ctimer_data_##n,            \
			      &pwm_mcux_ctimer_config_##n, POST_KERNEL,                            \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &pwm_mcux_ctimer_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_MCUX_CTIMER_DEVICE_INIT_MCUX)
