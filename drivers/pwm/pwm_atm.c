/*
 * Copyright (c) 2021-2024, Atmosic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pwm_atm, CONFIG_PWM_LOG_LEVEL);

#include <errno.h>
#include <zephyr/drivers/pwm.h>
#ifdef CONFIG_PM
#include <zephyr/pm/policy.h>
#endif
#include <soc.h>

#include "arch.h"
#include "at_wrpr.h"
#include "at_pinmux.h"
#include "at_apb_pwm_regs_core_macro.h"
#include "at_apb_pseq_regs_core_macro.h"

#ifndef PWM_PWM0_CTRL__TOT_DUR__READ
#define DT_DRV_COMPAT atmosic_atm3x_pwm
#define SYS_CLK_IN_KHZ (PCLK_ALT_FREQ / 1000) // PWM block runs on 16MHz clock domain
#define PWM_CLK_CTRL (WRPR_CTRL__CLK_ENABLE | WRPR_CTRL__CLK_SEL)
#define PWM_IRQ PWM_IRQn
#define PWM0_TOT_WIDTH (PWM_PWM0_DUR__LO_DUR__WIDTH + PWM_PWM0_DUR__HI_DUR__WIDTH)
#define PWM_SET_DURATION(I, hi_dur, lo_dur)                                                        \
	do {                                                                                       \
		CMSDK_PWM->PWM##I##_DUR = PWM_PWM##I##_DUR__HI_DUR__WRITE((hi_dur)) |              \
					  PWM_PWM##I##_DUR__LO_DUR__WRITE((lo_dur));               \
	} while (0)

#define PWM_SET_PARAMS(I, polarity, mode)                                                          \
	do {                                                                                       \
		CMSDK_PWM->PWM##I##_CTRL = PWM_PWM##I##_CTRL__INVERT__WRITE(polarity) |            \
					   PWM_PWM##I##_CTRL__MODE__WRITE(mode) |                  \
					   PWM_PWM##I##_CTRL__OK_TO_RUN__WRITE(0);                 \
	} while (0)
#define PWM(n) CONCAT(PWM, DT_INST_PROP(n, channel))
#else
#define DT_DRV_COMPAT atmosic_atmx2_pwm
#include "at_apb_nvm_regs_core_macro.h"
#include "nvm.h"
#define PWM_CLK_CTRL WRPR_CTRL__CLK_ENABLE
#define PULSE_CYCLES_FROM_DUTY_CYCLE(period_cycles, duty_cycle)	((period_cycles * duty_cycle) / 100)
#define PWM0_TOT_WIDTH PWM_PWM0_CTRL__TOT_DUR__WIDTH
#define PWM_SET_DURATION(I, hi_dur, lo_dur)                                                        \
	do {                                                                                       \
		CMSDK_PWM->PWM##I##_CTRL |= PWM_PWM##I##_CTRL__TOT_DUR__WRITE((hi_dur + lo_dur)) | \
					    PWM_PWM##I##_CTRL__LO_DUR__WRITE((lo_dur));            \
	} while (0)

#define PWM_SET_PARAMS(I, polarity, mode)                                                          \
	do {                                                                                       \
		CMSDK_PWM->PWM##I##_CTRL = PWM_PWM##I##_CTRL__INVERT__WRITE(polarity);             \
	} while (0)
#define PWM(n) CONCAT(CONCAT(PWM_, DT_INST_PROP(n, channel)), _)
#endif

#define PWM_MIN_HZ 123 // Lowest supported freq
#define SYS_CLK_IN_HZ (SYS_CLK_IN_KHZ * 1000)

#define PWM_TOT_DUR_ADJ 1
#define MAX_PWM_INST 8

typedef enum {
	PWM_CONTINUOUS_MODE,
	PWM_COUNTING_MODE,
	PWM_IR_MODE,
	PWM_IR_FIFO_MODE,
} pwm_mode_t;

#define DEV_CFG(dev) ((struct pwm_atm_config const *)(dev)->config)

typedef void (*set_callback_t)(void);

struct pwm_atm_config {
	uint32_t volatile *ctrl;
	set_callback_t config_pins;
	uint32_t max_freq;
	uint32_t min_freq;
#ifdef PWM_PWM0_CTRL__TOT_DUR__READ
	uint8_t max_duty_cycle;
	uint8_t min_duty_cycle;
#endif
};

#ifdef PSEQ_CTRL0__PWM_LATCH_OPEN__CLR
__FAST
static void pwm_pseq_latch_close(void)
{
	WRPR_CTRL_SET(CMSDK_PSEQ, WRPR_CTRL__CLK_ENABLE);
	{
		PSEQ_CTRL0__PWM_LATCH_OPEN__CLR(CMSDK_PSEQ->CTRL0);
	}
	WRPR_CTRL_SET(CMSDK_PSEQ, WRPR_CTRL__CLK_DISABLE);
}
#endif

#ifdef CONFIG_PM
static bool pm_constraint_on;
static void pwm_atm_pm_constraint_set(const struct device *dev)
{
	if (!pm_constraint_on) {
		pm_constraint_on = true;
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
		pm_policy_state_lock_get(PM_STATE_SOFT_OFF, PM_ALL_SUBSTATES);
	}
}

static void pwm_atm_pm_constraint_release(const struct device *dev)
{
	if (pm_constraint_on) {
		pm_constraint_on = false;
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
		pm_policy_state_lock_put(PM_STATE_SOFT_OFF, PM_ALL_SUBSTATES);
	}
}
#endif /* CONFIG_PM */

