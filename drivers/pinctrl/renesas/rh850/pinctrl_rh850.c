/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <soc.h>

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);
	bsp_io_port_pin_t port_pin;

	R_BSP_PinAccessEnable();

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		const pinctrl_soc_pin_t *pin = &pins[i];

		port_pin = pin->port_id | pin->pin_num;
		R_BSP_PortWriteEnable(port_pin);
		R_BSP_PinCfg(port_pin, pin->cfg);
		R_BSP_PortWriteDisable(port_pin);
	}

	R_BSP_PinAccessDisable();

	return 0;
}
