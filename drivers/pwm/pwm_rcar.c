/*
 * Copyright (c) 2022 IoT.bzh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_pwm_rcar

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control/renesas_cpg_mssr.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pwm.h>

#include <soc.h>

#define LOG_LEVEL CONFIG_PWM_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pwm_rcar);

/* PWM Controller capabilities */
#define RCAR_PWM_MAX_CYCLE   1023U
#define RCAR_PWM_MAX_DIV     24U
#define RCAR_PWM_MAX_CHANNEL 6

/* Registers */
#define RCAR_PWM_REG_SHIFT 0x1000
#define RCAR_PWM_CR(channel)                                                                       \
	((uint32_t)((channel * RCAR_PWM_REG_SHIFT)) + 0x00) /* PWM Control Register */
#define RCAR_PWM_CNT(channel)                                                                      \
	((uint32_t)((channel * RCAR_PWM_REG_SHIFT)) + 0x04) /* PWM Count Register */

/* PWMCR (PWM Control Register) */
#define RCAR_PWM_CR_CC_MASK  0x000f0000 /* Clock Control */
#define RCAR_PWM_CR_CC_SHIFT 16
#define RCAR_PWM_CR_CCMD     BIT(15) /* Frequency Division Mode */
#define RCAR_PWM_CR_SYNC     BIT(11)
#define RCAR_PWM_CR_SS	     BIT(4) /* Single Pulse Output */
#define RCAR_PWM_CR_EN	     BIT(0) /* Channel Enable */

/* PWM Diviser is on 5 bits (CC combined with CCMD) */
#define RCAR_PWM_DIVISER_MASK  (RCAR_PWM_CR_CC_MASK | RCAR_PWM_CR_CCMD)
#define RCAR_PWM_DIVISER_SHIFT 15

/* PWMCNT (PWM Count Register) */
#define RCAR_PWM_CNT_CYC_MASK  0x03ff0000 /* PWM Cycle */
#define RCAR_PWM_CNT_CYC_SHIFT 16
#define RCAR_PWM_CNT_PH_MASK   0x000003ff /* PWM High-Level Period */
#define RCAR_PWM_CNT_PH_SHIFT  0

struct pwm_rcar_cfg {
	uint32_t reg_addr;
	const struct device *clock_dev;
	struct rcar_cpg_clk core_clk;
	struct rcar_cpg_clk mod_clk;
	const struct pinctrl_dev_config *pcfg;
};

struct pwm_rcar_data {
	uint32_t clk_rate;
};

static uint32_t pwm_rcar_read(const struct pwm_rcar_cfg *config, uint32_t offs)
{
	return sys_read32(config->reg_addr + offs);
}

static void pwm_rcar_write(const struct pwm_rcar_cfg *config, uint32_t offs, uint32_t value)
{
	sys_write32(value, config->reg_addr + offs);
}

static void pwm_rcar_write_bit(const struct pwm_rcar_cfg *config, uint32_t offs, uint32_t bits,
			       bool value)
{
	uint32_t reg_val = pwm_rcar_read(config, offs);

	if (value) {
		reg_val |= bits;
	} else {
		reg_val &= ~(bits);
	}

	pwm_rcar_write(config, offs, reg_val);
}

static int pwm_rcar_update_clk(const struct pwm_rcar_cfg *config, uint32_t channel,
			       uint32_t *period_cycles, uint32_t *pulse_cycles)
{
	uint32_t reg_val, power, diviser;

	power = pwm_rcar_read(config, RCAR_PWM_CR(channel)) & RCAR_PWM_DIVISER_MASK;
	power = power >> RCAR_PWM_DIVISER_SHIFT;
	diviser = 1 << power;

	LOG_DBG("Found old diviser : 2^%d=%d", power, diviser);

	/* Looking for the best possible clock diviser */
	if (*period_cycles > RCAR_PWM_MAX_CYCLE) {
		/* Reducing clock speed */
		while (*period_cycles > RCAR_PWM_MAX_CYCLE) {
			diviser *= 2;
			*period_cycles /= 2;
			*pulse_cycles /= 2;
			power++;
			if (power > RCAR_PWM_MAX_DIV) {
				return -ENOTSUP;
			}
		}
	} else {
		/* Increasing clock speed */
		while (*period_cycles < (RCAR_PWM_MAX_CYCLE / 2)) {
			if (power == 0) {
				return -ENOTSUP;
			}
			diviser /= 2;
			*period_cycles *= 2;
			*pulse_cycles *= 2;
			power--;
		}
	}
	LOG_DBG("Found new diviser : 2^%d=%d\n", power, diviser);

	/* Set new clock Diviser */
	reg_val = pwm_rcar_read(config, RCAR_PWM_CR(channel));
	reg_val &= ~RCAR_PWM_DIVISER_MASK;
	reg_val |= (power << RCAR_PWM_DIVISER_SHIFT);
	pwm_rcar_write(config, RCAR_PWM_CR(channel), reg_val);

	return 0;
}

