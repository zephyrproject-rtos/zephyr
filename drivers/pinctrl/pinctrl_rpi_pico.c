/*
 * Copyright (c) 2021 Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>

/* pico-sdk includes */
#include <hardware/gpio.h>

static void pinctrl_configure_pin(const pinctrl_soc_pin_t *pin)
{
	gpio_init(pin->pin_num);
	gpio_set_function(pin->pin_num, pin->alt_func);
	gpio_set_pulls(pin->pin_num, pin->pullup, pin->pulldown);
	gpio_set_drive_strength(pin->pin_num, pin->drive_strength);
	gpio_set_slew_rate(pin->pin_num, (pin->slew_rate ?
				GPIO_SLEW_RATE_FAST : GPIO_SLEW_RATE_SLOW));
	gpio_set_input_hysteresis_enabled(pin->pin_num, pin->schmitt_enable);
	gpio_set_input_enabled(pin->pin_num, pin->input_enable);
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		pinctrl_configure_pin(pins++);
	}

	return 0;
}
