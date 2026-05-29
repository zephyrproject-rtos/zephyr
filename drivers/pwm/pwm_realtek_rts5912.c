/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_rts5912_pwm

#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_rts5912.h>

#include "reg/reg_pwm.h"

LOG_MODULE_REGISTER(pwm, CONFIG_PWM_LOG_LEVEL);

#define PWM_CYCLE_PER_SEC MHZ(50)

struct pwm_rts5912_config {
	volatile struct pwm_regs *pwm_regs;
	uint32_t pwm_clk_grp;
	uint32_t pwm_clk_idx;
	const struct device *clk_dev;
	const struct pinctrl_dev_config *pcfg;
};

static int pwm_rts5912_set_cycles(const struct device *dev, uint32_t channel,
				  uint32_t period_cycles, uint32_t pulse_cycles, pwm_flags_t flags)
{
	const struct pwm_rts5912_config *const pwm_config = dev->config;
	volatile struct pwm_regs *pwm_regs = pwm_config->pwm_regs;

	uint32_t pwm_div, pwm_duty;

	if (channel > 0) {
		return -EIO;
	}

	pwm_div = period_cycles;
	pwm_duty = pulse_cycles;

	pwm_regs->div = pwm_div;
	pwm_regs->duty = pwm_duty;

	LOG_DBG("period_cycles=%d, pulse_cycles=%d, pwm_div=%d, pwm_duty=%d", period_cycles,
		pulse_cycles, pwm_div, pwm_duty);

	if (flags == PWM_POLARITY_INVERTED) {
		pwm_regs->ctrl |= PWM_CTRL_INVT;
	}
	pwm_regs->ctrl |= PWM_CTRL_EN;

	return 0;
}

static int pwm_rts5912_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					  uint64_t *cycles)
{
	ARG_UNUSED(dev);

	if (channel > 0) {
		return -EIO;
	}

	if (cycles) {
		*cycles = PWM_CYCLE_PER_SEC;
	}

	return 0;
}

static DEVICE_API(pwm, pwm_rts5912_driver_api) = {
	.set_cycles = pwm_rts5912_set_cycles,
	.get_cycles_per_sec = pwm_rts5912_get_cycles_per_sec,
};

static int pwm_rts5912_init(const struct device *dev)
{
	const struct pwm_rts5912_config *const pwm_config = dev->config;
	struct rts5912_sccon_subsys sccon;

	int rc = 0;
#ifdef CONFIG_PINCTRL
	rc = pinctrl_apply_state(pwm_config->pcfg, PINCTRL_STATE_DEFAULT);
	if (rc < 0) {
		LOG_ERR("PWM pinctrl setup failed (%d)", rc);
		return rc;
	}
#endif
#ifdef CONFIG_CLOCK_CONTROL
	if (!device_is_ready(pwm_config->clk_dev)) {
		return -ENODEV;
	}

	sccon.clk_grp = pwm_config->pwm_clk_grp;
	sccon.clk_idx = pwm_config->pwm_clk_idx;
	rc = clock_control_on(pwm_config->clk_dev, (clock_control_subsys_t)&sccon);
	if (rc != 0) {
		return rc;
	}
#endif
	return rc;
}

#define RTS5912_PWM_PINCTRL_DEF(inst) PINCTRL_DT_INST_DEFINE(inst)

#define RTS5912_PWM_CONFIG(inst)                                                                   \
	static struct pwm_rts5912_config pwm_rts5912_config_##inst = {                             \
		.pwm_regs = (struct pwm_regs *)DT_INST_REG_ADDR(inst),                             \
		.pwm_clk_grp = DT_INST_CLOCKS_CELL(inst, clk_grp),                                 \
		.pwm_clk_idx = DT_INST_CLOCKS_CELL(inst, clk_idx),                                 \
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                               \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
	};

#define RTS5912_PWM_DEVICE_INIT(index)                                                             \
	RTS5912_PWM_PINCTRL_DEF(index);                                                            \
	RTS5912_PWM_CONFIG(index);                                                                 \
	DEVICE_DT_INST_DEFINE(index, &pwm_rts5912_init, NULL, NULL, &pwm_rts5912_config_##index,   \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,                     \
			      &pwm_rts5912_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RTS5912_PWM_DEVICE_INIT)
