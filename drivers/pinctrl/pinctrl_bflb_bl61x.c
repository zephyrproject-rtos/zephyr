/*
 * Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>

#include <bflb_soc.h>
#include <glb_reg.h>
#include <hbn_reg.h>
#include <zephyr/dt-bindings/pinctrl/bflb-common-pinctrl.h>

#if defined(CONFIG_SOC_SERIES_BL61X)
#include <zephyr/dt-bindings/pinctrl/bl61x-pinctrl.h>
#else
#error "Unsupported Platform"
#endif

void pinctrl_bflb_configure_uart(uint8_t pin, uint8_t func)
{
	uint32_t regval, regval2;
	uint8_t sig, sig_pos;

	/* gpio pad check goes here */

	sig = pin % 12;

	if (sig < 8) {
		sig_pos = sig << 2;

		regval = sys_read32(GLB_BASE + GLB_UART_CFG1_OFFSET);
		regval &= (~(0x0f << sig_pos));
		regval |= (func << sig_pos);

		for (uint8_t i = 0; i < 8; i++) {
			/* reset other sigs which are the same as func */
			sig_pos = i << 2;
			if (((regval & (0x0f << sig_pos)) == (func << sig_pos))
				&& (i != sig) && (func != 0x0f)) {
				regval &= (~(0x0f << sig_pos));
				regval |= (0x0f << sig_pos);
			}
		}
		regval2 = sys_read32(GLB_BASE + GLB_UART_CFG2_OFFSET);

		for (uint8_t i = 8; i < 12; i++) {
			/* reset other sigs which are the same as func */
			sig_pos = (i - 8) << 2;
			if (((regval2 & (0x0f << sig_pos)) == (func << sig_pos))
				&& (i != sig) && (func != 0x0f)) {
				regval2 &= (~(0x0f << sig_pos));
				regval2 |= (0x0f << sig_pos);
			}
		}
		sys_write32(regval, GLB_BASE + GLB_UART_CFG1_OFFSET);
		sys_write32(regval2, GLB_BASE + GLB_UART_CFG2_OFFSET);
	} else {
		sig_pos = (sig - 8) << 2;

		regval = sys_read32(GLB_BASE + GLB_UART_CFG2_OFFSET);
		regval &= (~(0x0f << sig_pos));
		regval |= (func << sig_pos);

		for (uint8_t i = 8; i < 12; i++) {
			/* reset other sigs which are the same as func */
			sig_pos = (i - 8) << 2;
			if (((regval & (0x0f << sig_pos)) == (func << sig_pos))
				&& (i != sig) && (func != 0x0f)) {
				regval &= (~(0x0f << sig_pos));
				regval |= (0x0f << sig_pos);
			}
		}
		regval2 = sys_read32(GLB_BASE + GLB_UART_CFG1_OFFSET);

		for (uint8_t i = 0; i < 8; i++) {
			/* reset other sigs which are the same as func */
			sig_pos = i << 2;
			if (((regval2 & (0x0f << sig_pos)) == (func << sig_pos))
				&& (i != sig) && (func != 0x0f)) {
				regval2 &= (~(0x0f << sig_pos));
				regval2 |= (0x0f << sig_pos);
			}
		}
		sys_write32(regval, GLB_BASE + GLB_UART_CFG2_OFFSET);
		sys_write32(regval2, GLB_BASE + GLB_UART_CFG1_OFFSET);
	}
}

void pinctrl_bflb_init_pin(pinctrl_soc_pin_t pin)
{
	uint8_t drive;
	uint8_t function;
	uint16_t mode;
	uint8_t real_pin;
	uint32_t cfg = 0;

	real_pin = BFLB_PINMUX_GET_PIN(pin);
	function = BFLB_PINMUX_GET_FUN(pin);
	mode = BFLB_PINMUX_GET_MODE(pin);
	drive = BFLB_PINMUX_GET_DRIVER_STRENGTH(pin);

	/* gpio pad check goes here */

	/* disable RC32K muxing */
	if (real_pin == 16) {
		*(volatile uint32_t *)(HBN_BASE + HBN_PAD_CTRL_0_OFFSET)
			&= ~(1 << HBN_REG_EN_AON_CTRL_GPIO_POS);
	} else if (real_pin == 17) {
		*(volatile uint32_t *)(HBN_BASE + HBN_PAD_CTRL_0_OFFSET)
			&= ~(1 << (HBN_REG_EN_AON_CTRL_GPIO_POS + 1));
	}

	/* mask interrupt */
	cfg = GLB_REG_GPIO_0_INT_MASK_MSK;

	if (mode == BFLB_PINMUX_MODE_analog) {
		function = 10;
	} else if (mode == BFLB_PINMUX_MODE_periph) {
		cfg |= GLB_REG_GPIO_0_IE_MSK;
	} else {
		function = 11;

		if (mode == BFLB_PINMUX_MODE_input) {
			cfg |= GLB_REG_GPIO_0_IE_MSK;
		}

		if (mode == BFLB_PINMUX_MODE_output) {
			cfg |= GLB_REG_GPIO_0_OE_MSK;
		}
	}

	uint8_t pull_up = BFLB_PINMUX_GET_PULL_UP(pin);
	uint8_t pull_down = BFLB_PINMUX_GET_PULL_DOWN(pin);

	if (pull_up) {
		cfg |= GLB_REG_GPIO_0_PU_MSK;
	} else if (pull_down) {
		cfg |= GLB_REG_GPIO_0_PD_MSK;
	} else {
	}

	if (BFLB_PINMUX_GET_SMT(pin)) {
		cfg |= GLB_REG_GPIO_0_SMT_MSK;
	}

	cfg |= (drive << GLB_REG_GPIO_0_DRV_POS);
	cfg |= (function << GLB_REG_GPIO_0_FUNC_SEL_POS);

	/* output is controlled by _set and _clr and not value of _o*/
	cfg |= 0x1 << GLB_REG_GPIO_0_MODE_POS;

	sys_write32(cfg, GLB_BASE + GLB_GPIO_CFG0_OFFSET + (real_pin << 2));
}
