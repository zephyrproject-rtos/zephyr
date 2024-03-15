/*
 * Copyright (c) 2024 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ene_kb1200_pwm

#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pinctrl.h>
#include <reg/pwm.h>

/* Device config */
struct pwm_kb1200_config {
	/* pwm controller base address */
	struct pwm_regs *pwm;
	const struct pinctrl_dev_config *pcfg;
};

/* Driver data */
struct pwm_kb1200_data {
	/* PWM cycles per second */
	uint32_t cycles_per_sec;
};

/* PWM api functions */
static int pwm_kb1200_set_cycles(const struct device *dev, uint32_t channel, uint32_t period_cycles,
				 uint32_t pulse_cycles, pwm_flags_t flags)
{
	/* Single channel for each pwm device */
	ARG_UNUSED(channel);
	const struct pwm_kb1200_config *config = dev->config;
	int prescaler;
	uint32_t high_len;
	uint32_t cycle_len;

	/*
	 * Calculate PWM prescaler that let period_cycles map to
	 * maximum pwm period cycles and won't exceed it.
	 * Then prescaler = ceil (period_cycles / pwm_max_period_cycles)
	 */
	prescaler = DIV_ROUND_UP(period_cycles, PWM_MAX_CYCLES);
	if (prescaler > PWM_MAX_PRESCALER) {
		return -EINVAL;
	}

	/* If pulse_cycles is 0, switch PWM off and return. */
	if (pulse_cycles == 0) {
		config->pwm->PWMCFG &= ~PWM_ENABLE;
		return 0;
	}

	high_len = (pulse_cycles / prescaler);
	cycle_len = (period_cycles / prescaler);

	/* Select PWM inverted polarity (ie. active-low pulse). */
	if (flags & PWM_POLARITY_INVERTED) {
		high_len = cycle_len - high_len;
	}

	/* Set PWM prescaler. */
	config->pwm->PWMCFG = (config->pwm->PWMCFG & ~GENMASK(13, 8)) | ((prescaler - 1) << 8);

	/*
	 * period_cycles: PWM Cycle Length
	 * pulse_cycles : PWM High Length
	 */
	config->pwm->PWMHIGH = high_len;
	config->pwm->PWMCYC = cycle_len;

	/* Start pwm */
	config->pwm->PWMCFG |= PWM_ENABLE;

	return 0;
}

static int pwm_kb1200_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					 uint64_t *cycles)
{
	/* Single channel for each pwm device */
	ARG_UNUSED(channel);
	ARG_UNUSED(dev);

	if (cycles) {
		/* User does not have to know about lowest clock,
		 * the driver will select the most relevant one.
		 */
		*cycles = PWM_INPUT_FREQ_HI; /*32Mhz*/
	}
	return 0;
}

static const struct pwm_driver_api pwm_kb1200_driver_api = {
	.set_cycles = pwm_kb1200_set_cycles,
	.get_cycles_per_sec = pwm_kb1200_get_cycles_per_sec,
};

static int pwm_kb1200_init(const struct device *dev)
{
	int ret;
	const struct pwm_kb1200_config *config = dev->config;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		return ret;
	}
	config->pwm->PWMCFG = PWM_SOURCE_CLK_32M | PWM_RULE1 | PWM_PUSHPULL;

	return 0;
}

#define KB1200_PWM_INIT(inst)                                                                      \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static const struct pwm_kb1200_config pwm_kb1200_cfg_##inst = {                            \
		.pwm = (struct pwm_regs *)DT_INST_REG_ADDR(inst),                                  \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
	};                                                                                         \
	static struct pwm_kb1200_data pwm_kb1200_data_##inst;                                      \
	DEVICE_DT_INST_DEFINE(inst, &pwm_kb1200_init, NULL, &pwm_kb1200_data_##inst,               \
			      &pwm_kb1200_cfg_##inst, PRE_KERNEL_1, CONFIG_PWM_INIT_PRIORITY,      \
			      &pwm_kb1200_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KB1200_PWM_INIT)
