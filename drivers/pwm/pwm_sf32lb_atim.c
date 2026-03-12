/*
 * Copyright (c) 2025, Qingsong Gou <gouqs@hotmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_atim_pwm

#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/clock_control/sf32lb.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <register.h>

LOG_MODULE_REGISTER(pwm_sf32lb_atim, CONFIG_PWM_LOG_LEVEL);

#define CR1   offsetof(ATIM_TypeDef, CR1)
#define CR2   offsetof(ATIM_TypeDef, CR2)
#define DIER  offsetof(ATIM_TypeDef, DIER)
#define SR    offsetof(ATIM_TypeDef, SR)
#define EGR   offsetof(ATIM_TypeDef, EGR)
#define CCMR1 offsetof(ATIM_TypeDef, CCMR1)
#define CCMR2 offsetof(ATIM_TypeDef, CCMR2)
#define CCER  offsetof(ATIM_TypeDef, CCER)
#define PSC   offsetof(ATIM_TypeDef, PSC)
#define ARR   offsetof(ATIM_TypeDef, ARR)
#define CCR1  offsetof(ATIM_TypeDef, CCR1)
#define CCR2  offsetof(ATIM_TypeDef, CCR2)
#define CCR3  offsetof(ATIM_TypeDef, CCR3)
#define CCR4  offsetof(ATIM_TypeDef, CCR4)
#define BDTR  offsetof(ATIM_TypeDef, BDTR)

#define ATIM_PWM_MODE1 (6U)

#define CCMRX(ch) ((ch) <= 1 ? CCMR1 : CCMR2)

#define MAX_CH_NUM 4U

struct pwm_sf32lb_atim_config {
	uintptr_t base;
	const struct pinctrl_dev_config *pincfg;
	struct sf32lb_clock_dt_spec clock;
	uint32_t prescaler;
};

static int pwm_sf32lb_atim_set_cycles(const struct device *dev, uint32_t channel,
				      uint32_t period_cycles, uint32_t pulse_cycles,
				      pwm_flags_t flags)
{
	const struct pwm_sf32lb_atim_config *cfg = dev->config;
	uint32_t ccmr;

	if (channel >= MAX_CH_NUM) {
		return -EINVAL;
	}

	/* disable the channel */
	sys_clear_bit(cfg->base + CCER, channel * 4);

	sys_write32(period_cycles - 1, cfg->base + ARR);

	switch (channel) {
	case 0:
		sys_write32(pulse_cycles, cfg->base + CCR1);
		ccmr = sys_read32(cfg->base + CCMRX(channel));
		ccmr &= ~ATIM_CCMR1_OC1M_Msk;
		ccmr |= FIELD_PREP(ATIM_CCMR1_OC1M_Msk, ATIM_PWM_MODE1);
		ccmr |= ATIM_CCMR1_OC1PE;
		sys_write32(ccmr, cfg->base + CCMRX(channel));
		break;
	case 1:
		sys_write32(pulse_cycles, cfg->base + CCR2);
		ccmr = sys_read32(cfg->base + CCMRX(channel));
		ccmr &= ~ATIM_CCMR1_OC2M_Msk;
		ccmr |= FIELD_PREP(ATIM_CCMR1_OC2M_Msk, ATIM_PWM_MODE1);
		ccmr |= ATIM_CCMR1_OC2PE;
		sys_write32(ccmr, cfg->base + CCMRX(channel));
		break;
	case 2:
		sys_write32(pulse_cycles, cfg->base + CCR3);
		ccmr = sys_read32(cfg->base + CCMRX(channel));
		ccmr &= ~ATIM_CCMR2_OC3M_Msk;
		ccmr |= FIELD_PREP(ATIM_CCMR2_OC3M_Msk, ATIM_PWM_MODE1);
		ccmr |= ATIM_CCMR2_OC3PE;
		sys_write32(ccmr, cfg->base + CCMRX(channel));
		break;
	case 3:
		sys_write32(pulse_cycles, cfg->base + CCR4);
		ccmr = sys_read32(cfg->base + CCMRX(channel));
		ccmr &= ~ATIM_CCMR2_OC4M_Msk;
		ccmr |= FIELD_PREP(ATIM_CCMR2_OC4M_Msk, ATIM_PWM_MODE1);
		ccmr |= ATIM_CCMR2_OC4PE;
		sys_write32(ccmr, cfg->base + CCMRX(channel));
		break;
	default:
		return -EINVAL;
	}

	if (flags & PWM_POLARITY_INVERTED) {
		sys_set_bit(cfg->base + CCER, channel * 4 + 1);
	}

	/* enable the channel */
	sys_set_bit(cfg->base + CCER, channel * 4);

	return 0;
}

static int pwm_sf32lb_atim_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					      uint64_t *cycles)
{
	const struct pwm_sf32lb_atim_config *cfg = dev->config;
	uint32_t clk_rate;

	if (sf32lb_clock_control_get_rate_dt(&cfg->clock, &clk_rate)) {
		return -EIO;
	}

	*cycles = clk_rate / (cfg->prescaler + 1);

	return 0;
}

static DEVICE_API(pwm, pwm_sf32lb_atim_api) = {
	.set_cycles = pwm_sf32lb_atim_set_cycles,
	.get_cycles_per_sec = pwm_sf32lb_atim_get_cycles_per_sec,
};

static int pwm_sf32lb_atim_init(const struct device *dev)
{
	const struct pwm_sf32lb_atim_config *cfg = dev->config;
	int err;

	err = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	if (!sf32lb_clock_is_ready_dt(&cfg->clock)) {
		return -ENODEV;
	}

	err = sf32lb_clock_control_on_dt(&cfg->clock);
	if (err < 0) {
		return err;
	}

	sys_write32(cfg->prescaler, cfg->base + PSC);
	sys_set_bit(cfg->base + EGR, ATIM_EGR_UG_Pos);

	/* enable auto-reload preload */
	sys_set_bit(cfg->base + CR1, ATIM_CR1_ARPE_Pos);

	/* enable timer */
	sys_set_bit(cfg->base + CR1, ATIM_CR1_CEN_Pos);
	sys_set_bit(cfg->base + BDTR, ATIM_BDTR_MOE_Pos);

	return err;
}

#define PWM_SF32LB_ATIM_DEFINE(n)                                                                  \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static const struct pwm_sf32lb_atim_config pwm_sf32lb_atim_config_##n = {                  \
		.base = DT_REG_ADDR(DT_INST_PARENT(n)),                                            \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
		.clock = SF32LB_CLOCK_DT_INST_PARENT_SPEC_GET(n),                                  \
		.prescaler = DT_PROP(DT_INST_PARENT(n), sifli_prescaler),                          \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, pwm_sf32lb_atim_init, NULL, NULL,                                 \
			      &pwm_sf32lb_atim_config_##n, POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,  \
			      &pwm_sf32lb_atim_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_SF32LB_ATIM_DEFINE)
