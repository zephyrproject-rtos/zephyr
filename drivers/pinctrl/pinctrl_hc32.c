/*
 * Copyright (c) 2026 Liu Changjie <liucj1228@outlook.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xhsc_hc32_pinctrl

#include <errno.h>
#include <stdint.h>

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/hc32-pinctrl.h>
#include <zephyr/sys/util.h>

#include <hc32_ll.h>
#include <hc32_ll_gpio.h>

static int pinctrl_hc32_configure_pin(pinctrl_soc_pin_t pinmux)
{
	uint8_t port = HC32_PINMUX_GET_PORT(pinmux);
	uint8_t pin = HC32_PINMUX_GET_PIN(pinmux);
	uint8_t func = HC32_PINMUX_GET_FUNC(pinmux);

	if (port > HC32_PORT_I || pin > 15U) {
		return -EINVAL;
	}

	GPIO_SetFunc(port, (uint16_t)BIT(pin), func);

	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	int ret = 0;

	ARG_UNUSED(reg);

	LL_PERIPH_WE(LL_PERIPH_GPIO);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		ret = pinctrl_hc32_configure_pin(pins[i]);
		if (ret < 0) {
			break;
		}
	}

	LL_PERIPH_WP(LL_PERIPH_GPIO);

	return ret;
}