static void pinmux_config(uint8_t instance, uint8_t polarity, pwm_mode_t mode)
{
	switch (instance) {
	case 0: {
		PWM_SET_PARAMS(0, polarity, mode);
	} break;
	case 1: {
		PWM_SET_PARAMS(1, polarity, mode);
	} break;
	case 2: {
		PWM_SET_PARAMS(2, polarity, mode);
	} break;
	case 3: {
		PWM_SET_PARAMS(3, polarity, mode);
	} break;
	case 4: {
		PWM_SET_PARAMS(4, polarity, mode);
	} break;
	case 5: {
		PWM_SET_PARAMS(5, polarity, mode);
	} break;
	case 6: {
		PWM_SET_PARAMS(6, polarity, mode);
	} break;
	case 7: {
		PWM_SET_PARAMS(7, polarity, mode);
	} break;
	default: {
		ASSERT_INFO(0, instance, 0);
	} break;
	}

#ifdef PSEQ_CTRL0__PWM_LATCH_OPEN__CLR
	pwm_pseq_latch_close();
#endif
}

static void pwm_set_duration(uint8_t instance, uint32_t hi_dur, uint32_t lo_dur)
{
	switch (instance) {
	case 0:
		PWM_SET_DURATION(0, hi_dur, lo_dur);
		break;
	case 1:
		PWM_SET_DURATION(1, hi_dur, lo_dur);
		break;
	case 2:
		PWM_SET_DURATION(2, hi_dur, lo_dur);
		break;
	case 3:
		PWM_SET_DURATION(3, hi_dur, lo_dur);
		break;
	case 4:
		PWM_SET_DURATION(4, hi_dur, lo_dur);
		break;
	case 5:
		PWM_SET_DURATION(5, hi_dur, lo_dur);
		break;
	case 6:
		PWM_SET_DURATION(6, hi_dur, lo_dur);
		break;
	case 7:
		PWM_SET_DURATION(7, hi_dur, lo_dur);
		break;
	default:
		ASSERT_INFO(0, instance, 0);
		break;
	}
}

