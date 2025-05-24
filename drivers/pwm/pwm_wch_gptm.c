/*
 * Copyright (c) 2025 Michael Hope <michaelh@juju.nz>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_gptm_pwm

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/dt-bindings/pwm/pwm.h>

#include <hal_ch32fun.h>

/* Each of the 4 channels uses 1 byte of CHCTLR{1,2} */
#define CHCTLR_CHANNEL_MASK    0xFF
/* 'Invalid', i.e. low before any inversion. */
#define CHCTLR_OCXM_INVALID    0x04
/* 'Valid', i.e. high before any inversion. */
#define CHCTLR_OCXM_VALID      0x05
#define CHCTLR_OCXM_PWM_MODE1  0x06
/* Start bit offset for OC{1,3}M */
#define CHCTLR_OCXM_ODD_SHIFT  4
/* Start bit offset for OC{2,4}M */
#define CHCTLR_OCXM_EVEN_SHIFT 12
/* Each of the 4 channels uses 1 nibble of CCER */
#define CCER_MASK              (TIM_CC1P | TIM_CC1E)

struct pwm_wch_gptm_config {
	TIM_TypeDef *regs;
	const struct device *clock_dev;
	uint8_t clock_id;
	uint16_t prescaler;
	const struct pinctrl_dev_config *pin_cfg;
};

static int pwm_wch_gptm_set_cycles(const struct device *dev, uint32_t channel,
				   uint32_t period_cycles, uint32_t pulse_cycles, pwm_flags_t flags)
{
	const struct pwm_wch_gptm_config *config = dev->config;
	TIM_TypeDef *regs = config->regs;
	uint16_t ocxm;

	if (period_cycles > UINT16_MAX) {
		return -EINVAL;
	}

	if (period_cycles == 0) {
		ocxm = CHCTLR_OCXM_INVALID;
	} else if (pulse_cycles >= period_cycles) {
		/*
		 * If pulse_cycles == period_cycles then there is a one cycle glitch in the output.
		 * Mitigate by setting the output to 'always on'.
		 */
		ocxm = CHCTLR_OCXM_VALID;
	} else {
		ocxm = CHCTLR_OCXM_PWM_MODE1;
	}

	switch (channel) {
	case 0:
		regs->CH1CVR = pulse_cycles;
		regs->CHCTLR1 = (regs->CHCTLR1 & ~TIM_OC1M) | (ocxm << CHCTLR_OCXM_ODD_SHIFT);
		break;
	case 1:
		regs->CH2CVR = pulse_cycles;
		regs->CHCTLR1 = (regs->CHCTLR1 & ~TIM_OC2M) | (ocxm << CHCTLR_OCXM_EVEN_SHIFT);
		break;
	case 2:
		regs->CH3CVR = pulse_cycles;
		regs->CHCTLR2 = (regs->CHCTLR2 & ~TIM_OC3M) | (ocxm << CHCTLR_OCXM_ODD_SHIFT);
		break;
	case 3:
		regs->CH4CVR = pulse_cycles;
		regs->CHCTLR2 = (regs->CHCTLR2 & ~TIM_OC4M) | (ocxm << CHCTLR_OCXM_EVEN_SHIFT);
		break;
	default:
		return -EINVAL;
	}

	if (period_cycles != 0) {
		regs->ATRLR = period_cycles;
	}

	/* Set the polarity and enable */
	uint16_t shift = 4 * channel;

	if ((flags & PWM_POLARITY_INVERTED) != 0) {
		regs->CCER =
			(regs->CCER & ~(CCER_MASK << shift)) | ((TIM_CC1P | TIM_CC1E) << shift);
	} else {
		regs->CCER = (regs->CCER & ~(CCER_MASK << shift)) | (TIM_CC1E << shift);
	}

	return 0;
}

static int pwm_wch_gptm_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					   uint64_t *cycles)
{
	const struct pwm_wch_gptm_config *config = dev->config;
	clock_control_subsys_t clock_sys = (clock_control_subsys_t *)(uintptr_t)config->clock_id;
	uint32_t clock_rate;
	int err;

	err = clock_control_get_rate(config->clock_dev, clock_sys, &clock_rate);
	if (err != 0) {
		return err;
	}

	*cycles = clock_rate / (config->prescaler + 1);

	return 0;
}

static const struct pwm_driver_api pwm_wch_gptm_driver_api = {
	.set_cycles = pwm_wch_gptm_set_cycles,
	.get_cycles_per_sec = pwm_wch_gptm_get_cycles_per_sec,
};

static int pwm_wch_gptm_init(const struct device *dev)
{
	const struct pwm_wch_gptm_config *config = dev->config;
	TIM_TypeDef *regs = config->regs;
	int err;

	clock_control_on(config->clock_dev, (clock_control_subsys_t *)(uintptr_t)config->clock_id);

	err = pinctrl_apply_state(config->pin_cfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		return err;
	}

	/* Disable and configure the counter */
	regs->CTLR1 = TIM_ARPE;
	regs->PSC = config->prescaler;
	regs->CTLR1 |= TIM_CEN;

	return 0;
}

#define PWM_WCH_GPTM_INIT(idx)                                                                     \
	PINCTRL_DT_INST_DEFINE(idx);                                                               \
                                                                                                   \
	static const struct pwm_wch_gptm_config pwm_wch_gptm_##idx##_config = {                    \
		.regs = (TIM_TypeDef *)DT_REG_ADDR(DT_INST_PARENT(idx)),                           \
		.prescaler = DT_PROP(DT_INST_PARENT(idx), prescaler),                              \
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_INST_PARENT(idx))),                   \
		.clock_id = DT_CLOCKS_CELL(DT_INST_PARENT(idx), id),                               \
		.pin_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                                    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, &pwm_wch_gptm_init, NULL, NULL, &pwm_wch_gptm_##idx##_config,   \
			      POST_KERNEL, CONFIG_PWM_INIT_PRIORITY, &pwm_wch_gptm_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_WCH_GPTM_INIT)
