/*
 * Copyright (c) 2019, Linaro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_pwm

#include <errno.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/clock_control.h>
#include <soc.h>
#include <fsl_pwm.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_mcux, CONFIG_PWM_LOG_LEVEL);

#define CHANNEL_COUNT 3

struct pwm_mcux_config {
	PWM_Type *base;
	uint8_t index;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	pwm_clock_prescale_t prescale;
	pwm_register_reload_t reload;
	pwm_mode_t mode;
	bool run_wait;
	bool run_debug;
	const struct pinctrl_dev_config *pincfg;
};

struct pwm_mcux_data {
	uint32_t clock_freq;
	uint32_t period_cycles[CHANNEL_COUNT];
	uint32_t pulse_cycles[CHANNEL_COUNT];
	pwm_signal_param_t channel[CHANNEL_COUNT];
	struct k_mutex lock;
};

static int mcux_pwm_set_cycles_internal(const struct device *dev, uint32_t channel,
			       uint32_t period_cycles, uint32_t pulse_cycles,
			       pwm_flags_t flags)
{
	const struct pwm_mcux_config *config = dev->config;
	struct pwm_mcux_data *data = dev->data;
	pwm_level_select_t level;
	uint32_t pwm_clk_freq;

	pwm_clk_freq = data->clock_freq >> config->prescale;

	if (flags & PWM_POLARITY_INVERTED) {
		level = kPWM_LowTrue;
	} else {
		level = kPWM_HighTrue;
	}

	if (period_cycles != data->period_cycles[channel]
	    || level != data->channel[channel].level) {
		status_t status;

		data->period_cycles[channel] = period_cycles;

		data->channel[channel].pwmchannelenable = true;

		PWM_StopTimer(config->base, 1U << config->index);

		/*
		 * We will directly write the duty cycle pulse width
		 * and full pulse width into the VALx registers to
		 * setup PWM with higher resolution.
		 * Therefore we use dummy values for the duty cycle
		 * and frequency.
		 */
		data->channel[channel].dutyCyclePercent = 0;
		data->channel[channel].level = level;
		data->pulse_cycles[channel] = pulse_cycles;

		status = PWM_SetupPwm(config->base, config->index,
				      &data->channel[channel], 1U,
				      config->mode, pwm_clk_freq, data->clock_freq);
		if (status != kStatus_Success) {
			LOG_ERR("Could not set up pwm");
			return -ENOTSUP;
		}

		if (channel == 2) {
			/* For channels A/B, when the counter matches VAL2/VAL4 or VAL3/VAL5,
			 * the output status changes. VAL2 and VAL4 are set to 0, so the channel
			 * output is high at the beginning of the period, then becomes low when
			 * the counter matches VAL3/VAL5 (pulse width).
			 * Channel X only uses VAL0 for pulse width, so its polarity must be
			 * handled differently.
			 */
			if (level == kPWM_HighTrue) {
				config->base->SM[config->index].OCTRL |=
					((uint16_t)1U << PWM_OCTRL_POLX_SHIFT);
			} else {
				config->base->SM[config->index].OCTRL &=
					~((uint16_t)1U << PWM_OCTRL_POLX_SHIFT);
			}
		} else if (data->period_cycles[2] != 0U) {
			/* When setting channel A/B, PWM_SetupPwm internally calls
			 * PWM_SetDutycycleRegister which modifies VAL0. Since VAL0 controls
			 * channel X's pulse width, we need to restore it to maintain channel X's
			 * configured pulse cycles.
			 */
			config->base->SM[config->index].VAL0 = data->pulse_cycles[2];
		} else {
			/* No action required. */
		}

		/* Setup VALx values directly for edge aligned PWM */
		if (channel == 0) {
			/* Side A */
			PWM_SetVALxValue(config->base, config->index,
					 kPWM_ValueRegister_1,
					 (uint16_t)(period_cycles - 1U));
			PWM_SetVALxValue(config->base, config->index,
					 kPWM_ValueRegister_2, 0U);
			PWM_SetVALxValue(config->base, config->index,
					 kPWM_ValueRegister_3,
					 (uint16_t)pulse_cycles);
		} else if (channel == 1) {
			/* Side B */
			PWM_SetVALxValue(config->base, config->index,
					 kPWM_ValueRegister_1,
					 (uint16_t)(period_cycles - 1U));
			PWM_SetVALxValue(config->base, config->index,
					 kPWM_ValueRegister_4, 0U);
			PWM_SetVALxValue(config->base, config->index,
					 kPWM_ValueRegister_5,
					 (uint16_t)pulse_cycles);
		} else {
			/* Side X */
			PWM_SetVALxValue(config->base, config->index,
					 kPWM_ValueRegister_0,
					 (uint16_t)pulse_cycles);
			PWM_SetVALxValue(config->base, config->index,
					 kPWM_ValueRegister_1,
					 (uint16_t)(period_cycles - 1U));
		}

		PWM_SetPwmLdok(config->base, 1U << config->index, true);

		PWM_StartTimer(config->base, 1U << config->index);
	} else {
		uint64_t period_time_us =
			(uint64_t)data->period_cycles[channel] * 1000000U / (pwm_clk_freq);
		__ASSERT_NO_MSG(period_time_us <= 0xFFFFFFFFU);
		/* Wait for the registers to finish their previous load (LDOK cleared).
		 * The LDOK is cleared after one PWM period, so we wait period_time_us.
		 * Keep 1 millisecond here for compatibility.
		 */
		bool ldok_got_cleared = WAIT_FOR(
			!(config->base->MCTRL & PWM_MCTRL_LDOK(1U << config->index)),
			MAX(1000, (uint32_t)period_time_us),
			k_busy_wait(1) /* busywait meanwhile */
		);

		if (!ldok_got_cleared) {
			/*
			 * LDOK didn't get cleared in timeout, which is extremely rare.
			 * We return with an error though, because setting the VALx values in
			 * this state would do nothing
			 */
			return -EBUSY;
		}

		/* Setup VALx values directly for edge aligned PWM */
		if (channel == 0) {
			/* Side A */
			PWM_SetVALxValue(config->base, config->index,
					 kPWM_ValueRegister_2, 0U);
			PWM_SetVALxValue(config->base, config->index,
					 kPWM_ValueRegister_3,
					 (uint16_t)pulse_cycles);
		} else if (channel == 1) {
			/* Side B */
			PWM_SetVALxValue(config->base, config->index,
					 kPWM_ValueRegister_4, 0U);
			PWM_SetVALxValue(config->base, config->index,
					 kPWM_ValueRegister_5,
					 (uint16_t)pulse_cycles);
		} else {
			/* Side X */
			PWM_SetVALxValue(config->base, config->index,
					 kPWM_ValueRegister_0,
					 (uint16_t)pulse_cycles);
		}
		PWM_SetPwmLdok(config->base, 1U << config->index, true);
	}

	return 0;
}

