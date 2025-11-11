/*
 * Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT bflb_pwm_1

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pwm_bflb, CONFIG_PWM_LOG_LEVEL);

#include <soc.h>
#include <bflb_soc.h>
#include <glb_reg.h>
#include <common_defines.h>
#include <zephyr/dt-bindings/clock/bflb_clock_common.h>
#include <bouffalolab/common/pwm_v1_reg.h>

#define PWM_WAIT_TIMEOUT_MS	100
#define PWM_CH_OFFSET_MUL	0x20
#define CHANNELS		5

struct pwm_bflb_config {
	uintptr_t base;
	const struct pinctrl_dev_config *pcfg;
};

struct pwm_bflb_data {
	uint16_t period_cycles[CHANNELS];
};

static int pwm_bflb_get_cycles_per_sec(const struct device *dev, uint32_t ch, uint64_t *cycles)
{
	const struct device *clock_ctrl =  DEVICE_DT_GET_ANY(bflb_clock_controller);
	uint32_t clk;
	int ret;

	ret = clock_control_get_rate(clock_ctrl, (void *)BFLB_CLKID_CLK_BCLK, &clk);

	*cycles = clk;

	LOG_DBG("cycles: %u", clk);

	return ret;
}

static int pwm_bflb_detrigger(const struct device *dev, uint32_t ch)
{
	const struct pwm_bflb_config *cfg = dev->config;
	volatile uint32_t tmp;

	tmp = sys_read32(cfg->base + PWM0_CONFIG_OFFSET + ch * PWM_CH_OFFSET_MUL);
	tmp |= PWM_STOP_EN;
	sys_write32(tmp, cfg->base + PWM0_CONFIG_OFFSET + ch * PWM_CH_OFFSET_MUL);

	return 0;
}


static int pwm_bflb_trigger(const struct device *dev, uint32_t ch)
{
	const struct pwm_bflb_config *cfg = dev->config;
	volatile uint32_t tmp;

	tmp = sys_read32(cfg->base + PWM0_CONFIG_OFFSET + ch * PWM_CH_OFFSET_MUL);
	tmp &= ~PWM_STOP_EN;
	sys_write32(tmp, cfg->base + PWM0_CONFIG_OFFSET + ch * PWM_CH_OFFSET_MUL);

	return 0;
}

static int pwm_bflb_set_cycles(const struct device *dev, uint32_t ch, uint32_t period_cycles,
			   uint32_t pulse_cycles, pwm_flags_t flags)
{
	const struct pwm_bflb_config *cfg = dev->config;
	struct pwm_bflb_data *data = dev->data;
	k_timepoint_t end_timeout =
		sys_timepoint_calc(K_MSEC(PWM_WAIT_TIMEOUT_MS));
	volatile uint32_t tmp;
	uint32_t pulse, divider;
	uint16_t period;

	if (ch >= CHANNELS) {
		return -EINVAL;
	}

	divider = period_cycles / UINT16_MAX + 1;

	if (divider > UINT16_MAX) {
		divider = UINT16_MAX;
	}

	period = period_cycles / divider;

	if (data->period_cycles[ch] != period_cycles) {
		pwm_bflb_detrigger(dev, ch);

		do {
			tmp = sys_read32(cfg->base + PWM0_CONFIG_OFFSET + ch * PWM_CH_OFFSET_MUL);
		} while (!(tmp & PWM_STS_TOP) && !sys_timepoint_expired(end_timeout));
		if (sys_timepoint_expired(end_timeout)) {
			return -ETIMEDOUT;
		}

		tmp = sys_read32(cfg->base + PWM0_CLKDIV_OFFSET + ch * PWM_CH_OFFSET_MUL);
		tmp &= ~PWM_CLK_DIV_MASK;
		tmp |= divider << PWM_CLK_DIV_SHIFT;
		sys_write32(tmp, cfg->base + PWM0_CLKDIV_OFFSET + ch * PWM_CH_OFFSET_MUL);

		tmp = sys_read32(cfg->base + PWM0_PERIOD_OFFSET + ch * PWM_CH_OFFSET_MUL);
		tmp &= ~PWM_PERIOD_MASK;
		tmp |= (uint32_t)period << PWM_PERIOD_SHIFT;
		sys_write32(tmp, cfg->base + PWM0_PERIOD_OFFSET + ch * PWM_CH_OFFSET_MUL);
	}

	pulse = pulse_cycles / divider;

	LOG_DBG("divider: %u period: %u pulse: %u", divider, period, pulse);

	tmp = sys_read32(cfg->base + PWM0_THRE1_OFFSET + ch * PWM_CH_OFFSET_MUL);
	tmp &= ~PWM_THRE1_MASK;
	sys_write32(tmp, cfg->base + PWM0_THRE1_OFFSET + ch * PWM_CH_OFFSET_MUL);

	tmp = sys_read32(cfg->base + PWM0_THRE2_OFFSET + ch * PWM_CH_OFFSET_MUL);
	tmp &= ~PWM_THRE2_MASK;
	tmp |= pulse;
	sys_write32(tmp, cfg->base + PWM0_THRE2_OFFSET + ch * PWM_CH_OFFSET_MUL);

	tmp = sys_read32(cfg->base + PWM0_CONFIG_OFFSET + ch * PWM_CH_OFFSET_MUL);
	if (flags & PWM_POLARITY_INVERTED) {
		tmp |= PWM_OUT_INV;
	} else {
		tmp &= ~PWM_OUT_INV;
	}
	/* Dont wait for period end */
	tmp &= ~PWM_STOP_MODE;
	tmp &= ~PWM_REG_CLK_SEL_MASK;
	/* Use BCLK */
	tmp |= 1U << PWM_REG_CLK_SEL_SHIFT;
	sys_write32(tmp, cfg->base + PWM0_CONFIG_OFFSET + ch * PWM_CH_OFFSET_MUL);

	if (data->period_cycles[ch] != period_cycles) {
		pwm_bflb_trigger(dev, ch);

		do {
			tmp = sys_read32(cfg->base + PWM0_CONFIG_OFFSET + ch * PWM_CH_OFFSET_MUL);
		} while (tmp & PWM_STS_TOP && !sys_timepoint_expired(end_timeout));
		if (sys_timepoint_expired(end_timeout)) {
			return -ETIMEDOUT;
		}
		data->period_cycles[ch] = period_cycles;
	}
	return 0;
}

static DEVICE_API(pwm, pwm_bflb_driver_api) = {
	.get_cycles_per_sec = pwm_bflb_get_cycles_per_sec,
	.set_cycles = pwm_bflb_set_cycles,
};

static int pwm_bflb_init(const struct device *dev)
{
	const struct pwm_bflb_config *cfg = dev->config;
	int err;

	err = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		LOG_ERR("Failed to configure pins for PWM. err=%d", err);
		return err;
	}

	return 0;
}

#define PWM_BFLB_INIT(idx)									   \
												   \
	PINCTRL_DT_INST_DEFINE(idx);								   \
	static const struct pwm_bflb_config pwm_bflb_config_##idx = {				   \
		.base = DT_INST_REG_ADDR(idx),							   \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),					   \
	};											   \
	static struct pwm_bflb_data pwm_bflb_data_##idx = { 0 };				   \
												   \
	DEVICE_DT_INST_DEFINE(idx, pwm_bflb_init, NULL,						   \
			      &pwm_bflb_data_##idx, &pwm_bflb_config_##idx, POST_KERNEL,	   \
			      CONFIG_PWM_INIT_PRIORITY, &pwm_bflb_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_BFLB_INIT);
