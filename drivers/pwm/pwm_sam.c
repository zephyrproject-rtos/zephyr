/*
 * Copyright (c) 2019 Aurelien Jarno
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_pwm

#include <zephyr/device.h>
#include <errno.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>
#include <soc.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_sam, CONFIG_PWM_LOG_LEVEL);

/* Some SoCs use a slightly different naming scheme */
#if !defined(PWMCHNUM_NUMBER) && defined(PWMCH_NUM_NUMBER)
#define PWMCHNUM_NUMBER PWMCH_NUM_NUMBER
#endif

struct sam_pwm_config {
	Pwm *regs;
	const struct atmel_sam_pmc_config clock_cfg;
	const struct pinctrl_dev_config *pcfg;
	uint8_t prescaler;
	uint8_t divider;
};

static int sam_pwm_get_cycles_per_sec(const struct device *dev,
				      uint32_t channel, uint64_t *cycles)
{
	const struct sam_pwm_config *config = dev->config;
	uint8_t prescaler = config->prescaler;
	uint8_t divider = config->divider;

	*cycles = SOC_ATMEL_SAM_MCK_FREQ_HZ /
		  ((1 << prescaler) * divider);

	return 0;
}

static int sam_pwm_set_cycles(const struct device *dev, uint32_t channel,
			      uint32_t period_cycles, uint32_t pulse_cycles,
			      pwm_flags_t flags)
{
	const struct sam_pwm_config *config = dev->config;

	Pwm * const pwm = config->regs;
	uint32_t cmr;

	if (channel >= PWMCHNUM_NUMBER) {
		return -EINVAL;
	}

	if (period_cycles == 0U) {
		return -ENOTSUP;
	}

	if (period_cycles > 0xffff) {
		return -ENOTSUP;
	}

	/* Select clock A */
	cmr = PWM_CMR_CPRE_CLKA;

	if ((flags & PWM_POLARITY_MASK) == PWM_POLARITY_NORMAL) {
		cmr |= PWM_CMR_CPOL;
	}

	/* Disable the output if changing polarity (or clock) */
	if (pwm->PWM_CH_NUM[channel].PWM_CMR != cmr) {
		pwm->PWM_DIS = 1 << channel;

		pwm->PWM_CH_NUM[channel].PWM_CMR = cmr;
		pwm->PWM_CH_NUM[channel].PWM_CPRD = period_cycles;
		pwm->PWM_CH_NUM[channel].PWM_CDTY = pulse_cycles;
	} else {
		/* Update period and pulse using the update registers, so that the
		 * change is triggered at the next PWM period.
		 */
		pwm->PWM_CH_NUM[channel].PWM_CPRDUPD = period_cycles;
		pwm->PWM_CH_NUM[channel].PWM_CDTYUPD = pulse_cycles;
	}

	/* Enable the output */
	pwm->PWM_ENA = 1 << channel;

	return 0;
}

static int sam_pwm_init(const struct device *dev)
{
	const struct sam_pwm_config *config = dev->config;

	Pwm * const pwm = config->regs;
	uint8_t prescaler = config->prescaler;
	uint8_t divider = config->divider;
	int retval;

	/* FIXME: way to validate prescaler & divider */

	/* Enable PWM clock in PMC */
	(void)clock_control_on(SAM_DT_PMC_CONTROLLER,
			       (clock_control_subsys_t)&config->clock_cfg);

	retval = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (retval < 0) {
		return retval;
	}

	/* Configure the clock A that will be used by all 4 channels */
	pwm->PWM_CLK = PWM_CLK_PREA(prescaler) | PWM_CLK_DIVA(divider);

	return 0;
}

static const struct pwm_driver_api sam_pwm_driver_api = {
	.set_cycles = sam_pwm_set_cycles,
	.get_cycles_per_sec = sam_pwm_get_cycles_per_sec,
};

#define SAM_INST_INIT(inst)						\
	PINCTRL_DT_INST_DEFINE(inst);					\
	static const struct sam_pwm_config sam_pwm_config_##inst = {	\
		.regs = (Pwm *)DT_INST_REG_ADDR(inst),			\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),		\
		.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(inst),		\
		.prescaler = DT_INST_PROP(inst, prescaler),		\
		.divider = DT_INST_PROP(inst, divider),			\
	};								\
									\
	DEVICE_DT_INST_DEFINE(inst,					\
			    &sam_pwm_init, NULL,			\
			    NULL, &sam_pwm_config_##inst,		\
			    POST_KERNEL,				\
			    CONFIG_PWM_INIT_PRIORITY,			\
			    &sam_pwm_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SAM_INST_INIT)
