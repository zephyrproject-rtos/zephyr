/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>

#include <Siul2_Port_Ip.h>

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	/*
	 * By invoking Siul2_Port_Ip_Init multiple times on each group of pins,
	 * some functions like Siul2_Port_Ip_GetPinConfiguration and
	 * Siul2_Port_Ip_RevertPinConfiguration cannot be used since the internal
	 * state is not preserved between calls. Nevertheless, those functions
	 * are not needed to implement Pinctrl driver, so it's safe to use it
	 * until a public API exists to init each pin individually.
	 */
	Siul2_Port_Ip_Init(pin_cnt, pins);

	return 0;
}