void pwm_enable(uint8_t instance)
{
	switch (instance) {
	case 0:
		PWM_PWM0_CTRL__OK_TO_RUN__SET(CMSDK_PWM->PWM0_CTRL);
		break;
	case 1:
		PWM_PWM1_CTRL__OK_TO_RUN__SET(CMSDK_PWM->PWM1_CTRL);
		break;
	case 2:
		PWM_PWM2_CTRL__OK_TO_RUN__SET(CMSDK_PWM->PWM2_CTRL);
		break;
	case 3:
		PWM_PWM3_CTRL__OK_TO_RUN__SET(CMSDK_PWM->PWM3_CTRL);
		break;
	case 4:
		PWM_PWM4_CTRL__OK_TO_RUN__SET(CMSDK_PWM->PWM4_CTRL);
		break;
	case 5:
		PWM_PWM5_CTRL__OK_TO_RUN__SET(CMSDK_PWM->PWM5_CTRL);
		break;
	case 6:
		PWM_PWM6_CTRL__OK_TO_RUN__SET(CMSDK_PWM->PWM6_CTRL);
		break;
	case 7:
		PWM_PWM7_CTRL__OK_TO_RUN__SET(CMSDK_PWM->PWM7_CTRL);
		break;
	default:
		ASSERT_INFO(0, instance, 0);
		break;
	}
	return;
}

static int pwm_atm_get_cycles_per_sec(struct device const *dev, uint32_t pwm, uint64_t *cycles)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pwm);
	*cycles = SYS_CLK_IN_HZ;

	return 0;
}

static int pwm_atm_set_cycles(struct device const *dev, uint32_t channel, uint32_t period_cycles,
			      uint32_t pulse_cycles, pwm_flags_t flags)
{
	if (channel >= MAX_PWM_INST) {
		LOG_ERR("Invalid channel. Received (%d)", channel);
		return -EINVAL;
	}

	if ((period_cycles == 0U) || (pulse_cycles > period_cycles)) {
		LOG_ERR("Invalid combination of pulse and period cycles. Received: %d %d",
			pulse_cycles, period_cycles);
		return -EINVAL;
	}

#ifdef PWM_PWM0_CTRL__TOT_DUR__READ
	uint32_t duty_cycle = (pulse_cycles * 100) / period_cycles;
	uint32_t max_duty_cycle = DEV_CFG(dev)->max_duty_cycle;
	uint32_t min_duty_cycle = DEV_CFG(dev)->min_duty_cycle;

	if (duty_cycle > max_duty_cycle) {
		/* fallback to minimum duty cycle */
		LOG_WRN("Unsupported duty cycle %d, falling back to max duty cycle %d\n",
			duty_cycle, max_duty_cycle);
		duty_cycle = max_duty_cycle;
	}

	if (duty_cycle < min_duty_cycle) {
		/* fallback to minimum duty cycle */
		LOG_WRN("Unsupported duty cycle %d, falling back to min duty cycle %d\n",
			duty_cycle, min_duty_cycle);
		duty_cycle = min_duty_cycle;
	}
#endif // PWM_PWM0_CTRL__TOT_DUR__READ

	/* Scale the input to maintain the duty cycle if the exact timing cannot be accomodated  */
	uint32_t max_cycles = DEV_CFG(dev)->max_freq;

	if (period_cycles > max_cycles) {
#ifdef PWM_PWM0_CTRL__TOT_DUR__READ
		pulse_cycles = PULSE_CYCLES_FROM_DUTY_CYCLE(max_cycles, max_duty_cycle);
#else
		pulse_cycles *= ((float)max_cycles / (float)period_cycles);
#endif // PWM_PWM0_CTRL__TOT_DUR__READ
		period_cycles = max_cycles;
	}

	uint32_t min_cycles = DEV_CFG(dev)->min_freq;

	if (period_cycles < min_cycles) {
#ifdef PWM_PWM0_CTRL__TOT_DUR__READ
		pulse_cycles = PULSE_CYCLES_FROM_DUTY_CYCLE(min_cycles, min_duty_cycle);
#else
		pulse_cycles *= ((float)min_cycles / (float)period_cycles);
#endif // PWM_PWM0_CTRL__TOT_DUR__READ
		period_cycles = min_cycles;
	}

#ifdef PWM_PWM0_CTRL__TOT_DUR__READ
	/* Convert to period cycles to kilohertz */
	period_cycles = period_cycles / 1000;

	/* Convert to MPC Clock Cycles */
	uint32_t mpc_clk_period_cycles = (SYS_CLK_IN_KHZ / period_cycles);
	uint32_t mpc_clk_pulse_cycles = (mpc_clk_period_cycles * duty_cycle) / 100;
	uint32_t tot_dur = mpc_clk_period_cycles - PWM_TOT_DUR_ADJ;
	uint16_t hi_dur = mpc_clk_pulse_cycles - PWM_TOT_DUR_ADJ;
	uint16_t lo_dur = tot_dur - hi_dur;
#else
	uint32_t tot_dur = period_cycles - PWM_TOT_DUR_ADJ;
	uint16_t lo_dur = tot_dur - (((float)pulse_cycles / (float)period_cycles) * tot_dur);
	uint16_t hi_dur = tot_dur - lo_dur;
#endif // PWM_PMW0_CTRL__TOT_DUR__READ

#ifdef CONFIG_PM
	if (pulse_cycles) {
		pwm_atm_pm_constraint_set(dev);
	} else {
		pwm_atm_pm_constraint_release(dev);
	}
#endif // CONFIG_PM

	pinmux_config(channel, 0, PWM_CONTINUOUS_MODE);
	pwm_set_duration(channel, hi_dur, lo_dur);

	pwm_enable(channel);

	return 0;
}

