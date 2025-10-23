/*
 * Copyright (c) 2021-2025 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/dt-bindings/pinctrl/bflb-common-pinctrl.h>

#if defined(CONFIG_SOC_SERIES_BL60X)
#include <zephyr/dt-bindings/pinctrl/bl60x-pinctrl.h>
#elif defined(CONFIG_SOC_SERIES_BL61X)
#include <zephyr/dt-bindings/pinctrl/bl61x-pinctrl.h>
#elif defined(CONFIG_SOC_SERIES_BL70X)
#include <zephyr/dt-bindings/pinctrl/bl70x-pinctrl.h>
#else
#error "Unsupported Platform"
#endif

void pinctrl_bflb_configure_uart(uint8_t pin, uint8_t func);
void pinctrl_bflb_init_pin(pinctrl_soc_pin_t pin);

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	uint8_t i;

	ARG_UNUSED(reg);

	for (i = 0U; i < pin_cnt; i++) {

		if ((BFLB_PINMUX_GET_FUN(pins[i]) & BFLB_PINMUX_FUN_MASK)
			== BFLB_PINMUX_FUN_INST_uart0) {
			pinctrl_bflb_configure_uart(BFLB_PINMUX_GET_PIN(pins[i]),
			BFLB_PINMUX_GET_SIGNAL(pins[i]) + 4 * BFLB_PINMUX_GET_INST(pins[i]));
		}

		/* gpio init*/
		pinctrl_bflb_init_pin(pins[i]);
	}

	return 0;
}