static int mcux_pwm_set_cycles(const struct device *dev, uint32_t channel,
			       uint32_t period_cycles, uint32_t pulse_cycles,
			       pwm_flags_t flags)
{
	struct pwm_mcux_data *data = dev->data;
	int result;

	if (channel >= CHANNEL_COUNT) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	if (period_cycles == 0) {
		LOG_ERR("Channel can not be set to inactive level");
		return -ENOTSUP;
	}

	if (period_cycles > UINT16_MAX) {
		/* 16-bit resolution */
		LOG_ERR("Too long period (%u), adjust pwm prescaler!",
			period_cycles);
		/* TODO: dynamically adjust prescaler */
		return -EINVAL;
	}
	k_mutex_lock(&data->lock, K_FOREVER);
	result = mcux_pwm_set_cycles_internal(dev, channel, period_cycles, pulse_cycles, flags);
	k_mutex_unlock(&data->lock);
	return result;
}

static int mcux_pwm_get_cycles_per_sec(const struct device *dev,
				       uint32_t channel, uint64_t *cycles)
{
	const struct pwm_mcux_config *config = dev->config;
	struct pwm_mcux_data *data = dev->data;

	*cycles = data->clock_freq >> config->prescale;

	return 0;
}

static int pwm_mcux_init(const struct device *dev)
{
	const struct pwm_mcux_config *config = dev->config;
	struct pwm_mcux_data *data = dev->data;
	pwm_config_t pwm_config;
	status_t status;
	int i, err;

	k_mutex_init(&data->lock);

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
				   &data->clock_freq)) {
		LOG_ERR("Could not get clock frequency");
		return -EINVAL;
	}

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	LOG_DBG("Set prescaler %d, reload mode %d",
			1 << config->prescale, config->reload);

	PWM_GetDefaultConfig(&pwm_config);
	pwm_config.prescale = config->prescale;
	pwm_config.reloadLogic = config->reload;
	pwm_config.clockSource = kPWM_BusClock;
	pwm_config.enableDebugMode = config->run_debug;
#if !defined(FSL_FEATURE_PWM_HAS_NO_WAITEN) || (!FSL_FEATURE_PWM_HAS_NO_WAITEN)
	pwm_config.enableWait = config->run_wait;
#endif

	status = PWM_Init(config->base, config->index, &pwm_config);
	if (status != kStatus_Success) {
		LOG_ERR("Unable to init PWM");
		return -EIO;
	}

	/* Disable fault sources */
	for (i = 0; i < FSL_FEATURE_PWM_FAULT_CH_COUNT; i++) {
		config->base->SM[config->index].DISMAP[i] = 0x0000;
	}

	data->channel[0].pwmChannel = kPWM_PwmA;
	data->channel[0].level = kPWM_HighTrue;
	data->channel[1].pwmChannel = kPWM_PwmB;
	data->channel[1].level = kPWM_HighTrue;
	data->channel[2].pwmChannel = kPWM_PwmX;
	data->channel[2].level = kPWM_HighTrue;

	return 0;
}

static DEVICE_API(pwm, pwm_mcux_driver_api) = {
	.set_cycles = mcux_pwm_set_cycles,
	.get_cycles_per_sec = mcux_pwm_get_cycles_per_sec,
};

#define PWM_DEVICE_INIT_MCUX(n)			  \
	static struct pwm_mcux_data pwm_mcux_data_ ## n;		  \
	PINCTRL_DT_INST_DEFINE(n);					  \
									  \
	static const struct pwm_mcux_config pwm_mcux_config_ ## n = {     \
		.base = (PWM_Type *)DT_REG_ADDR(DT_INST_PARENT(n)),	  \
		.index = DT_INST_PROP(n, index),			  \
		.mode = kPWM_EdgeAligned,				  \
		.prescale = _CONCAT(kPWM_Prescale_Divide_, DT_INST_PROP(n, nxp_prescaler)),\
		.reload = DT_ENUM_IDX_OR(DT_DRV_INST(n), nxp_reload,\
				kPWM_ReloadPwmFullCycle),\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),		\
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),\
		.run_wait = DT_INST_PROP(n, run_in_wait),		  \
		.run_debug = DT_INST_PROP(n, run_in_debug),		  \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		  \
	};								  \
									  \
	DEVICE_DT_INST_DEFINE(n,					  \
			    pwm_mcux_init,				  \
			    NULL,					  \
			    &pwm_mcux_data_ ## n,			  \
			    &pwm_mcux_config_ ## n,			  \
			    POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,	  \
			    &pwm_mcux_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_DEVICE_INIT_MCUX)
