/*
 * Copyright (c) 2025 Qingdao IotPi Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/pinctrl.h>

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	for (uint8_t i = 0; i < pin_cnt; i++) {
		const pinctrl_soc_pin_t *pin = pins + i;
		const uint32_t value = pin->flags | pin->pinmux.sel;
		sys_write32(value, pin->pinmux.reg);
	}
	return 0;
}
