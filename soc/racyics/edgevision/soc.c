/*
 * SPDX-FileCopyrightText: Copyright 2026 sensry.io
 * SPDX-License-Identifier: Apache-2.0
 */

#include "soc.h"

#include <zephyr/drivers/pinctrl.h>

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(pins);
	ARG_UNUSED(pin_cnt);
	ARG_UNUSED(reg);

	return 0;
}
