/*
 * Copyright (c) 2021 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8xxx2_pwm

#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pwm/it8xxx2_pwm.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <soc_dt.h>
#include <stdlib.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_ite_it8xxx2, CONFIG_PWM_LOG_LEVEL);

#define PWM_CTRX_MIN	100
#define PWM_FREQ	EC_FREQ
#define PCSSG_MASK	0x3

struct pwm_it8xxx2_cfg {
	/* PWM channel duty cycle register */
	uintptr_t reg_dcr;
	/* PWM channel clock source selection register */
	uintptr_t reg_pcssg;
	/* PWM channel clock source gating register */
	uintptr_t reg_pcsgr;
	/* PWM channel output polarity register */
	uintptr_t reg_pwmpol;
	/* PWM channel */
	int channel;
	/* PWM prescaler control register base */
	struct pwm_it8xxx2_regs *base;
	/* Select PWM prescaler that output to PWM channel */
	int prs_sel;
	/* PWM alternate configuration */
	const struct pinctrl_dev_config *pcfg;
};

static void pwm_enable(const struct device *dev, int enabled)
{
	const struct pwm_it8xxx2_cfg *config = dev->config;
	volatile uint8_t *reg_pcsgr = (uint8_t *)config->reg_pcsgr;
	int ch = config->channel;

	if (enabled) {
		/* PWM channel clock source not gating */
		*reg_pcsgr &= ~BIT(ch);
	} else {
		/* PWM channel clock source gating */
		*reg_pcsgr |= BIT(ch);
	}
}

static int pwm_it8xxx2_get_cycles_per_sec(const struct device *dev,
					  uint32_t channel, uint64_t *cycles)
{
	ARG_UNUSED(channel);

	/*
	 * There are three ways to call pwm_it8xxx2_set_cycles() from pwm api:
	 * 1) pwm_set_cycles_usec() -> pwm_set_cycles_cycles() -> pwm_it8xxx2_set_cycles()
	 *    target_freq = pwm_clk_src / period_cycles
	 *                = cycles / (period * cycles / USEC_PER_SEC)
	 *                = USEC_PER_SEC / period
	 * 2) pwm_set_cycles_nsec() -> pwm_set_cycles_cycles() -> pwm_it8xxx2_set_cycles()
	 *    target_freq = pwm_clk_src / period_cycles
	 *                = cycles / (period * cycles / NSEC_PER_SEC)
	 *                = NSEC_PER_SEC / period
	 * 3) pwm_set_cycles_cycles() -> pwm_it8xxx2_set_cycles()
	 *    target_freq = pwm_clk_src / period_cycles
	 *                = cycles / period
	 *
	 * If we need to pwm output in EC power saving mode, then we will switch
	 * the prescaler clock source (cycles) from 8MHz to 32.768kHz. In order
	 * to get the same target_freq in the 3) case, we always return PWM_FREQ.
	 */
	*cycles = (uint64_t) PWM_FREQ;

	return 0;
}

static int pwm_it8xxx2_set_cycles(const struct device *dev,
				  uint32_t channel, uint32_t period_cycles,
				  uint32_t pulse_cycles, pwm_flags_t flags)
{
	const struct pwm_it8xxx2_cfg *config = dev->config;
	struct pwm_it8xxx2_regs *const inst = config->base;
	volatile uint8_t *reg_dcr = (uint8_t *)config->reg_dcr;
	volatile uint8_t *reg_pwmpol = (uint8_t *)config->reg_pwmpol;
	int ch = config->channel;
	int prs_sel = config->prs_sel;
	uint32_t actual_freq = 0xffffffff, target_freq, deviation, cxcprs, ctr;
	uint64_t pwm_clk_src;

	/* PWM channel clock source gating before configuring */
	pwm_enable(dev, 0);

	/* Select PWM inverted polarity (ex. active-low pulse) */
	if (flags & PWM_POLARITY_INVERTED) {
		*reg_pwmpol |= BIT(ch);
	} else {
		*reg_pwmpol &= ~BIT(ch);
	}

	/* If pulse cycles is 0, set duty cycle 0 and enable pwm channel */
	if (pulse_cycles == 0) {
		*reg_dcr = 0;
		pwm_enable(dev, 1);
		return 0;
	}

	pwm_it8xxx2_get_cycles_per_sec(dev, channel, &pwm_clk_src);
	target_freq = ((uint32_t) pwm_clk_src) / period_cycles;

	/*
	 * Support PWM output frequency:
	 * 1) 8MHz clock source: 1Hz <= target_freq <= 79207Hz
	 * 2) 32.768KHz clock source: 1Hz <= target_freq <= 324Hz
	 * NOTE: PWM output signal maximum supported frequency comes from
	 *       [8MHz or 32.768KHz] / 1 / (PWM_CTRX_MIN + 1).
	 *       PWM output signal minimum supported frequency comes from
	 *       [8MHz or 32.768KHz] / 65536 / 256, the minimum integer is 1.
	 */
	if (target_freq < 1) {
		LOG_ERR("PWM output frequency is < 1 !");
		return -EINVAL;
	}

	deviation = (target_freq / 100) + 1;

	/*
	 * Default clock source setting is 8MHz, when ITE chip is in power
	 * saving mode, clock source 8MHz will be gated (32.768KHz won't).
	 * So if we still need pwm output in mode, then we should set frequency
	 * <=324Hz in board dts. Now change prescaler clock source from 8MHz to
	 * 32.768KHz to support pwm output in mode.
	 */
	if (target_freq <= 324) {
		if (inst->PCFSR & BIT(prs_sel)) {
			inst->PCFSR &= ~BIT(prs_sel);
		}

		pwm_clk_src = (uint64_t) 32768;
	}

	/*
	 * PWM output signal frequency is
	 * pwm_clk_src / ((CxCPRS[15:0] + 1) * (CTRx[7:0] + 1))
	 * NOTE: 1) define CTR minimum is 100 for more precisely when
	 *          calculate DCR
	 *       2) CxCPRS[15:0] value 0001h results in a divisor 2
	 *          CxCPRS[15:0] value FFFFh results in a divisor 65536
	 *          CTRx[7:0] value 00h results in a divisor 1
	 *          CTRx[7:0] value FFh results in a divisor 256
	 */
	for (ctr = 0xFF; ctr >= PWM_CTRX_MIN; ctr--) {
		cxcprs = (((uint32_t) pwm_clk_src) / (ctr + 1) / target_freq);
		/*
		 * Make sure cxcprs isn't zero, or we will have
		 * divide-by-zero on calculating actual_freq.
		 */
		if (cxcprs != 0) {
			actual_freq = ((uint32_t) pwm_clk_src) / (ctr + 1) / cxcprs;
			if (abs(actual_freq - target_freq) < deviation) {
				/* CxCPRS[15:0] = cxcprs - 1 */
				cxcprs--;
				break;
			}
		}
	}

	if (cxcprs > UINT16_MAX) {
		LOG_ERR("PWM prescaler CxCPRS only support 2 bytes !");
		return -EINVAL;
	}

	/* Set PWM prescaler clock divide and cycle time register */
	if (prs_sel == PWM_PRESCALER_C4) {
		inst->C4CPRS = cxcprs & 0xFF;
		inst->C4MCPRS = (cxcprs >> 8) & 0xFF;
		inst->CTR1 = ctr;
	} else if (prs_sel == PWM_PRESCALER_C6) {
		inst->C6CPRS = cxcprs & 0xFF;
		inst->C6MCPRS = (cxcprs >> 8) & 0xFF;
		inst->CTR2 = ctr;
	} else if (prs_sel == PWM_PRESCALER_C7) {
		inst->C7CPRS = cxcprs & 0xFF;
		inst->C7MCPRS = (cxcprs >> 8) & 0xFF;
		inst->CTR3 = ctr;
	}

	/* Set PWM channel duty cycle register */
	*reg_dcr = (ctr * pulse_cycles) / period_cycles;

	/* PWM channel clock source not gating */
	pwm_enable(dev, 1);

	LOG_DBG("clock source freq %d, target freq %d",
		(uint32_t) pwm_clk_src, target_freq);

	return 0;
}

