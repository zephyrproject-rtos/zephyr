/*
 * Copyright (c) 2023 Zephyr Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc13xx_cc26xx_timer_pwm

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pwm.h>

#include <driverlib/gpio.h>
#include <driverlib/prcm.h>
#include <driverlib/timer.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME pwm_cc13xx_cc26xx_timer
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_PWM_LOG_LEVEL);

/* TODO: Clock frequency can be settable via KConfig, see TOP:PRCM:GPTCLKDIV */
#define CPU_FREQ ((uint32_t)DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency))

/* GPT peripherals in 16 bit mode have maximum 24 counter bits incl. the
 * prescaler. Count is set to (2^24 - 2) to allow for a glitch free 100% duty
 * cycle at max. period count.
 */
#define PWM_COUNT_MAX      0xFFFFFE
#define PWM_INITIAL_PERIOD PWM_COUNT_MAX
#define PWM_INITIAL_DUTY   0U /* initially off */

struct pwm_cc13xx_cc26xx_data {
};

struct pwm_cc13xx_cc26xx_config {
	const uint32_t gpt_base; /* GPT register base address */
	const struct pinctrl_dev_config *pcfg;

	LOG_INSTANCE_PTR_DECLARE(log);
};

static void write_value(const struct pwm_cc13xx_cc26xx_config *config, uint32_t value,
			uint32_t prescale_register, uint32_t value_register)
{
	/* Upper byte represents the prescaler value. */
	uint8_t prescaleValue = 0xff & (value >> 16);

	HWREG(config->gpt_base + prescale_register) = prescaleValue;

	/* The remaining bytes represent the load / match value. */
	HWREG(config->gpt_base + value_register) = value & 0xffff;
}

static int set_period_and_pulse(const struct pwm_cc13xx_cc26xx_config *config, uint32_t period,
				uint32_t pulse)
{
	uint32_t match_value = pulse;

	if (pulse == 0U) {
		TimerDisable(config->gpt_base, TIMER_B);
#ifdef CONFIG_PM
		Power_releaseConstraint(PowerCC26XX_DISALLOW_STANDBY);
#endif
		match_value = period + 1;
	}

	/* Fail if period is out of range */
	if ((period > PWM_COUNT_MAX) || (period == 0)) {
		LOG_ERR("Period (%d) is out of range.", period);
		return -EINVAL;
	}

	/* Compare to new period and fail if invalid */
	if (period < (match_value - 1) || (match_value < 0)) {
		LOG_ERR("Period (%d) is shorter than pulse (%d).", period, pulse);
		return -EINVAL;
	}

	/* Store new period and update timer */
	write_value(config, period, GPT_O_TBPR, GPT_O_TBILR);
	write_value(config, match_value, GPT_O_TBPMR, GPT_O_TBMATCHR);

	if (pulse > 0U) {
#ifdef CONFIG_PM
		Power_setConstraint(PowerCC26XX_DISALLOW_STANDBY);
#endif
		TimerEnable(config->gpt_base, TIMER_B);
	}

	LOG_DBG("Period and pulse successfully set.");
	return 0;
}

static int set_cycles(const struct device *dev, uint32_t channel, uint32_t period, uint32_t pulse,
		      pwm_flags_t flags)
{
	const struct pwm_cc13xx_cc26xx_config *config = dev->config;

	if (channel != 0) {
		return -EIO;
	}

	if (flags & PWM_POLARITY_INVERTED) {
		HWREG(config->gpt_base + GPT_O_CTL) |= GPT_CTL_TBPWML_INVERTED;
	} else {
		HWREG(config->gpt_base + GPT_O_CTL) |= GPT_CTL_TBPWML_NORMAL;
	}

	set_period_and_pulse(config, period, pulse);

	return 0;
}

static int get_cycles_per_sec(const struct device *dev, uint32_t channel, uint64_t *cycles)
{
	if (channel > 0) {
		return -EIO;
	}

	if (cycles) {
		*cycles = CPU_FREQ;
	}

	return 0;
}

static const struct pwm_driver_api pwm_driver_api = {
	.set_cycles = set_cycles,
	.get_cycles_per_sec = get_cycles_per_sec,
};

