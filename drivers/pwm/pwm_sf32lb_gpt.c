/*
 * Copyright (c) 2025, Qingsong Gou <gouqs@hotmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_gpt_pwm

#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/clock_control/sf32lb.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

#include <register.h>

#define GPT_CCMRX(ch) (GPT_CCMR1 + ((ch) >> 1U))
#define CCRX(ch)      (GPT_CCR1 + ((ch) << 2U))

#define GPT_CR1   offsetof(GPT_TypeDef, CR1)
#define GPT_PSC   offsetof(GPT_TypeDef, PSC)
#define GPT_ARR   offsetof(GPT_TypeDef, ARR)
#define GPT_CCR1  offsetof(GPT_TypeDef, CCR1)
#define GPT_CCER  offsetof(GPT_TypeDef, CCER)
#define GPT_CCMR1 offsetof(GPT_TypeDef, CCMR1)
#define GPT_EGR   offsetof(GPT_TypeDef, EGR)

#define GPT_CCMR1_OC1M_1 (0x2U << GPT_CCMR1_OC1M_Pos)
#define GPT_CCMR1_OC1M_2 (0x4U << GPT_CCMR1_OC1M_Pos)

#define GPT_OCMODE_PWM1 (GPT_CCMR1_OC1M_1 | GPT_CCMR1_OC1M_2)

#define MAX_CH_NUM (4U)

LOG_MODULE_REGISTER(pwm_sf32lb, CONFIG_PWM_LOG_LEVEL);

struct pwm_sf32lb_config {
	uintptr_t base;
	const struct pinctrl_dev_config *pcfg;
	struct sf32lb_clock_dt_spec clock;
	uint16_t prescaler;
};

static int pwm_sf32lb_set_cycles(const struct device *dev, uint32_t channel, uint32_t period_cycles,
				 uint32_t pulse_cycles, pwm_flags_t flags)
{
	const struct pwm_sf32lb_config *config = dev->config;
	uint8_t pos;

	pos = channel * 4U;

	if (channel >= MAX_CH_NUM) {
		LOG_ERR("Invalid PWM channel: %u. Must be 0-3.", channel);
		return -EINVAL;
	}

	LOG_DBG("Setting PWM period_cycles: %d, pulse_cycles: %d", period_cycles, pulse_cycles);

	if ((period_cycles > UINT16_MAX) || (pulse_cycles > UINT16_MAX)) {
		LOG_ERR("Cannot set PWM output, value exceeds 16-bit timer limit.");
		return -ENOTSUP;
	}

	if (period_cycles == 0U) {
		sys_clear_bit(config->base + GPT_CCER, pos);
		return 0;
	}

	sys_clear_bit(config->base + GPT_CCER, pos);
	sys_clear_bits(config->base + GPT_CCER, GPT_CCER_CC1P << pos);

	if (flags & PWM_POLARITY_INVERTED) {
		sys_set_bits(config->base + GPT_CCER, GPT_CCER_CC1P << pos);
	}

	sys_write32(period_cycles - 1, config->base + GPT_ARR);
	sys_write32(pulse_cycles, config->base + CCRX(channel));

	switch (channel) {
	case 0:
		sys_clear_bits(config->base + GPT_CCMRX(channel), GPT_CCMR1_OC1M);
		sys_set_bits(config->base + GPT_CCMRX(channel), GPT_CCMR1_OC1PE);
		sys_set_bits(config->base + GPT_CCMRX(channel), GPT_OCMODE_PWM1);
		break;
	case 1:
		sys_clear_bits(config->base + GPT_CCMRX(channel), GPT_CCMR1_OC2M);
		sys_set_bits(config->base + GPT_CCMRX(channel), GPT_CCMR1_OC2PE);
		sys_set_bits(config->base + GPT_CCMRX(channel), GPT_OCMODE_PWM1);
		break;
	case 2:
		sys_clear_bits(config->base + GPT_CCMRX(channel), GPT_CCMR2_OC3M);
		sys_set_bits(config->base + GPT_CCMRX(channel), GPT_CCMR2_OC3PE);
		sys_set_bits(config->base + GPT_CCMRX(channel), GPT_OCMODE_PWM1);
		break;
	case 3:
		sys_clear_bits(config->base + GPT_CCMRX(channel), GPT_CCMR2_OC4M);
		sys_set_bits(config->base + GPT_CCMRX(channel), GPT_CCMR2_OC4PE);
		sys_set_bits(config->base + GPT_CCMRX(channel), GPT_OCMODE_PWM1);
		break;
	default:
		return -EINVAL;
	}

	sys_set_bit(config->base + GPT_CCER, pos);

	return 0;
}

static int pwm_sf32lb_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					 uint64_t *cycles)
{
	const struct pwm_sf32lb_config *config = dev->config;
	uint32_t clock_freq;
	int ret;

	if (channel >= MAX_CH_NUM) {
		LOG_ERR("Invalid PWM channel: %u. Must be 0-3.", channel);
		return -EINVAL;
	}

	ret = sf32lb_clock_control_get_rate_dt(&config->clock, &clock_freq);
	if (ret < 0) {
		return ret;
	}

	*cycles = (uint64_t)(clock_freq / (config->prescaler + 1U));

	return ret;
}

static int pwm_sf32lb_init(const struct device *dev)
{
	const struct pwm_sf32lb_config *config = dev->config;
	int ret;

	if (!sf32lb_clock_is_ready_dt(&config->clock)) {
		return -ENODEV;
	}

	ret = sf32lb_clock_control_on_dt(&config->clock);
	if (ret < 0) {
		return ret;
	}

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Failed to configure pins");
		return ret;
	}

	sys_write32(config->prescaler, config->base + GPT_PSC);
	sys_set_bit(config->base + GPT_EGR, GPT_EGR_UG_Pos);

	sys_set_bit(config->base + GPT_CR1, GPT_CR1_CEN_Pos);

	return ret;
}

static DEVICE_API(pwm, pwm_sf32lb_driver_api) = {
	.set_cycles = pwm_sf32lb_set_cycles,
	.get_cycles_per_sec = pwm_sf32lb_get_cycles_per_sec,
};

#define PWM_SF32LB_DEFINE(n)                                                                       \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static const struct pwm_sf32lb_config pwm_sf32lb_config_##n = {                            \
		.base = DT_REG_ADDR(DT_INST_PARENT(n)),                                            \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.clock = SF32LB_CLOCK_DT_INST_PARENT_SPEC_GET(n),                                  \
		.prescaler = DT_PROP(DT_INST_PARENT(n), sifli_prescaler),                          \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, pwm_sf32lb_init, NULL, NULL,                                      \
			      &pwm_sf32lb_config_##n, POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,       \
			      &pwm_sf32lb_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_SF32LB_DEFINE)
