/*
 * Copyright (c) 2021-2025 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>
#include <bflb_pinctrl.h>
#include <bflb_glb.h>
#include <bflb_gpio.h>

/* clang-format off */

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	GLB_GPIO_Cfg_Type pincfg;
	uint8_t i;

	ARG_UNUSED(reg);

	for (i = 0U; i < pin_cnt; i++) {
		pincfg.gpioFun  = BFLB_PINMUX_GET_FUN(pins[i]);
		pincfg.gpioMode = BFLB_PINMUX_GET_MODE(pins[i]);
		pincfg.gpioPin  = BFLB_PINMUX_GET_PIN(pins[i]);
		pincfg.pullType = BFLB_PINMUX_GET_PULL_MODES(pins[i]);
		pincfg.smtCtrl  = BFLB_PINMUX_GET_SMT(pins[i]);
		pincfg.drive    = BFLB_PINMUX_GET_DRIVER_STRENGTH(pins[i]);

		if (pincfg.gpioFun == BFLB_PINMUX_FUN_INST_uart0) {
			GLB_UART_Fun_Sel(pincfg.gpioPin % 8,
					 (BFLB_PINMUX_GET_INST(pins[i]))
					  * 0x4U /* rts, cts, rx, tx */
					  + BFLB_PINMUX_GET_SIGNAL(pins[i])
					 );
		}

		GLB_GPIO_Init(&pincfg);
	}

	return 0;
}

/* clang-format on */
