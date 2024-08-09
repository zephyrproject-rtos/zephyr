/*
 * Copyright (c) 2023-2024 Chen Xingyu <hi@xingrz.me>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sophgo_cvi_pinctrl

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>

#define PINCTRL_BASE DT_INST_REG_ADDR(0)

#define PINCTRL_FMUX(n) (0x00 + (n) * 4)

#define FMUX_MASK BIT_MASK(3)

static void pinctrl_configure_pin(const pinctrl_soc_pin_t *pin)
{
	uint32_t regval;

	regval = sys_read32(PINCTRL_BASE + PINCTRL_FMUX(pin->fmux_idx));
	regval &= ~FMUX_MASK;
	regval |= pin->fmux_sel;
	sys_write32(regval, PINCTRL_BASE + PINCTRL_FMUX(pin->fmux_idx));
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0; i < pin_cnt; i++) {
		pinctrl_configure_pin(&pins[i]);
	}

	return 0;
}
