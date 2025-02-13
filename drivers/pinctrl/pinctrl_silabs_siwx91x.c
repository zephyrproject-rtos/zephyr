/*
 * Copyright (c) 2023 Antmicro
 * Copyright (c) 2024 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_siwx91x_pinctrl

#include <zephyr/drivers/pinctrl.h>
#include "sl_si91x_peripheral_gpio.h"

#define MODE_COUNT               16
#define HP_PERIPHERAL_ON_ULP_PIN 6

static bool pinctrl_siwx91x_valid_mode(uint8_t mode)
{
	return mode < MODE_COUNT;
}

static void pinctrl_siwx91x_set(uint8_t port, uint8_t pin, uint8_t ulppin, uint8_t mode,
				uint8_t ulpmode, uint8_t pad)
{
	if (pad == 0) {
		sl_si91x_gpio_enable_host_pad_selection((port << 4) | pin);
	} else if (pad != 0xFF && pad != 9) {
		sl_si91x_gpio_enable_pad_selection(pad);
	}

	if (port == SL_GPIO_ULP_PORT) {
		sl_si91x_gpio_enable_ulp_pad_receiver(ulppin);
	} else {
		sl_si91x_gpio_enable_pad_receiver((port << 4) | pin);
	}

	if (pinctrl_siwx91x_valid_mode(mode)) {
		GPIO->PIN_CONFIG[(port << 4) | pin].GPIO_CONFIG_REG_b.MODE = mode;
	}

	if (pinctrl_siwx91x_valid_mode(ulpmode)) {
		if (pinctrl_siwx91x_valid_mode(mode) && ulpmode != HP_PERIPHERAL_ON_ULP_PIN) {
			sl_si91x_gpio_ulp_soc_mode(ulppin, ulpmode);
			ulpmode = 0;
		}
		ULP_GPIO->PIN_CONFIG[ulppin].GPIO_CONFIG_REG_b.MODE = ulpmode;
	}
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);
	int i;

	for (i = 0; i < pin_cnt; i++) {
		pinctrl_siwx91x_set(pins[i].port, pins[i].pin, pins[i].ulppin, pins[i].mode,
				    pins[i].ulpmode, pins[i].pad);
	}

	return 0;
}
