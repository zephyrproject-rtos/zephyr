/*
 * Copyright (c) 2019 Aurelien Jarno
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_pwm

#include <device.h>
#include <errno.h>
#include <drivers/pwm.h>
#include <soc.h>

#define LOG_LEVEL CONFIG_PWM_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(pwm_sam);

struct sam_pwm_config {
	Pwm *regs;
	u32_t id;
	u8_t prescaler;
	u8_t divider;
};

#define DEV_CFG(dev) \
	((const struct sam_pwm_config * const)(dev)->config_info)

static int sam_pwm_get_cycles_per_sec(struct device *dev, u32_t pwm,
				      u64_t *cycles)
{
	u8_t prescaler = DEV_CFG(dev)->prescaler;
	u8_t divider = DEV_CFG(dev)->divider;

	*cycles = SOC_ATMEL_SAM_MCK_FREQ_HZ /
		  ((1 << prescaler) * divider);

	return 0;
}

static int sam_pwm_pin_set(struct device *dev, u32_t ch,
			   u32_t period_cycles, u32_t pulse_cycles,
			   pwm_flags_t flags)
{
	Pwm *const pwm = DEV_CFG(dev)->regs;

	if (ch >= PWMCHNUM_NUMBER) {
		return -EINVAL;
	}

	if (flags) {
		/* PWM polarity not supported (yet?) */
		return -ENOTSUP;
	}

	if (period_cycles == 0U || pulse_cycles > period_cycles) {
		return -EINVAL;
	}

	if (period_cycles > 0xffff) {
		return -ENOTSUP;
	}

	/* Select clock A */
	pwm->PWM_CH_NUM[ch].PWM_CMR = PWM_CMR_CPRE_CLKA_Val;

	/* Update period and pulse using the update registers, so that the
	 * change is triggered at the next PWM period.
	 */
	pwm->PWM_CH_NUM[ch].PWM_CPRDUPD = period_cycles;
	pwm->PWM_CH_NUM[ch].PWM_CDTYUPD = pulse_cycles;

	/* Enable the output */
	pwm->PWM_ENA = 1 << ch;

	return 0;
}

static int sam_pwm_init(struct device *dev)
{
	Pwm *const pwm = DEV_CFG(dev)->regs;
	u32_t id = DEV_CFG(dev)->id;
	u8_t prescaler = DEV_CFG(dev)->prescaler;
	u8_t divider = DEV_CFG(dev)->divider;

	/* FIXME: way to validate prescaler & divider */

	/* Enable the PWM peripheral */
	soc_pmc_peripheral_enable(id);

	/* Configure the clock A that will be used by all 4 channels */
	pwm->PWM_CLK = PWM_CLK_PREA(prescaler) | PWM_CLK_DIVA(divider);

	return 0;
}

static const struct pwm_driver_api sam_pwm_driver_api = {
	.pin_set = sam_pwm_pin_set,
	.get_cycles_per_sec = sam_pwm_get_cycles_per_sec,
};

#define SAM_INST_INIT(inst)						\
	static const struct sam_pwm_config sam_pwm_config_##inst = {	\
		.regs = (Pwm *)DT_INST_REG_ADDR(inst),			\
		.id = DT_INST_PROP(inst, peripheral_id),		\
		.prescaler = DT_INST_PROP(inst, prescaler),		\
		.divider = DT_INST_PROP(inst, divider),			\
	};								\
									\
	DEVICE_AND_API_INIT(sam_pwm_##inst, DT_INST_LABEL(inst),	\
			    &sam_pwm_init,				\
			    NULL, &sam_pwm_config_##inst,		\
			    POST_KERNEL,				\
			    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			    &sam_pwm_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SAM_INST_INIT)
