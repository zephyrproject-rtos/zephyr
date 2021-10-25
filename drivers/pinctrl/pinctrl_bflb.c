/*
 * Copyright (c) 2021 Gerson Fernando Budke <nandojve@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/pinctrl.h>
#include <dt-bindings/pinctrl/bflb-pinctrl.h>
#include <bflb_glb.h>
#include <bflb_gpio.h>

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	GLB_GPIO_Cfg_Type pincfg;
	uint8_t i;

	ARG_UNUSED(reg);

	for (i = 0U; i < pin_cnt; i++) {
		pincfg.gpioFun  = BFLB_FUN_2_FUNC(pins[i].fun);
		pincfg.gpioMode = BFLB_FUN_2_MODE(pins[i].fun);
		pincfg.gpioPin  = pins[i].pin;
		pincfg.pullType = BFLB_CFG_2_GPIO_MODE(pins[i].cfg);
		pincfg.smtCtrl  = BFLB_CFG_2_GPIO_INP_SMT(pins[i].cfg);
		pincfg.drive    = BFLB_CFG_2_GPIO_DRV_STR(pins[i].cfg);

		if (pincfg.gpioFun == GPIO_FUN_UART) {
			GLB_UART_Fun_Sel(pincfg.gpioPin % 8,
					 (BFLB_FUN_2_INST(pins[i].fun))
					  * BFLB_SIG_UART_LEN
					  + (pins[i].flags & BFLB_SIG_UART_MASK)
					 );
		}

		GLB_GPIO_Init(&pincfg);
	}

	return 0;
}
