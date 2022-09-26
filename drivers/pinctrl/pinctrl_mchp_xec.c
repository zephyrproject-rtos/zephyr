/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2021 Linaro Limited
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_pinctrl

#include <zephyr/drivers/pinctrl.h>
#include <soc.h>

/* Microchip XEC: each GPIO pin has two 32-bit control register.
 * The first 32-bit register contains all pin features except
 * slew rate and driver strength in the second control register.
 * We compute the register index from the beginning of the GPIO
 * control address space which is the same range of the PINCTRL
 * parent node.
 */

static void config_drive_slew(struct gpio_regs * const regs, uint32_t idx, uint32_t conf)
{
	uint32_t slew = conf & (MCHP_XEC_OSPEEDR_MASK << MCHP_XEC_OSPEEDR_POS);
	uint32_t drvstr = conf & (MCHP_XEC_ODRVSTR_MASK << MCHP_XEC_ODRVSTR_POS);
	uint32_t val = 0;
	uint32_t mask = 0;

	if (slew != MCHP_XEC_OSPEEDR_NO_CHG) {
		mask |= MCHP_GPIO_CTRL2_SLEW_MASK;
		if (slew == MCHP_XEC_OSPEEDR_FAST) {
			val |= MCHP_GPIO_CTRL2_SLEW_FAST;
		}
	}
	if (drvstr != MCHP_XEC_ODRVSTR_NO_CHG) {
		mask |= MCHP_GPIO_CTRL2_DRV_STR_MASK;
		val |= (drvstr << MCHP_GPIO_CTRL2_DRV_STR_POS);
	}

	if (!mask) {
		return;
	}

	regs->CTRL2[idx] = (regs->CTRL2[idx] & ~mask) | (val & mask);
}

/* Configure pin by writing GPIO Control and Control2 registers.
 * NOTE: Disable alternate output feature since the GPIO driver does.
 * While alternate output is enabled (default state of pin) HW does not
 * ignores writes to the parallel output bit for the pin. To set parallel
 * output value we must keep pin direction as input, set alternate output
 * disable, program pin value to parallel output bit, and then disable
 * alternate output mode.
 */
static int xec_config_pin(uint32_t portpin, uint32_t conf, uint32_t altf)
{
	struct gpio_regs * const regs = (struct gpio_regs * const)DT_INST_REG_ADDR(0);
	uint32_t port = MCHP_XEC_PINMUX_PORT(portpin);
	uint32_t pin = (uint32_t)MCHP_XEC_PINMUX_PIN(portpin);
	uint32_t msk = MCHP_GPIO_CTRL_AOD_MASK;
	uint32_t val = MCHP_GPIO_CTRL_AOD_DIS;
	uint32_t idx = 0u;
	uint32_t temp = 0u;

	if (port >= NUM_MCHP_GPIO_PORTS) {
		return -EINVAL;
	}

	/* MCHP XEC family is 32 pins per port */
	idx = (port * 32U) + pin;

	config_drive_slew(regs, idx, conf);

	/* default input pad enabled, buffer type push-pull, no internal pulls */
	msk |= (BIT(MCHP_GPIO_CTRL_INPAD_DIS_POS) | MCHP_GPIO_CTRL_BUFT_MASK |
			MCHP_GPIO_CTRL_PUD_MASK |
			MCHP_GPIO_CTRL_MUX_MASK);

	if (conf & BIT(MCHP_XEC_PIN_LOW_POWER_POS)) {
		msk |= MCHP_GPIO_CTRL_PWRG_MASK;
		val |= MCHP_GPIO_CTRL_PWRG_OFF;
	}

	temp = (conf & MCHP_XEC_PUPDR_MASK) >> MCHP_XEC_PUPDR_POS;
	switch (temp) {
	case MCHP_XEC_PULL_UP:
		val |= MCHP_GPIO_CTRL_PUD_PU;
		break;
	case MCHP_XEC_PULL_DOWN:
		val |= MCHP_GPIO_CTRL_PUD_PD;
		break;
	case MCHP_XEC_REPEATER:
		val |= MCHP_GPIO_CTRL_PUD_RPT;
		break;
	default:
		val |= MCHP_GPIO_CTRL_PUD_NONE;
		break;
	}

	if ((conf >> MCHP_XEC_OTYPER_POS) & MCHP_XEC_OTYPER_MASK) {
		val |= MCHP_GPIO_CTRL_BUFT_OPENDRAIN;
	}

	regs->CTRL[idx] = (regs->CTRL[idx] & ~msk) | val;

	temp = (conf >> MCHP_XEC_OVAL_POS) & MCHP_XEC_OVAL_MASK;
	if (temp) {
		if (temp == MCHP_XEC_OVAL_DRV_HIGH) {
			regs->PAROUT[port] |= BIT(pin);
		} else {
			regs->PAROUT[port] &= ~BIT(pin);
		}

		regs->CTRL[idx] |= MCHP_GPIO_CTRL_DIR_OUTPUT;
	}

	val = (uint32_t)((altf & MCHP_GPIO_CTRL_MUX_MASK0) << MCHP_GPIO_CTRL_MUX_POS);
	regs->CTRL[idx] |= val;

	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	uint32_t portpin, mux, cfg, func;
	int ret;

	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		mux = pins[i].pinmux;

		func = MCHP_XEC_PINMUX_FUNC(mux);
		if (func >= MCHP_AFMAX) {
			return -EINVAL;
		}

		cfg = pins[i].pincfg;
		portpin = MEC_XEC_PINMUX_PORT_PIN(mux);

		ret = xec_config_pin(portpin, cfg, func);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
