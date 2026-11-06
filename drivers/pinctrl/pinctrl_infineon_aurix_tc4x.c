/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_aurix_tc4x_pinctrl

#include <errno.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

/* Port n registers at base + n * 0x400; pad driver configuration (TC4Dx UM v1.1, 48.4) */
#define PORT_BASE   ((mem_addr_t)DT_INST_REG_ADDR(0))
#define PORT_SIZE   DT_INST_REG_SIZE(0)
#define PORT_STRIDE 0x400
#define DRVCFG(pin) (0x304 + (pin) * 0x10)

#define DRVCFG_DIR  BIT(0)
#define DRVCFG_OD   BIT(1)
#define DRVCFG_MODE GENMASK(7, 4)
#define DRVCFG_PD   GENMASK(10, 8)
#define DRVCFG_PL   GENMASK(14, 12)

#define MODE_TRISTATE  0
#define MODE_PULL_DOWN 1
#define MODE_PULL_UP   2

static int pinctrl_configure_pin(const pinctrl_soc_pin_t *pin)
{
	uint32_t port = AURIX_PINMUX_PORT(pin->pinmux);
	mem_addr_t reg = PORT_BASE + port * PORT_STRIDE + DRVCFG(AURIX_PINMUX_PIN(pin->pinmux));
	uint32_t val;

	if ((port + 1) * PORT_STRIDE > PORT_SIZE) {
		return -EINVAL;
	}

	val = sys_read32(reg) & ~(DRVCFG_DIR | DRVCFG_OD | DRVCFG_MODE);
	if (pin->output) {
		val |= DRVCFG_DIR | FIELD_PREP(DRVCFG_MODE, AURIX_PINMUX_FUNC(pin->pinmux));
		val |= pin->open_drain ? DRVCFG_OD : 0;
	} else {
		/* PD and PL select the output driver and level; keep them 0 on inputs */
		val &= ~(DRVCFG_PD | DRVCFG_PL);
		val |= FIELD_PREP(DRVCFG_MODE, pin->pull_up     ? MODE_PULL_UP
					       : pin->pull_down ? MODE_PULL_DOWN
								: MODE_TRISTATE);
	}
	sys_write32(val, reg);

	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0; i < pin_cnt; i++) {
		int ret = pinctrl_configure_pin(&pins[i]);

		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}
