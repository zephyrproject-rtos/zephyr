/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Realtek Semiconductor Corporation, SIBG-SD7
 * Author: Lin Yu-Cheng <lin_yu_cheng@realtek.com>
 */

#define DT_DRV_COMPAT realtek_rts5912_pinctrl

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/realtek-rts5912-pinctrl.h>

#include <reg/reg_gpio.h>

static volatile GPIO_Type *pinctrl_base =
	(volatile GPIO_Type *)(DT_REG_ADDR(DT_NODELABEL(pinctrl)));

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);
	uint32_t pin, pinmux, func;

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		pinmux = (uint32_t)pins[i];
		pin = REALTEK_RTS5912_PINMUX_GET_GPIO_PIN(pinmux);
		func = REALTEK_RTS5912_GET_PURE_PINMUX(pinmux);
		pinctrl_base->GCR[pin] = func;
	}

	return 0;
}
