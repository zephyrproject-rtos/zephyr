/*
 * Copyright (c) 2023 Chen Xingyu <hi@xingrz.me>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT hisilicon_hi3861_pinctrl

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/arch/common/sys_io.h>

#define IOMUX_BASE DT_INST_REG_ADDR(0)

#define FUNC_SEL_REG(n) (IOMUX_BASE + 0x604 + n * 4)
#define PAD_CTRL_REG(n) (IOMUX_BASE + 0x904 + n * 4)

static void pinctrl_configure_pin(const pinctrl_soc_pin_t *pin)
{
	sys_write32(pin->signal, FUNC_SEL_REG(pin->pad));
	sys_write32(pin->pad_ctrl, PAD_CTRL_REG(pin->pad));
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0; i < pin_cnt; i++) {
		pinctrl_configure_pin(&pins[i]);
	}

	return 0;
}
