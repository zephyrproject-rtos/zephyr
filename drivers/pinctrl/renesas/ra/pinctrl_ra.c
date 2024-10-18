/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <soc.h>

#define PORT_POS (8)

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	bsp_io_port_pin_t port_pin;

	R_BSP_PinAccessEnable();

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		const pinctrl_soc_pin_t *pin = &pins[i];

		port_pin = (pin->port_num << PORT_POS) | pin->pin_num;
		R_BSP_PinCfg(port_pin, pin->cfg);
	}

	R_BSP_PinAccessDisable();

	return 0;
}

int ra_pinctrl_query_config(uint32_t port, uint32_t pin, pinctrl_soc_pin_t *pincfg)
{
	if (port >= RA_PINCTRL_PORT_NUM || pin >= RA_PINCTRL_PIN_NUM) {
		return -EINVAL;
	}

	pincfg->port_num = port;
	pincfg->pin_num = pin;

	pincfg->cfg = R_PFS->PORT[port].PIN[pin].PmnPFS;
	return 0;
}