static int pwm_atm_init(struct device const *dev)
{
	struct pwm_atm_config const *config = DEV_CFG(dev);

	WRPR_CTRL_SET(CMSDK_PWM, PWM_CLK_CTRL);
	config->config_pins();

	return 0;
}

static struct pwm_driver_api const pwm_atm_driver_api = {
	.set_cycles = pwm_atm_set_cycles,
	.get_cycles_per_sec = pwm_atm_get_cycles_per_sec,
};

#define PWM_CTRL(n) CONCAT(CONCAT(&CMSDK_PWM->PWM, DT_INST_PROP(n, channel)), _CTRL)
#define PWM_MAX_FREQ(n) DT_PROP(DT_NODELABEL(pwm##n), max_frequency)
#define PWM_MIN_FREQ(n) DT_PROP(DT_NODELABEL(pwm##n), min_frequency)
#define PWM_MAX_DUTY_CYCLE(n) DT_PROP(DT_NODELABEL(pwm##n), max_duty_cycle)
#define PWM_MIN_DUTY_CYCLE(n) DT_PROP(DT_NODELABEL(pwm##n), min_duty_cycle)
#define PWM_DEVICE_INIT(n)                                                                         \
	static void pwm_atm_config_pins_##n(void)                                                  \
	{                                                                                          \
		PIN_SELECT(DT_INST_PROP(n, pin), PWM(n));                                          \
	}                                                                                          \
	static struct pwm_atm_config const pwm_atm_config_##n = {                                  \
		.ctrl = PWM_CTRL(n),                                                               \
		.config_pins = pwm_atm_config_pins_##n,                                            \
		.max_freq = PWM_MAX_FREQ(n),                                                       \
		.min_freq = PWM_MIN_FREQ(n),                                                       \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(n, max_duty_cycle),                               \
		           (.max_duty_cycle = PWM_MAX_DUTY_CYCLE(n),))                             \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(n, min_duty_cycle),                               \
		           (.min_duty_cycle = PWM_MIN_DUTY_CYCLE(n),))                             \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &pwm_atm_init, NULL, NULL, &pwm_atm_config_##n, POST_KERNEL,      \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &pwm_atm_driver_api);            \
	BUILD_ASSERT(PWM_CTRL(n) == (uint32_t *)DT_REG_ADDR(                                       \
					    DT_NODELABEL(CONCAT(pwm, DT_INST_PROP(n, channel)))));

DT_INST_FOREACH_STATUS_OKAY(PWM_DEVICE_INIT)
