/*
 * Copyright (c) 2021, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#define DT_DRV_COMPAT nxp_sctimer_pwm

#include <errno.h>
#include <zephyr/drivers/pwm.h>
#include <fsl_sctimer.h>
#include <fsl_clock.h>
#ifdef CONFIG_PINCTRL
#include <zephyr/drivers/pinctrl.h>
#endif

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_mcux_sctimer, CONFIG_PWM_LOG_LEVEL);

#define CHANNEL_COUNT FSL_FEATURE_SCT_NUMBER_OF_OUTPUTS

struct pwm_mcux_sctimer_config {
	SCT_Type *base;
	uint32_t prescale;
#ifdef CONFIG_PINCTRL
	const struct pinctrl_dev_config *pincfg;
#endif
};

struct pwm_mcux_sctimer_data {
	uint32_t period_cycles[CHANNEL_COUNT];
	uint32_t event_number[CHANNEL_COUNT];
	sctimer_pwm_signal_param_t channel[CHANNEL_COUNT];
};

static int mcux_sctimer_pwm_set_cycles(const struct device *dev,
				       uint32_t channel, uint32_t period_cycles,
				       uint32_t pulse_cycles, pwm_flags_t flags)
{
	const struct pwm_mcux_sctimer_config *config = dev->config;
	struct pwm_mcux_sctimer_data *data = dev->data;
	uint8_t duty_cycle;

	if (channel >= CHANNEL_COUNT) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	if (period_cycles == 0) {
		LOG_ERR("Channel can not be set to inactive level");
		return -ENOTSUP;
	}

	if ((flags & PWM_POLARITY_INVERTED) == 0) {
		data->channel[channel].level = kSCTIMER_HighTrue;
	} else {
		data->channel[channel].level = kSCTIMER_LowTrue;
	}

	duty_cycle = 100 * pulse_cycles / period_cycles;

	if (duty_cycle == 0) {
		SCT_Type *base = config->base;

		SCTIMER_StopTimer(config->base, kSCTIMER_Counter_U);

		/* Set the output to inactive State */
		if (data->channel[channel].level == kSCTIMER_HighTrue) {
			base->OUTPUT &= ~(1UL << channel);
		} else {
			base->OUTPUT |= (1UL << channel);
		}

		return 0;
	}

	if (period_cycles != data->period_cycles[channel]) {
		uint32_t clock_freq;
		uint32_t pwm_freq;

		data->period_cycles[channel] = period_cycles;

		/*
		 * Do not divide by the prescale factor as this is accounted for in
		 * the SDK function
		 */
		clock_freq = CLOCK_GetFreq(kCLOCK_BusClk);
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

		SCTIMER_StartTimer(config->base, kSCTIMER_Counter_U);
	} else {
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

	*cycles = CLOCK_GetFreq(kCLOCK_BusClk) / config->prescale;

	return 0;
}

static int mcux_sctimer_pwm_init(const struct device *dev)
{
	const struct pwm_mcux_sctimer_config *config = dev->config;
	struct pwm_mcux_sctimer_data *data = dev->data;
	sctimer_config_t pwm_config;
	status_t status;
	int i;
#ifdef CONFIG_PINCTRL
	int err;

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}
#endif

	SCTIMER_GetDefaultConfig(&pwm_config);
	/* Divide the SCT clock by 8 */
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
		data->period_cycles[i] = 0;
	}

	return 0;
}

static const struct pwm_driver_api pwm_mcux_sctimer_driver_api = {
	.set_cycles = mcux_sctimer_pwm_set_cycles,
	.get_cycles_per_sec = mcux_sctimer_pwm_get_cycles_per_sec,
};

#ifdef CONFIG_PINCTRL
#define PWM_MCUX_SCTIMER_PINCTRL_DEFINE(n) PINCTRL_DT_INST_DEFINE(n);
#define PWM_MCUX_SCTIMER_PINCTRL_INIT(n) .pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),
#else
#define PWM_MCUX_SCTIMER_PINCTRL_DEFINE(n)
#define PWM_MCUX_SCTIMER_PINCTRL_INIT(n)
#endif

#define PWM_MCUX_SCTIMER_DEVICE_INIT_MCUX(n)						\
	PWM_MCUX_SCTIMER_PINCTRL_DEFINE(n)						\
	static struct pwm_mcux_sctimer_data pwm_mcux_sctimer_data_##n;			\
											\
	static const struct pwm_mcux_sctimer_config pwm_mcux_sctimer_config_##n = {	\
		.base = (SCT_Type *)DT_INST_REG_ADDR(n),				\
		.prescale = DT_INST_PROP(n, prescaler),					\
		PWM_MCUX_SCTIMER_PINCTRL_INIT(n)					\
	};										\
											\
	DEVICE_DT_INST_DEFINE(n,							\
			      mcux_sctimer_pwm_init,					\
			      NULL,							\
			      &pwm_mcux_sctimer_data_##n,				\
			      &pwm_mcux_sctimer_config_##n,				\
			      POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,			\
			      &pwm_mcux_sctimer_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_MCUX_SCTIMER_DEVICE_INIT_MCUX)
