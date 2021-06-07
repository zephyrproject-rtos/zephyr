/*
 * Copyright (c) 2021 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8xxx2_pwm

#include <device.h>
#include <drivers/pwm.h>
#include <drivers/pinmux.h>
#include <dt-bindings/pwm/it8xxx2_pwm.h>
#include <errno.h>
#include <kernel.h>
#include <soc.h>
#include <stdlib.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(pwm_ite_it8xxx2, CONFIG_PWM_LOG_LEVEL);

#define PWM_CTRX_MIN	100
#define PWM_EC_FREQ	MHZ(8)
#define PCSSG_MASK	0x3

/* Device config */
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
	uintptr_t base;
	/* Select PWM prescaler that output to PWM channel */
	int prs_sel;
	/* Pinmux control device structure */
	const struct device *pinctrls;
	/* GPIO pin */
	uint8_t pin;
	/* Alternate function */
	uint8_t alt_fun;
};

/* Driver convenience defines */
#define DRV_CONFIG(dev)		((const struct pwm_it8xxx2_cfg * const)(dev)->config)
#define DRV_REG(dev)		(struct pwm_it8xxx2_regs *)(DRV_CONFIG(dev)->base)
#define DEV_PINMUX(inst)	\
	DEVICE_DT_GET(DT_PHANDLE_BY_IDX(DT_NODELABEL(pinctrl_pwm##inst), pinctrls, 0))
#define DEV_PIN(inst)		\
	DT_PHA(DT_PHANDLE_BY_IDX(DT_DRV_INST(inst), pinctrl_0, 0), pinctrls, pin)
#define DEV_ALT_FUN(inst)	\
	DT_PHA(DT_PHANDLE_BY_IDX(DT_DRV_INST(inst), pinctrl_0, 0), pinctrls, alt_func)

static void pwm_enable(const struct device *dev, int enabled)
{
	const struct pwm_it8xxx2_cfg *config = DRV_CONFIG(dev);
	volatile uint8_t *reg_pcsgr = (uint8_t *)config->reg_pcsgr;
	int ch = config->channel;

	if (enabled)
		/* PWM channel clock source not gating */
		*reg_pcsgr &= ~BIT(ch);
	else
		/* PWM channel clock source gating */
		*reg_pcsgr |= BIT(ch);
}

static int pwm_it8xxx2_get_cycles_per_sec(const struct device *dev,
					     uint32_t pwm, uint64_t *cycles)
{
	const struct pwm_it8xxx2_cfg *config = DRV_CONFIG(dev);
	struct pwm_it8xxx2_regs *const inst = DRV_REG(dev);
	int prs_sel = config->prs_sel;

	ARG_UNUSED(pwm);

	/* Get clock source cycles per second that output to prescaler */
	if ((inst->PCFSR) & BIT(prs_sel))
		*cycles = (uint64_t) PWM_EC_FREQ;
	else
		*cycles = (uint64_t) 32768;

	return 0;
}

static int pwm_it8xxx2_pin_set(const struct device *dev,
				uint32_t pwm, uint32_t period_cycles,
				uint32_t pulse_cycles, pwm_flags_t flags)
{
	const struct pwm_it8xxx2_cfg *config = DRV_CONFIG(dev);
	struct pwm_it8xxx2_regs *const inst = DRV_REG(dev);
	volatile uint8_t *reg_dcr = (uint8_t *)config->reg_dcr;
	volatile uint8_t *reg_pwmpol = (uint8_t *)config->reg_pwmpol;
	int ch = config->channel;
	int prs_sel = config->prs_sel;
	uint32_t actual_freq = 0xffffffff, target_freq, deviation, cxcprs, ctr;
	uint64_t pwm_clk_src;

	ARG_UNUSED(pwm);

	if (pulse_cycles > period_cycles)
		return -EINVAL;

	/* PWM channel clock source gating before configuring */
	pwm_enable(dev, 0);

	/* Select PWM inverted polarity (ex. active-low pulse) */
	if (flags & PWM_POLARITY_INVERTED)
		*reg_pwmpol |= BIT(ch);
	else
		*reg_pwmpol &= ~BIT(ch);

	/* If pulse cycles is 0, set duty cycle 0 and enable pwm channel */
	if (pulse_cycles == 0) {
		*reg_dcr = 0;
		pwm_enable(dev, 1);
		return 0;
	}

	pwm_it8xxx2_get_cycles_per_sec(dev, pwm, &pwm_clk_src);
	target_freq = ((uint32_t) pwm_clk_src) / period_cycles;
	deviation = (target_freq / 100) + 1;

	/*
	 * Default clock source setting is 8MHz, when ITE chip is in power
	 * saving mode, clock source 8MHz will be gated (32.768KHz won't).
	 * So if we still need pwm output in mode, then we should set frequency
	 * <=324Hz in board dts. Now change prescaler clock source from 8MHz to
	 * 32.768KHz to support pwm output in mode.
	 * NOTE: PWM output signal maximum supported frequency 324Hz comes from
	 *       32768 / (PWM_CTRX_MIN + 1).
	 */
	if ((target_freq <= 324) && (inst->PCFSR & BIT(prs_sel))) {
		inst->PCFSR &= ~BIT(prs_sel);
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
		cxcprs = (((uint32_t) pwm_clk_src) / (ctr + 1) / target_freq) - 1;
		if (cxcprs >= 0) {
			actual_freq = ((uint32_t) pwm_clk_src) / (ctr + 1) / (cxcprs + 1);
			if (abs(actual_freq - target_freq) < deviation)
				break;
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
	const struct pwm_it8xxx2_cfg *config = DRV_CONFIG(dev);
	struct pwm_it8xxx2_regs *const inst = DRV_REG(dev);
	volatile uint8_t *reg_pcssg = (uint8_t *)config->reg_pcssg;
	int ch = config->channel;
	int prs_sel = config->prs_sel;
	int pcssg_shift;
	int pcssg_mask;

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
	pinmux_pin_set(config->pinctrls, config->pin, config->alt_fun);

	return 0;
}

static const struct pwm_driver_api pwm_it8xxx2_api = {
	.pin_set = pwm_it8xxx2_pin_set,
	.get_cycles_per_sec = pwm_it8xxx2_get_cycles_per_sec,
};

/* Device Instance */
#define PWM_IT8XXX2_INIT(inst)								\
	static const struct pwm_it8xxx2_cfg pwm_it8xxx2_cfg_##inst = {			\
		.reg_dcr = DT_INST_REG_ADDR_BY_IDX(inst, 0),				\
		.reg_pcssg = DT_INST_REG_ADDR_BY_IDX(inst, 1),				\
		.reg_pcsgr = DT_INST_REG_ADDR_BY_IDX(inst, 2),				\
		.reg_pwmpol = DT_INST_REG_ADDR_BY_IDX(inst, 3),				\
		.channel = DT_PROP(DT_INST(inst, ite_it8xxx2_pwm), channel),		\
		.base = DT_REG_ADDR(DT_NODELABEL(prs)),					\
		.prs_sel = DT_PROP(DT_INST(inst, ite_it8xxx2_pwm), prescaler_cx),	\
		.pinctrls = DEV_PINMUX(inst),						\
		.pin = DEV_PIN(inst),							\
		.alt_fun = DEV_ALT_FUN(inst),						\
	};										\
											\
	DEVICE_DT_INST_DEFINE(inst,							\
			      &pwm_it8xxx2_init,					\
			      NULL,							\
			      NULL,							\
			      &pwm_it8xxx2_cfg_##inst,					\
			      PRE_KERNEL_1,						\
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,			\
			      &pwm_it8xxx2_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_IT8XXX2_INIT)
