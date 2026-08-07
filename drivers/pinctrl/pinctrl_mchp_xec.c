/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2021 Linaro Limited
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_pinctrl

#include <soc.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/mchp-xec-pinctrl.h>
#include <zephyr/sys/sys_io.h>

#define XEC_GPIO_PORT_REG_SPACE 0x80u

/*
 * Microchip XEC: each GPIO pin has two 32-bit control register.
 * The first 32-bit register contains all pin features except
 * slew rate and driver strength in the second control register.
 * We compute the register index from the beginning of the GPIO
 * control address space which is the same range of the PINCTRL
 * parent node. A zero value in the PINCTRL pinmux field means
 * do not touch.
 */
static void config_drive_slew(mm_reg_t gpio_cr1_addr, const pinctrl_soc_pin_t *p)
{
	mm_reg_t gpio_cr2_addr = gpio_cr1_addr + MEC_GPIO_CR2_OFS;
	uint32_t msk = 0, val = 0;

	if (p->slew_rate != 0) { /* touch slew rate? */
		msk |= MEC_GPIO_CR2_SLEW_MSK;
		if (p->slew_rate > 1u) {
			val |= MEC_GPIO_CR2_SLEW_SET(MEC_GPIO_CR2_SLEW_SLOW);
		} else {
			val |= MEC_GPIO_CR2_SLEW_SET(MEC_GPIO_CR2_SLEW_FAST);
		}
	}

	if (p->drive_str != 0) { /* touch drive strength? */
		msk |= MEC_GPIO_CR2_DSTR_MSK;
		val |= MEC_GPIO_CR2_DSTR_SET((uint32_t)p->drive_str - 1u);
	}

	if (msk == 0) {
		return;
	}

	val = (val & msk) | (sys_read32(gpio_cr2_addr) & ~msk);
	sys_write32(val, gpio_cr2_addr);
}

/*
 * Internal pulls feature:
 * None, weak pull-up, weak pull-down, or repeater mode (both pulls enabled).
 * We do not touch this field unless one or more of the DT booleans are set.
 * If the no-bias boolean is set then disable internal pulls.
 * If pull up and/or down is set enable the respective pull or both for what
 * MCHP calls repeater(keeper) mode.
 */
static uint32_t prog_pud(uint32_t pcr1, const pinctrl_soc_pin_t *p)
{
	if (p->no_pud != 0) {
		pcr1 &= ~(MEC_GPIO_CR1_PUD_MSK);
		pcr1 |= MEC_GPIO_CR1_PUD_NONE;
		return pcr1;
	}

	if ((p->pd != 0) || (p->pu != 0)) {
		pcr1 &= ~(MEC_GPIO_CR1_PUD_MSK);
		if (p->pu != 0) {
			pcr1 |= MEC_GPIO_CR1_PUD_PU;
		}
		if (p->pd != 0) {
			pcr1 |= MEC_GPIO_CR1_PUD_PD;
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
static int xec_config_pin(uint32_t portpin, const pinctrl_soc_pin_t *p, uint32_t altf)
{
	mm_reg_t gpio_cr1_addr = (mm_reg_t)DT_INST_REG_ADDR(0);
	uint32_t port = MCHP_XEC_PINMUX_PORT(portpin);
	uint32_t pin = (uint32_t)MCHP_XEC_PINMUX_PIN(portpin);
	uint32_t pcr1 = 0u;

	if (port >= MEC_GPIO_MAX_PORTS) {
		return -EINVAL;
	}

	/* MCHP XEC family Control 1 and 2 are 32-bit registers */
	gpio_cr1_addr += (port * XEC_GPIO_PORT_REG_SPACE) + (pin * 4u);

	config_drive_slew(gpio_cr1_addr, p);

	/* Clear alternate output disable and input pad disable */
	pcr1 = sys_read32(gpio_cr1_addr) &
		~(BIT(MEC_GPIO_CR1_OCR_POS) | BIT(MEC_GPIO_CR1_INPD_POS));
	sys_write32(pcr1, gpio_cr1_addr);
	pcr1 = sys_read32(gpio_cr1_addr);

	pcr1 = prog_pud(pcr1, p);

	/* Touch output enable. We always enable input */
	if (p->out_dis != 0) {
		pcr1 &= ~(MEC_GPIO_CR1_DIR_MSK);
		pcr1 |= MEC_GPIO_CR1_DIR_SET(MEC_GPIO_CR1_DIR_IN);
	}

	if (p->out_en != 0) {
		pcr1 &= ~(MEC_GPIO_CR1_DIR_MSK);
		pcr1 |= MEC_GPIO_CR1_DIR_SET(MEC_GPIO_CR1_DIR_OUT);
	}

	/* Touch output state? Bit can be set even if the direction is input only */
	if (p->out_lo != 0) {
		pcr1 &= ~(MEC_GPIO_CR1_ODAT_MSK);
		pcr1 |= MEC_GPIO_CR1_ODAT_SET(MEC_GPIO_CR1_ODAT_LO);
	}

	if (p->out_hi != 0) {
		pcr1 &= ~(MEC_GPIO_CR1_ODAT_MSK);
		pcr1 |= MEC_GPIO_CR1_ODAT_SET(MEC_GPIO_CR1_ODAT_HI);
	}

	/* Touch output buffer type? */
	if (p->obuf_pp != 0) {
		pcr1 &= ~(MEC_GPIO_CR1_OBUF_MSK);
		pcr1 |= MEC_GPIO_CR1_OBUF_SET(MEC_GPIO_CR1_OBUF_PP);
	}

	if (p->obuf_od != 0) {
		pcr1 &= ~(MEC_GPIO_CR1_OBUF_MSK);
		pcr1 |= MEC_GPIO_CR1_OBUF_SET(MEC_GPIO_CR1_OBUF_OD);
	}

	/* Always touch power gate */
	pcr1 &= ~(MEC_GPIO_CR1_PG_MSK);
	if (p->lp != 0) {
		pcr1 |= MEC_GPIO_CR1_PG_SET(MEC_GPIO_CR1_PG_UNPWRD);
	} else {
		pcr1 |= MEC_GPIO_CR1_PG_SET(MEC_GPIO_CR1_PG_VTR);
	}

	/* Always touch MUX (alternate function) */
	pcr1 &= ~(MEC_GPIO_CR1_MUX_MSK);
	pcr1 |= MEC_GPIO_CR1_MUX_SET((uint32_t)altf);

	/* Always touch invert of alternate function. Need another bit to avoid touching */
	if (p->finv != 0) {
		pcr1 |= BIT(MEC_GPIO_CR1_FPOL_POS);
	} else {
		pcr1 &= ~BIT(MEC_GPIO_CR1_FPOL_POS);
	}

	/* output state set in control & parallel regs */
	sys_write32(pcr1, gpio_cr1_addr);
	/* make output state in control read-only in control and read-write in parallel reg */
	sys_set_bit(gpio_cr1_addr, MEC_GPIO_CR1_OCR_POS);

	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	uint32_t portpin = 0, func = 0;
	int ret = 0;

	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		const pinctrl_soc_pin_t *p = &pins[i];

		func = MCHP_XEC_PINMUX_FUNC(p->pinmux);
		if (func >= MCHP_AFMAX) {
			return -EINVAL;
		}

		portpin = MEC_XEC_PINMUX_PORT_PIN(p->pinmux);

		ret = xec_config_pin(portpin, p, func);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
