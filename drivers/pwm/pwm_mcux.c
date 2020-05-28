/*
 * Copyright (c) 2019, Linaro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_pwm

#include <errno.h>
#include <drivers/pwm.h>
#include <soc.h>
#include <fsl_pwm.h>
#include <fsl_clock.h>

#define LOG_LEVEL CONFIG_PWM_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(pwm_mcux);

#define CHANNEL_COUNT 2

struct pwm_mcux_config {
	PWM_Type *base;
	uint8_t index;
	clock_name_t clock_source;
	pwm_clock_prescale_t prescale;
	pwm_mode_t mode;
};

struct pwm_mcux_data {
	uint32_t period_cycles[CHANNEL_COUNT];
	pwm_signal_param_t channel[CHANNEL_COUNT];
};

static int mcux_pwm_pin_set(struct device *dev, uint32_t pwm,
			    uint32_t period_cycles, uint32_t pulse_cycles,
			    pwm_flags_t flags)
{
	const struct pwm_mcux_config *config = dev->config;
	struct pwm_mcux_data *data = dev->data;
	uint8_t duty_cycle;

	if (pwm >= CHANNEL_COUNT) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	if (flags) {
		/* PWM polarity not supported (yet?) */
		return -ENOTSUP;
	}

	if ((period_cycles == 0) || (pulse_cycles > period_cycles)) {
		LOG_ERR("Invalid combination: period_cycles=%u, "
			"pulse_cycles=%u", period_cycles, pulse_cycles);
		return -EINVAL;
	}

	if (period_cycles > UINT16_MAX) {
		/* 16-bit resolution */
		LOG_ERR("Too long period (%u), adjust pwm prescaler!",
			period_cycles);
		/* TODO: dynamically adjust prescaler */
		return -EINVAL;
	}

	duty_cycle = 100 * pulse_cycles / period_cycles;

	/* FIXME: Force re-setup even for duty-cycle update */
	if (period_cycles != data->period_cycles[pwm]) {
		uint32_t clock_freq;
		uint32_t pwm_freq;
		status_t status;

		data->period_cycles[pwm] = period_cycles;

		LOG_DBG("SETUP dutycycle to %u\n", duty_cycle);

		clock_freq = CLOCK_GetFreq(config->clock_source);
		pwm_freq = (clock_freq >> config->prescale) / period_cycles;

		if (pwm_freq == 0) {
			LOG_ERR("Could not set up pwm_freq=%d", pwm_freq);
			return -EINVAL;
		}

		PWM_StopTimer(config->base, 1U << config->index);

		data->channel[pwm].dutyCyclePercent = duty_cycle;

		status = PWM_SetupPwm(config->base, config->index,
				      &data->channel[0], CHANNEL_COUNT,
				      config->mode, pwm_freq, clock_freq);
		if (status != kStatus_Success) {
			LOG_ERR("Could not set up pwm");
			return -ENOTSUP;
		}

		PWM_SetPwmLdok(config->base, 1U << config->index, true);

		PWM_StartTimer(config->base, 1U << config->index);
	} else {
		PWM_UpdatePwmDutycycle(config->base, config->index,
				       (pwm == 0) ? kPWM_PwmA : kPWM_PwmB,
				       config->mode, duty_cycle);
		PWM_SetPwmLdok(config->base, 1U << config->index, true);
	}

	return 0;
}

static int mcux_pwm_get_cycles_per_sec(struct device *dev, uint32_t pwm,
				       uint64_t *cycles)
{
	const struct pwm_mcux_config *config = dev->config;

	*cycles = CLOCK_GetFreq(config->clock_source) >> config->prescale;

	return 0;
}

static int pwm_mcux_init(struct device *dev)
{
	const struct pwm_mcux_config *config = dev->config;
	struct pwm_mcux_data *data = dev->data;
	pwm_config_t pwm_config;
	status_t status;

	PWM_GetDefaultConfig(&pwm_config);
	pwm_config.prescale = config->prescale;
	pwm_config.reloadLogic = kPWM_ReloadPwmFullCycle;

	status = PWM_Init(config->base, config->index, &pwm_config);
	if (status != kStatus_Success) {
		LOG_ERR("Unable to init PWM");
		return -EIO;
	}

	/* Disable fault sources */
	((PWM_Type *)config->base)->SM[config->index].DISMAP[0] = 0x0000;
	((PWM_Type *)config->base)->SM[config->index].DISMAP[1] = 0x0000;

	data->channel[0].pwmChannel = kPWM_PwmA;
	data->channel[0].level = kPWM_HighTrue;
	data->channel[1].pwmChannel = kPWM_PwmB;
	data->channel[1].level = kPWM_HighTrue;

	return 0;
}

static const struct pwm_driver_api pwm_mcux_driver_api = {
	.pin_set = mcux_pwm_pin_set,
	.get_cycles_per_sec = mcux_pwm_get_cycles_per_sec,
};

#define PWM_DEVICE_INIT_MCUX(n)			  \
	static struct pwm_mcux_data pwm_mcux_data_ ## n;		  \
									  \
	static const struct pwm_mcux_config pwm_mcux_config_ ## n = {     \
		.base = (void *)DT_REG_ADDR(DT_PARENT(DT_DRV_INST(n))),   \
		.index = DT_INST_PROP(n, index),			  \
		.mode = kPWM_EdgeAligned,				  \
		.prescale = kPWM_Prescale_Divide_128,			  \
		.clock_source = kCLOCK_IpgClk,				  \
	};								  \
									  \
	DEVICE_AND_API_INIT(pwm_mcux_ ## n,				  \
			    DT_INST_LABEL(n),				  \
			    pwm_mcux_init,				  \
			    &pwm_mcux_data_ ## n,			  \
			    &pwm_mcux_config_ ## n,			  \
			    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,\
			    &pwm_mcux_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_DEVICE_INIT_MCUX)
