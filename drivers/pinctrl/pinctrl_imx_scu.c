/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/firmware/imx_scu.h>
#include <svc/pad/pad_api.h>

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	const struct device *scu_dev;
	sc_ipc_t ipc_handle;
	int i, ret;

	scu_dev = DEVICE_DT_GET(DT_NODELABEL(scu));

	if (!device_is_ready(scu_dev))
		return -ENODEV;

	ipc_handle = imx_scu_get_ipc_handle(scu_dev);

	for (i = 0; i < pin_cnt; i++) {
		ret = sc_pad_set_mux(ipc_handle,
				pins[i].pad,
				pins[i].mux,
				pins[i].sw_config,
				pins[i].lp_config);
		if (ret != SC_ERR_NONE)
			return -EINVAL;

		if (pins[i].drive_strength != IMX_PINCTRL_INVALID_PIN_PROP &&
				pins[i].pull_selection != IMX_PINCTRL_INVALID_PIN_PROP) {
			ret = sc_pad_set_gp_28fdsoi(ipc_handle, pins[i].pad,
					pins[i].drive_strength, pins[i].pull_selection);
			if (ret != SC_ERR_NONE)
				return -EINVAL;
		}
	}
	return 0;
}