#ifdef CONFIG_PM
static int get_timer_inst_number(const struct pwm_cc13xx_cc26xx_config *config)
{
	switch (config->gpt_base) {
	case GPT0_BASE:
		return 0;
	case GPT1_BASE:
		return 1;
	case GPT2_BASE:
		return 2;
	case GPT3_BASE:
		return 3;
	default:
		CODE_UNREACHABLE;
	}
}
#else
static int get_timer_peripheral(const struct pwm_cc13xx_cc26xx_config *config)
{
	switch (config->gpt_base) {
	case GPT0_BASE:
		return PRCM_PERIPH_TIMER0;
	case GPT1_BASE:
		return PRCM_PERIPH_TIMER1;
	case GPT2_BASE:
		return PRCM_PERIPH_TIMER2;
	case GPT3_BASE:
		return PRCM_PERIPH_TIMER3;
	default:
		CODE_UNREACHABLE;
	}
}
#endif /* CONFIG_PM */

static int init_pwm(const struct device *dev)
{
	const struct pwm_cc13xx_cc26xx_config *config = dev->config;
	pinctrl_soc_pin_t pin = config->pcfg->states[0].pins[0];
	int ret;

#ifdef CONFIG_PM
	/* Set dependency on gpio resource to turn on power domains */
	Power_setDependency(get_timer_inst_number(config));
#else
	/* Enable peripheral power domain. */
	PRCMPowerDomainOn(PRCM_DOMAIN_PERIPH);

	/* Enable GPIO peripheral. */
	PRCMPeripheralRunEnable(get_timer_peripheral(config));
	PRCMPeripheralSleepEnable(get_timer_peripheral(config));
	PRCMPeripheralDeepSleepEnable(get_timer_peripheral(config));

	/* Load PRCM settings. */
	PRCMLoadSet();
	while (!PRCMLoadGet()) {
		continue;
	}
#endif /* CONFIG_PM */

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("failed to setup PWM pinctrl");
		return ret;
	}

	/* Configures the PWM idle output level.
	 *
	 * TODO: Make PWM idle high/low configurable via custom DT PWM flag.
	 */
	GPIO_writeDio(pin.pin, 0);

	GPIO_setOutputEnableDio(pin.pin, GPIO_OUTPUT_ENABLE);

	/* Peripheral should not be accessed until power domain is on. */
	while (PRCMPowerDomainsAllOn(PRCM_DOMAIN_PERIPH) != PRCM_DOMAIN_POWER_ON) {
		continue;
	}

	TimerDisable(config->gpt_base, TIMER_B);

	HWREG(config->gpt_base + GPT_O_CFG) = GPT_CFG_CFG_16BIT_TIMER;
	/* Stall timer when debugging.
	 *
	 * TODO: Make debug stall configurable via custom DT prop.
	 */
	HWREG(config->gpt_base + GPT_O_CTL) |= GPT_CTL_TBSTALL;

	HWREG(config->gpt_base + GPT_O_TBMR) = GPT_TBMR_TBAMS_PWM | GPT_TBMR_TBMRSU_TOUPDATE |
					       GPT_TBMR_TBPWMIE_EN | GPT_TBMR_TBMR_PERIODIC;

	set_period_and_pulse(config, PWM_INITIAL_PERIOD, PWM_INITIAL_DUTY);

	return 0;
}

#define DT_TIMER(idx)           DT_INST_PARENT(idx)
#define DT_TIMER_BASE_ADDR(idx) (DT_REG_ADDR(DT_TIMER(idx)))

#define PWM_DEVICE_INIT(idx)                                                                       \
	PINCTRL_DT_INST_DEFINE(idx);                                                               \
	LOG_INSTANCE_REGISTER(LOG_MODULE_NAME, idx, CONFIG_PWM_LOG_LEVEL);                         \
	static const struct pwm_cc13xx_cc26xx_config pwm_cc13xx_cc26xx_##idx##_config = {          \
		.gpt_base = DT_TIMER_BASE_ADDR(idx),                                               \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                                       \
		LOG_INSTANCE_PTR_INIT(log, LOG_MODULE_NAME, idx)};                                 \
                                                                                                   \
	static struct pwm_cc13xx_cc26xx_data pwm_cc13xx_cc26xx_##idx##_data;                       \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, init_pwm, NULL, &pwm_cc13xx_cc26xx_##idx##_data,                \
			      &pwm_cc13xx_cc26xx_##idx##_config, POST_KERNEL,                      \
			      CONFIG_PWM_INIT_PRIORITY, &pwm_driver_api)

DT_INST_FOREACH_STATUS_OKAY(PWM_DEVICE_INIT);
