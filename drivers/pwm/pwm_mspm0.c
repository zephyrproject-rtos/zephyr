/*
 * Copyright (c) 2024, Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0g1x0x_g3x0x_timer_pwm

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mspm0_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/irq.h>
#include <soc.h>

/* Driverlib includes */
#include <ti/driverlib/dl_timera.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_mspm0, CONFIG_PWM_LOG_LEVEL);

#define CHANNEL_COUNT 2
#define MSPM0_TIMER_CC_COUNT 2

struct pwm_mspm0_config {
	GPTIMER_Regs *base;
	const struct mspm0_clockSys *clock_subsys;
	uint8_t	cc_idx;
	bool is_advanced;

	DL_Timer_ClockConfig clk_config;
	const struct pinctrl_dev_config *pincfg;
};

struct pwm_mspm0_data {
	DL_Timer_PWMConfig pwm_config;
	uint32_t pulse_cycle;
	struct k_mutex lock;
};

static int mspm0_pwm_set_cycles(const struct device *dev, uint32_t channel,
			       uint32_t period_cycles, uint32_t pulse_cycles,
			       pwm_flags_t flags)
{
	const struct pwm_mspm0_config *config = dev->config;
	struct pwm_mspm0_data *data = dev->data;
	int result;

	if (channel >= CHANNEL_COUNT) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	if (period_cycles == 0) {
		LOG_ERR("Channel can not be set to inactive level");
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	data->pwm_config.period = period_cycles;
	if (config->is_advanced) {
		DL_TimerA_PWMConfig pwmcfg = {0};

		pwmcfg.period = data->pwm_config.period;
		pwmcfg.pwmMode = data->pwm_config.pwmMode;
		pwmcfg.startTimer = data->pwm_config.startTimer;
		if (config->cc_idx >= MSPM0_TIMER_CC_COUNT) {
			pwmcfg.isTimerWithFourCC = true;
		}
		DL_TimerA_initPWMMode(config->base, &pwmcfg);
	} else {
		DL_Timer_initPWMMode(config->base, &data->pwm_config);
	}

	data->pulse_cycle = pulse_cycles;
	DL_Timer_setCaptureCompareValue(config->base,
					pulse_cycles,
					config->cc_idx);
	k_mutex_unlock(&data->lock);
	return result;
}

static int mspm0_pwm_get_cycles_per_sec(const struct device *dev,
					uint32_t channel, uint64_t *cycles)
{
	const struct pwm_mspm0_config *config = dev->config;
	const struct device *const clk_dev = DEVICE_DT_GET(DT_NODELABEL(clkmux));
	DL_Timer_ClockConfig clkcfg;
	uint32_t clock_rate;

	DL_Timer_getClockConfig(config->base, &clkcfg);

	if (clock_control_get_rate(clk_dev,
			(clock_control_subsys_t)config->clock_subsys,
			&clock_rate)) {
		return -EINVAL;
	}
	*cycles = clock_rate >> clkcfg.prescale;

	return 0;
}

static int pwm_mspm0_init(const struct device *dev)
{
	const struct pwm_mspm0_config *config = dev->config;
	const struct device *const clk_dev = DEVICE_DT_GET(DT_NODELABEL(clkmux));
	struct pwm_mspm0_data *data = dev->data;
	int err;

	k_mutex_init(&data->lock);

	if (!device_is_ready(clk_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	DL_Timer_reset(config->base);

	if (! DL_Timer_isPowerEnabled(config->base)) {
		DL_Timer_enablePower(config->base);
	}

	DL_Timer_setClockConfig(config->base, &config->clk_config);

	if (config->is_advanced) {
		DL_TimerA_PWMConfig pwmcfg = {0};

		pwmcfg.period = data->pwm_config.period;
		pwmcfg.pwmMode = data->pwm_config.pwmMode;
		pwmcfg.startTimer = data->pwm_config.startTimer;
		if (config->cc_idx >= MSPM0_TIMER_CC_COUNT) {
			pwmcfg.isTimerWithFourCC = true;
		}
		DL_TimerA_initPWMMode(config->base, &pwmcfg);
	} else {
		DL_Timer_initPWMMode(config->base, &data->pwm_config);
	}

	DL_Timer_setCaptureCompareValue(config->base,
					data->pulse_cycle,
					config->cc_idx);

	DL_Timer_clearInterruptStatus(config->base,
				      DL_TIMER_INTERRUPT_ZERO_EVENT);
	DL_Timer_enableInterrupt(config->base,
				 DL_TIMER_INTERRUPT_ZERO_EVENT);

	DL_Timer_enableClock(config->base);
	DL_Timer_setCCPDirection(config->base, 1 << config->cc_idx);

	DL_Timer_startCounter(config->base);

	return 0;
}

static const struct pwm_driver_api pwm_mspm0_driver_api = {
	.set_cycles = mspm0_pwm_set_cycles,
	.get_cycles_per_sec = mspm0_pwm_get_cycles_per_sec,
};

#define MSPM0_PWM_MODE(mode)	DT_CAT(DL_TIMER_PWM_MODE_, mode)

#define PWM_DEVICE_INIT_MSPM0(n)					  \
	static struct pwm_mspm0_data pwm_mspm0_data_ ## n = {		  \
		.pulse_cycle  = DT_PROP(DT_DRV_INST(n), ti_pulse_cycle),  \
		.pwm_config = {.pwmMode = MSPM0_PWM_MODE(DT_STRING_TOKEN(DT_DRV_INST(n), ti_pwm_mode)),\
			       .period = DT_PROP(DT_DRV_INST(n), ti_period),	  \
			       },					  \
	};								  \
	PINCTRL_DT_INST_DEFINE(n);					  \
									  \
	static const struct mspm0_clockSys mspm0_pwm_clockSys ## n =      \
		MSPM0_CLOCK_SUBSYS_FN(n);                                 \
									  \
	static const struct pwm_mspm0_config pwm_mspm0_config_ ## n = {   \
		.base = (GPTIMER_Regs *)DT_REG_ADDR(DT_INST_PARENT(n)),	  \
		.clock_subsys = &mspm0_pwm_clockSys ## n,                 \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		  \
		.cc_idx = DT_PROP(DT_DRV_INST(n), ti_cc_index),		  \
		.is_advanced = DT_INST_NODE_HAS_PROP(n, ti_advanced),	  \
		.clk_config = {.clockSel = (DT_INST_CLOCKS_CELL(n, bus) & \
					    MSPM0_CLOCK_SEL_MASK),        \
			       .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,    \
			       .prescale = DT_PROP(DT_DRV_INST(n), ti_prescaler), \
			       },					  \
	};								  \
									  \
	DEVICE_DT_INST_DEFINE(n,					  \
			      pwm_mspm0_init,				  \
			      NULL,					  \
			      &pwm_mspm0_data_ ## n,			  \
			      &pwm_mspm0_config_ ## n,			  \
			      POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,	  \
			      &pwm_mspm0_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_DEVICE_INIT_MSPM0)
