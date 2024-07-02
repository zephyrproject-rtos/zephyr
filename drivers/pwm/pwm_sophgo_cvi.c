/*
 * Copyright (c) 2023-2024 Chen Xingyu <hi@xingrz.me>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sophgo_cvi_pwm

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#if defined(CONFIG_PINCTRL)
#include <zephyr/drivers/pinctrl.h>
#endif /* CONFIG_PINCTRL */

#define HLPERIOD(base, n)   (base + 0x000 + (n) * 8)
#define PERIOD(base, n)     (base + 0x004 + (n) * 8)
#define PWMCONFIG(base)     (base + 0x040)
#define PWMSTART(base)      (base + 0x044)
#define PWMDONE(base)       (base + 0x048)
#define PWMUPDATE(base)     (base + 0x04c)
#define PCOUNT(base, n)     (base + 0x050 + (n) * 4)
#define PULSECOUNT(base, n) (base + 0x060 + (n) * 4)
#define SHIFTCOUNT(base, n) (base + 0x080 + (n) * 4)
#define SHIFTSTART(base)    (base + 0x090)
#define PWM_OE(base)        (base + 0x0d0)

/* PWMCONFIG */
#define CFG_POLARITY(n) BIT(n + 0)
#define CFG_PWMMODE(n)  BIT(n + 8)
#define CFG_SHIFTMODE   BIT(16)

#define PWM_CH_MAX 4

struct pwm_cvi_config {
	mm_reg_t base;
	uint32_t clk_pwm;
#if defined(CONFIG_PINCTRL)
	const struct pinctrl_dev_config *pcfg;
#endif /* CONFIG_PINCTRL */
};

static int pwm_cvi_set_cycles(const struct device *dev, uint32_t channel, uint32_t period_cycles,
			      uint32_t pulse_cycles, pwm_flags_t flags)
{
	const struct pwm_cvi_config *cfg = dev->config;
	uint32_t regval;

	if (channel > PWM_CH_MAX) {
		return -EINVAL;
	}

	if (period_cycles > cfg->clk_pwm) {
		return -EINVAL;
	}

	if (pulse_cycles >= period_cycles) {
		pulse_cycles = period_cycles - 1;
	}

	/* Configure output */
	regval = sys_read32(PWM_OE(cfg->base));
	regval |= BIT(channel);
	sys_write32(regval, PWM_OE(cfg->base));

	/* Set polarity and mode */
	regval = sys_read32(PWMCONFIG(cfg->base));
	if (flags & PWM_POLARITY_INVERTED) {
		regval &= ~CFG_POLARITY(channel); /* active low */
	} else {
		regval |= CFG_POLARITY(channel); /* active high */
	}
	regval &= ~CFG_PWMMODE(channel); /* continuous mode */
	sys_write32(regval, PWMCONFIG(cfg->base));

	/* Set period and pulse */
	sys_write32(period_cycles, PERIOD(cfg->base, channel));
	sys_write32(pulse_cycles, HLPERIOD(cfg->base, channel));

	if (sys_read32(PWMSTART(cfg->base)) & BIT(channel)) {
		/* Update channel */
		regval = sys_read32(PWMUPDATE(cfg->base));
		regval |= BIT(channel);
		sys_write32(regval, PWMUPDATE(cfg->base));
		regval &= ~BIT(channel);
		sys_write32(regval, PWMUPDATE(cfg->base));
	} else {
		/* Start channel */
		regval = sys_read32(PWMSTART(cfg->base));
		regval &= ~BIT(channel);
		sys_write32(regval, PWMSTART(cfg->base));
		regval |= BIT(channel);
		sys_write32(regval, PWMSTART(cfg->base));
	}

	return 0;
}

static int pwm_cvi_get_cycles_per_sec(const struct device *dev, uint32_t channel, uint64_t *cycles)
{
	const struct pwm_cvi_config *cfg = dev->config;

	if (channel > PWM_CH_MAX) {
		return -EINVAL;
	}

	*cycles = cfg->clk_pwm;

	return 0;
}

static int pwm_cvi_init(const struct device *dev)
{
#if defined(CONFIG_PINCTRL)
	const struct pwm_cvi_config *cfg = dev->config;
	int ret;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}
#endif /* CONFIG_PINCTRL */

	return 0;
}

static const struct pwm_driver_api pwm_cvi_api = {
	.set_cycles = pwm_cvi_set_cycles,
	.get_cycles_per_sec = pwm_cvi_get_cycles_per_sec,
};

#define PWM_CVI_INST(n)                                                                            \
	IF_ENABLED(CONFIG_PINCTRL, (PINCTRL_DT_INST_DEFINE(n);))                                   \
                                                                                                   \
	static const struct pwm_cvi_config pwm_cvi_cfg_##n = {                                     \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.clk_pwm = DT_INST_PROP(n, clock_frequency),                                       \
		IF_ENABLED(CONFIG_PINCTRL, (.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),))           \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &pwm_cvi_init, NULL, NULL, &pwm_cvi_cfg_##n, PRE_KERNEL_1,        \
			      CONFIG_PWM_INIT_PRIORITY, &pwm_cvi_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_CVI_INST)
