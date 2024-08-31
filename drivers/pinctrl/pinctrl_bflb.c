/*
 * Copyright (c) 2021-2024 Gerson Fernando Budke <nandojve@gmail.com>
 * Copyright (c) 2024 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>

/* clang-format off */

#include <glb_reg.h>
#if defined(CONFIG_SOC_SERIES_BL60X)
#  include <zephyr/dt-bindings/pinctrl/bl60x-pinctrl.h>
#else
#  error Pinctrl definitions does not exists for this platform.
#endif

static void pinctrl_blfb_configure_uart(uint8_t pin, uint8_t func)
{
	uint32_t regval;
	uint8_t sig;
	uint8_t sig_pos;

	regval = sys_read32(GLB_BASE + GLB_UART_SIG_SEL_0_OFFSET);

	sig     = pin % 8;
	sig_pos = sig << 2;

	regval &= (~(0x0f << sig_pos));
	regval |= (func << sig_pos);

	for (uint8_t i = 0; i < 8; i++) {
		/* reset other sigs which are the same with uart_func */
		sig_pos = i << 2;
		if (((regval & (0x0f << sig_pos)) == (func << sig_pos))
		&&  (i != sig) && (func != 0x0f)) {
			regval &= (~(0x0f << sig_pos));
			regval |= (0x0f << sig_pos);
		}
	}

	sys_write32(regval, GLB_BASE + GLB_UART_SIG_SEL_0_OFFSET);
}

static void pinctrl_bflb_init_pin(pinctrl_soc_pin_t pin)
{
	uint8_t drive;
	uint8_t function;
	uint16_t mode;
	uint32_t regval;
	uint8_t real_pin;
	uint8_t is_odd = 0;
	uint32_t cfg = 0;
	uint32_t cfg_address;

	real_pin = BFLB_PINMUX_GET_PIN(pin);
	function = BFLB_PINMUX_GET_FUN(pin);
	mode = BFLB_PINMUX_GET_MODE(pin);
	drive = BFLB_PINMUX_GET_DRIVER_STRENGTH(pin);

	/* Disable output anyway */
	regval = sys_read32(GLB_BASE + GLB_GPIO_CFGCTL34_OFFSET + ((real_pin >> 5) << 2));
	regval &= ~(1 << (pin & 0x1f));
	sys_write32(regval, GLB_BASE + GLB_GPIO_CFGCTL34_OFFSET + ((real_pin >> 5) << 2));

	is_odd = real_pin & 1;

	cfg_address = GLB_BASE + GLB_GPIO_CFGCTL0_OFFSET + (real_pin / 2 * 4);
	cfg = sys_read32(cfg_address);
	cfg &= ~(0xffff << (16 * is_odd));

	regval = sys_read32(GLB_BASE + GLB_GPIO_CFGCTL34_OFFSET + ((real_pin >> 5) << 2));

	if (mode == BFLB_PINMUX_MODE_analog) {
		regval &= ~(1 << (real_pin & 0x1f));
		function = 10;
	} else if (mode == BFLB_PINMUX_MODE_periph) {
		cfg |= (1 << (is_odd * 16 + 0));
		regval &= ~(1 << (real_pin & 0x1f));
	} else {
		function = 11;

		if (mode == BFLB_PINMUX_MODE_input) {
			cfg |= (1 << (is_odd * 16 + 0));
		}

		if (mode == BFLB_PINMUX_MODE_output) {
			regval |= (1 << (real_pin & 0x1f));
		}
	}

	sys_write32(regval, GLB_BASE + GLB_GPIO_CFGCTL34_OFFSET + ((real_pin >> 5) << 2));

	uint8_t pull_up = BFLB_PINMUX_GET_PULL_UP(pin);
	uint8_t pull_down = BFLB_PINMUX_GET_PULL_DOWN(pin);

	if (pull_up) {
		cfg |= (1 << (is_odd * 16 + 4));
	} else if (pull_down) {
		cfg |= (1 << (is_odd * 16 + 5));
	}

	if (BFLB_PINMUX_GET_SMT(pin)) {
		cfg |= (1 << (is_odd * 16 + 1));
	}

	cfg |= (drive << (is_odd * 16 + 2));
	cfg |= (function << (is_odd * 16 + 8));
	sys_write32(cfg, cfg_address);
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	uint8_t i;

	ARG_UNUSED(reg);

	for (i = 0U; i < pin_cnt; ++i) {
		if (BFLB_PINMUX_GET_FUN(pins[i]) == BFLB_PINMUX_FUN_INST_uart0) {
			pinctrl_blfb_configure_uart(BFLB_PINMUX_GET_PIN(pins[i]),
						    BFLB_PINMUX_GET_SIGNAL(pins[i]));
		}

		/* gpio init*/
		pinctrl_bflb_init_pin(pins[i]);
	}

	return 0;
}

/* clang-format on */
