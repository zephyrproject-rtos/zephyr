/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <svc/pad/pad_api.h>
#include <main/ipc.h>

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins,
			   uint8_t pin_cnt, uintptr_t reg)
{
	sc_ipc_t ipc_handle;
	int ret, i;

	ret = sc_ipc_open(&ipc_handle, DT_REG_ADDR(DT_NODELABEL(scu_mu)));
	if (ret != SC_ERR_NONE) {
		return -ENODEV;
	}

	for (i = 0; i < pin_cnt; i++) {
		/* TODO: for now, pad configuration is not supported. As such,
		 * the state of the pad is the following:
		 *	1) Normal configuration (no OD)
		 *	2) ISO off
		 *	3) Pull select and drive strength initialized by another
		 *	entity (e.g: SCFW, Linux etc...) or set to the default
		 *	values as specified in the TRM.
		 */
		ret = sc_pad_set_mux(ipc_handle, pins[i].pad, pins[i].mux, 0, 0);
		if (ret != SC_ERR_NONE) {
			return -EINVAL;
		}
	}

	return 0;
}
