/*
 * Copyright (c) 2022 Vaishnav Achath
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc13xx_cc26xx_pinctrl

#include <zephyr/drivers/pinctrl.h>

static int pinctrl_c13xx_cc26xx_set(uint32_t pin, uint32_t func, uint32_t mode)
{
	if (pin >= NUM_IO_MAX || func >= NUM_IO_PORTS) {
		return -EINVAL;
	}

	IOCPortConfigureSet(pin, func, mode);

	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		pinctrl_c13xx_cc26xx_set(pins[i].pin, pins[i].iofunc, pins[i].iomode);
	}

	return 0;
}