static int pwm_rcar_set_cycles(const struct device *dev, uint32_t channel, uint32_t period_cycles,
			       uint32_t pulse_cycles, pwm_flags_t flags)
{
	const struct pwm_rcar_cfg *config = dev->config;
	uint32_t reg_val;
	int ret = 0;

	if (channel > RCAR_PWM_MAX_CHANNEL) {
		return -ENOTSUP;
	}

	if (flags != PWM_POLARITY_NORMAL) {
		return -ENOTSUP;
	}

	/* Prohibited values */
	if (period_cycles == 0U || pulse_cycles == 0U || pulse_cycles > period_cycles) {
		return -EINVAL;
	}

	LOG_DBG("base_reg=0x%x, pulse_cycles=%d, period_cycles=%d,"
		" duty_cycle=%d",
		config->reg_addr, pulse_cycles, period_cycles,
		(pulse_cycles * 100U / period_cycles));

	/* Disable PWM */
	pwm_rcar_write_bit(config, RCAR_PWM_CR(channel), RCAR_PWM_CR_EN, false);

	/* Set continuous mode */
	pwm_rcar_write_bit(config, RCAR_PWM_CR(channel), RCAR_PWM_CR_SS, false);

	/* Enable SYNC mode */
	pwm_rcar_write_bit(config, RCAR_PWM_CR(channel), RCAR_PWM_CR_SYNC, true);

	/*
	 * Set clock counter according to the requested period_cycles
	 * if period_cycles is less than half of the counter, then the
	 * clock diviser could be updated as the diviser is a modulo 2.
	 */
	if (period_cycles > RCAR_PWM_MAX_CYCLE || period_cycles < (RCAR_PWM_MAX_CYCLE / 2)) {
		LOG_DBG("Adapting frequency diviser...");
		ret = pwm_rcar_update_clk(config, channel, &period_cycles, &pulse_cycles);
		if (ret != 0) {
			return ret;
		}
	}

	/* Set total period cycle */
	reg_val = pwm_rcar_read(config, RCAR_PWM_CNT(channel));
	reg_val &= ~(RCAR_PWM_CNT_CYC_MASK);
	reg_val |= (period_cycles << RCAR_PWM_CNT_CYC_SHIFT);
	pwm_rcar_write(config, RCAR_PWM_CNT(channel), reg_val);

	/* Set high level period cycle */
	reg_val = pwm_rcar_read(config, RCAR_PWM_CNT(channel));
	reg_val &= ~(RCAR_PWM_CNT_PH_MASK);
	reg_val |= (pulse_cycles << RCAR_PWM_CNT_PH_SHIFT);
	pwm_rcar_write(config, RCAR_PWM_CNT(channel), reg_val);

	/* Enable PWM */
	pwm_rcar_write_bit(config, RCAR_PWM_CR(channel), RCAR_PWM_CR_EN, true);

	return ret;
}

static int pwm_rcar_get_cycles_per_sec(const struct device *dev, uint32_t channel, uint64_t *cycles)
{
	const struct pwm_rcar_cfg *config = dev->config;
	struct pwm_rcar_data *data = dev->data;
	uint32_t diviser;

	if (channel > RCAR_PWM_MAX_CHANNEL) {
		return -ENOTSUP;
	}

	diviser = pwm_rcar_read(config, RCAR_PWM_CR(channel)) & RCAR_PWM_DIVISER_MASK;
	diviser = diviser >> RCAR_PWM_DIVISER_SHIFT;
	*cycles = data->clk_rate >> diviser;

	LOG_DBG("Actual division: %d and Frequency: %d Hz", diviser, (uint32_t)*cycles);

	return 0;
}

static int pwm_rcar_init(const struct device *dev)
{
	const struct pwm_rcar_cfg *config = dev->config;
	struct pwm_rcar_data *data = dev->data;
	int ret;

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	ret = clock_control_on(config->clock_dev, (clock_control_subsys_t)&config->mod_clk);
	if (ret < 0) {
		return ret;
	}

	ret = clock_control_get_rate(config->clock_dev, (clock_control_subsys_t)&config->core_clk,
				     &data->clk_rate);

	if (ret < 0) {
		return ret;
	}

	return 0;
}

static const struct pwm_driver_api pwm_rcar_driver_api = {
	.set_cycles = pwm_rcar_set_cycles,
	.get_cycles_per_sec = pwm_rcar_get_cycles_per_sec,
};

/* Device Instantiation */
#define PWM_DEVICE_RCAR_INIT(n)                                                                    \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static const struct pwm_rcar_cfg pwm_rcar_cfg_##n = {                                      \
		.reg_addr = DT_INST_REG_ADDR(n),                                                   \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.mod_clk.module = DT_INST_CLOCKS_CELL_BY_IDX(n, 0, module),                        \
		.mod_clk.domain = DT_INST_CLOCKS_CELL_BY_IDX(n, 0, domain),                        \
		.core_clk.module = DT_INST_CLOCKS_CELL_BY_IDX(n, 1, module),                       \
		.core_clk.domain = DT_INST_CLOCKS_CELL_BY_IDX(n, 1, domain),                       \
	};                                                                                         \
	static struct pwm_rcar_data pwm_rcar_data_##n;                                             \
	DEVICE_DT_INST_DEFINE(n, pwm_rcar_init, NULL, &pwm_rcar_data_##n, &pwm_rcar_cfg_##n,       \
			      POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,                               \
			      &pwm_rcar_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_DEVICE_RCAR_INIT)
