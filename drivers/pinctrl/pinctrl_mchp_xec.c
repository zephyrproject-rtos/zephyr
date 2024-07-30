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

/*
 * Microchip XEC: each GPIO pin has two 32-bit control register.
 * The first 32-bit register contains all pin features except
 * slew rate and driver strength in the second control register.
 * We compute the register index from the beginning of the GPIO
 * control address space which is the same range of the PINCTRL
 * parent node. A zero value in the PINCTRL pinmux field means
 * do not touch.
 */
static void config_drive_slew(struct gpio_regs * const regs, uint32_t idx, uint32_t conf)
{
	uint32_t slew = (conf >> MCHP_XEC_SLEW_RATE_POS) & MCHP_XEC_SLEW_RATE_MSK0;
	uint32_t drvstr = (conf >> MCHP_XEC_DRV_STR_POS) & MCHP_XEC_DRV_STR_MSK0;
	uint32_t msk = 0, val = 0;

	if (slew) {
		msk |= MCHP_GPIO_CTRL2_SLEW_MASK;
		/* slow slew value is 0 */
		if (slew == MCHP_XEC_SLEW_RATE_FAST0) {
			val |= MCHP_GPIO_CTRL2_SLEW_FAST;
		}
	}

	if (drvstr) {
		msk |= MCHP_GPIO_CTRL2_DRV_STR_MASK;
		/* drive strength values are 0 based */
		val |= ((drvstr - 1u) << MCHP_GPIO_CTRL2_DRV_STR_POS);
	}

	if (!msk) {
		return;
	}

	regs->CTRL2[idx] = (regs->CTRL2[idx] & ~msk) | (val & msk);
}

/*
 * Internal pulls feature:
 * None, weak pull-up, weak pull-down, or repeater mode (both pulls enabled).
 * We do not touch this field unless one or more of the DT booleans are set.
 * If the no-bias boolean is set then disable internal pulls.
 * If pull up and/or down is set enable the respective pull or both for what
 * MCHP calls repeater(keeper) mode.
 */
static uint32_t prog_pud(uint32_t pcr1, uint32_t conf)
{
	if (conf & BIT(MCHP_XEC_NO_PUD_POS)) {
		pcr1 &= ~(MCHP_GPIO_CTRL_PUD_MASK);
		pcr1 |= MCHP_GPIO_CTRL_PUD_NONE;
		return pcr1;
	}

	if (conf & (BIT(MCHP_XEC_PU_POS) | BIT(MCHP_XEC_PD_POS))) {
		pcr1 &= ~(MCHP_GPIO_CTRL_PUD_MASK);
		if (conf & BIT(MCHP_XEC_PU_POS)) {
			pcr1 |= MCHP_GPIO_CTRL_PUD_PU;
		}
		if (conf & BIT(MCHP_XEC_PD_POS)) {
			pcr1 |= MCHP_GPIO_CTRL_PUD_PD;
		}
	}

	return pcr1;
}

/*
 * DT enable booleans take precedence over disable booleans.
 * We initially clear alternate output disable allowing us to set output state
 * in the control register. Hardware sets output state bit in both control and
 * parallel output register bits. Alternate output disable only controls which
 * register bit is writable by the EC. We also clear the input pad disable
 * bit because we need the input pin state and we don't know if the requested
 * alternate function is input or bi-directional.
 * Note 1: hardware allows input and output to be simultaneously enabled.
 * Note 2: hardware interrupt detection is only on the input path.
 */
static int xec_config_pin(uint32_t portpin, uint32_t conf, uint32_t altf)
{
	struct gpio_regs * const regs = (struct gpio_regs * const)DT_INST_REG_ADDR(0);
	uint32_t port = MCHP_XEC_PINMUX_PORT(portpin);
	uint32_t pin = (uint32_t)MCHP_XEC_PINMUX_PIN(portpin);
	uint32_t idx = 0u, pcr1 = 0u;

	if (port >= NUM_MCHP_GPIO_PORTS) {
		return -EINVAL;
	}

	/* MCHP XEC family is 32 pins per port */
	idx = (port * 32U) + pin;

	config_drive_slew(regs, idx, conf);

	/* Clear alternate output disable and input pad disable */
	regs->CTRL[idx] &= ~(BIT(MCHP_GPIO_CTRL_AOD_POS) | BIT(MCHP_GPIO_CTRL_INPAD_DIS_POS));
	pcr1 = regs->CTRL[idx]; /* current configuration including pin input state */
	pcr1 = regs->CTRL[idx]; /* read multiple times to allow propagation from pad */
	pcr1 = regs->CTRL[idx]; /* Is this necessary? */

	pcr1 = prog_pud(pcr1, conf);

	/* Touch output enable. We always enable input */
	if (conf & BIT(MCHP_XEC_OUT_DIS_POS)) {
		pcr1 &= ~(MCHP_GPIO_CTRL_DIR_OUTPUT);
	}
	if (conf & BIT(MCHP_XEC_OUT_EN_POS)) {
		pcr1 |= MCHP_GPIO_CTRL_DIR_OUTPUT;
	}

	/* Touch output state? Bit can be set even if the direction is input only */
	if (conf & BIT(MCHP_XEC_OUT_LO_POS)) {
		pcr1 &= ~BIT(MCHP_GPIO_CTRL_OUTVAL_POS);
	}
	if (conf & BIT(MCHP_XEC_OUT_HI_POS)) {
		pcr1 |= BIT(MCHP_GPIO_CTRL_OUTVAL_POS);
	}

	/* Touch output buffer type? */
	if (conf & BIT(MCHP_XEC_PUSH_PULL_POS)) {
		pcr1 &= ~(MCHP_GPIO_CTRL_BUFT_OPENDRAIN);
	}
	if (conf & BIT(MCHP_XEC_OPEN_DRAIN_POS)) {
		pcr1 |= MCHP_GPIO_CTRL_BUFT_OPENDRAIN;
	}

	/* Always touch power gate */
	pcr1 &= ~MCHP_GPIO_CTRL_PWRG_MASK;
	if (conf & BIT(MCHP_XEC_PIN_LOW_POWER_POS)) {
		pcr1 |= MCHP_GPIO_CTRL_PWRG_OFF;
	} else {
		pcr1 |= MCHP_GPIO_CTRL_PWRG_VTR_IO;
	}

	/* Always touch MUX (alternate function) */
	pcr1 &= ~MCHP_GPIO_CTRL_MUX_MASK;
	pcr1 |= (uint32_t)((altf & MCHP_GPIO_CTRL_MUX_MASK0) << MCHP_GPIO_CTRL_MUX_POS);

	/* Always touch invert of alternate function. Need another bit to avoid touching */
	if (conf & BIT(MCHP_XEC_FUNC_INV_POS)) {
		pcr1 |= BIT(MCHP_GPIO_CTRL_POL_POS);
	} else {
		pcr1 &= ~BIT(MCHP_GPIO_CTRL_POL_POS);
	}

	/* output state set in control & parallel regs */
	regs->CTRL[idx] = pcr1;
	/* make output state in control read-only in control and read-write in parallel reg */
	regs->CTRL[idx] = pcr1 | BIT(MCHP_GPIO_CTRL_AOD_POS);

	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	uint32_t portpin, pinmux, func;
	int ret;

	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		pinmux = pins[i];

		func = MCHP_XEC_PINMUX_FUNC(pinmux);
		if (func >= MCHP_AFMAX) {
			return -EINVAL;
		}

		portpin = MEC_XEC_PINMUX_PORT_PIN(pinmux);

		ret = xec_config_pin(portpin, pinmux, func);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
