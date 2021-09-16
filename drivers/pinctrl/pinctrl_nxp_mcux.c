/*
 * Copyright (c) 2021 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/pinctrl.h>

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	for (uint8_t i = 0U; i < pin_cnt; i++) {
		pins[i].port_reg->PCR[pins[i].pin] = pins[i].mux;
	}

	return 0;
}
