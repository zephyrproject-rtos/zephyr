/*
 * Copyright (c) 2025, Qingsong Gou <gouqs@hotmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_lptim_pwm

#include <zephyr/drivers/clock_control/sf32lb.h>
#include <zephyr/dt-bindings/clock/sf32lb52x-clocks.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <register.h>

LOG_MODULE_REGISTER(pwm_sf32lb_lptim, CONFIG_PWM_LOG_LEVEL);

#define LPTIM_CR   offsetof(LPTIM_TypeDef, CR)
#define LPTIM_CFGR offsetof(LPTIM_TypeDef, CFGR)
#define LPTIM_CMP  offsetof(LPTIM_TypeDef, CMP)
#define LPTIM_ARR  offsetof(LPTIM_TypeDef, ARR)
#define LPTIM_CNT  offsetof(LPTIM_TypeDef, CNT)
#define LPTIM_ISR  offsetof(LPTIM_TypeDef, ISR)
#define LPTIM_ICR  offsetof(LPTIM_TypeDef, ICR)

struct pwm_sf32lb_lptim_config {
	uintptr_t base;
	const struct pinctrl_dev_config *pincfg;
	uint8_t prescaler;
	struct sf32lb_clock_dt_spec clock;
};

static int pwm_sf32lb_lptim_set_cycles(const struct device *dev, uint32_t channel,
				       uint32_t period_cycles, uint32_t pulse_cycles,
				       pwm_flags_t flags)
{
	const struct pwm_sf32lb_lptim_config *cfg = dev->config;
	uint32_t cfgr;

	if (channel != 0) {
		LOG_ERR("Invalid channel %u, LPTIM only supports channel 0", channel);
		return -EINVAL;
	}

	if (period_cycles > 0xFFFFFF || pulse_cycles > 0xFFFFFF) {
		LOG_ERR("Period or pulse cycles exceed maximum value");
		return -EINVAL;
	}

	sys_clear_bit(cfg->base + LPTIM_CR, LPTIM_CR_ENABLE_Pos);

	cfgr = sys_read32(cfg->base + LPTIM_CFGR);
	if (flags & PWM_POLARITY_INVERTED) {
		cfgr |= LPTIM_CFGR_WAVPOL_Msk;
	} else {
		cfgr &= ~LPTIM_CFGR_WAVPOL_Msk;
	}
	sys_write32(cfgr, cfg->base + LPTIM_CFGR);

	sys_write32(period_cycles - 1, cfg->base + LPTIM_ARR);

	sys_write32(pulse_cycles, cfg->base + LPTIM_CMP);

	sys_write32(LPTIM_CR_CNTSTRT | LPTIM_CR_ENABLE, cfg->base + LPTIM_CR);

	LOG_DBG("LPTIM PWM set: period=%u, pulse=%u, prescaler=%u", period_cycles, pulse_cycles,
		cfg->prescaler);

	return 0;
}

static int pwm_sf32lb_lptim_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					       uint64_t *cycles)
{
	const struct pwm_sf32lb_lptim_config *cfg = dev->config;
	uint32_t rate;
	uint32_t prescaler_div;
	uint32_t clk_sel;
	uint32_t cfgr;
	uint16_t clk_id;

	if (channel != 0) {
		LOG_ERR("Invalid channel %u, LPTIM only supports channel 0", channel);
		return -EINVAL;
	}

	if (sys_test_bit(cfg->base + LPTIM_CFGR, LPTIM_CFGR_CKSEL_Pos)) {
		LOG_ERR("External clock source not supported");
		return -ENOTSUP;
	}
	cfgr = sys_read32(cfg->base + LPTIM_CFGR);
	clk_sel = FIELD_GET(LPTIM_CFGR_CKSEL_Msk, cfgr);
	clk_id = clk_sel == 0 ? SF32LB52X_CLOCK_LRC10 : SF32LB52X_CLOCK_PCLK2;

	sf32lb_clock_control_get_rate_dt(&cfg->clock, &rate);

	prescaler_div = BIT(cfg->prescaler);
	*cycles = rate / prescaler_div;

	return 0;
}

static const struct pwm_driver_api pwm_sf32lb_lptim_api = {
	.set_cycles = pwm_sf32lb_lptim_set_cycles,
	.get_cycles_per_sec = pwm_sf32lb_lptim_get_cycles_per_sec,
};

static int pwm_sf32lb_lptim_init(const struct device *dev)
{
	const struct pwm_sf32lb_lptim_config *cfg = dev->config;
	uint32_t cfgr;
	int err;

	err = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		LOG_ERR("Failed to apply pinctrl state: %d", err);
		return err;
	}

	if (!sf32lb_clock_is_ready_dt(&cfg->clock)) {
		return -ENODEV;
	}

	cfgr = sys_read32(cfg->base + LPTIM_CFGR);
	cfgr &= ~LPTIM_CFGR_WAVE_Msk;
	cfgr &= ~LPTIM_CFGR_PRESC_Msk;
	cfgr |= FIELD_PREP(LPTIM_CFGR_PRESC_Msk, cfg->prescaler);
	sys_write32(cfgr, cfg->base + LPTIM_CFGR);

	return err;
}

#define PWM_SF32LB_LPTIM_DEFINE(n)                                                                 \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static const struct pwm_sf32lb_lptim_config pwm_sf32lb_lptim_config_##n = {                \
		.base = DT_REG_ADDR(DT_INST_PARENT(n)),                                            \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
		.clock = SF32LB_CLOCK_DT_INST_PARENT_SPEC_GET(n),                                  \
		.prescaler = DT_PROP(DT_INST_PARENT(n), sifli_prescaler),                          \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, pwm_sf32lb_lptim_init, NULL, NULL,                                \
			      &pwm_sf32lb_lptim_config_##n, POST_KERNEL, CONFIG_PWM_INIT_PRIORITY, \
			      &pwm_sf32lb_lptim_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_SF32LB_LPTIM_DEFINE)