static int pwm_it8xxx2_init(const struct device *dev)
{
	const struct pwm_it8xxx2_cfg *config = dev->config;
	struct pwm_it8xxx2_regs *const inst = config->base;
	volatile uint8_t *reg_pcssg = (uint8_t *)config->reg_pcssg;
	int ch = config->channel;
	int prs_sel = config->prs_sel;
	int pcssg_shift;
	int pcssg_mask;
	int status;

	/* PWM channel clock source gating before configuring */
	pwm_enable(dev, 0);

	/* Select clock source 8MHz for prescaler */
	inst->PCFSR |= BIT(prs_sel);

	/* Bit shift and mask of prescaler clock source select group register */
	pcssg_shift = (ch % 4) * 2;
	pcssg_mask = (prs_sel & PCSSG_MASK) << pcssg_shift;

	/* Select which prescaler output to PWM channel */
	*reg_pcssg &= ~(PCSSG_MASK << pcssg_shift);
	*reg_pcssg |= pcssg_mask;

	/*
	 * The cycle timer1 of it8320 later series was enhanced from
	 * 8bits to 10bits resolution, and others are still 8bit resolution.
	 * Because the cycle timer1 high byte default value is not zero,
	 * we clear cycle timer1 high byte at init and use it as 8-bit
	 * resolution like others.
	 */
	inst->CTR1M = 0;

	/* Enable PWMs clock counter */
	inst->ZTIER |= IT8XXX2_PWM_PCCE;

	/* Set alternate mode of PWM pin */
	status = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (status < 0) {
		LOG_ERR("Failed to configure PWM pins");
		return status;
	}

	return 0;
}

static const struct pwm_driver_api pwm_it8xxx2_api = {
	.set_cycles = pwm_it8xxx2_set_cycles,
	.get_cycles_per_sec = pwm_it8xxx2_get_cycles_per_sec,
};

/* Device Instance */
#define PWM_IT8XXX2_INIT(inst)								\
	PINCTRL_DT_INST_DEFINE(inst);							\
											\
	static const struct pwm_it8xxx2_cfg pwm_it8xxx2_cfg_##inst = {			\
		.reg_dcr = DT_INST_REG_ADDR_BY_IDX(inst, 0),				\
		.reg_pcssg = DT_INST_REG_ADDR_BY_IDX(inst, 1),				\
		.reg_pcsgr = DT_INST_REG_ADDR_BY_IDX(inst, 2),				\
		.reg_pwmpol = DT_INST_REG_ADDR_BY_IDX(inst, 3),				\
		.channel = DT_PROP(DT_INST(inst, ite_it8xxx2_pwm), channel),		\
		.base = (struct pwm_it8xxx2_regs *) DT_REG_ADDR(DT_NODELABEL(prs)),	\
		.prs_sel = DT_PROP(DT_INST(inst, ite_it8xxx2_pwm), prescaler_cx),	\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),				\
	};										\
											\
	DEVICE_DT_INST_DEFINE(inst,							\
			      &pwm_it8xxx2_init,					\
			      NULL,							\
			      NULL,							\
			      &pwm_it8xxx2_cfg_##inst,					\
			      PRE_KERNEL_1,						\
			      CONFIG_PWM_INIT_PRIORITY,					\
			      &pwm_it8xxx2_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_IT8XXX2_INIT)
