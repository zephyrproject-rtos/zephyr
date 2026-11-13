/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_aurix_tc3x_pinctrl

#include <errno.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

/* Port n registers at base + n * 0x100 (TC3xx UM v2.0.0, Table 482) */
#define PORT_BASE   ((mem_addr_t)DT_INST_REG_ADDR(0))
#define PORT_SIZE   DT_INST_REG_SIZE(0)
#define PORT_STRIDE 0x100
#define IOCR(pin)   (0x10 + ((pin) / 4) * 4)

/* Port control code PCx of pin x in IOCR bits 8 * (x % 4) + 7:3 (Table 485) */
#define PC_POS(pin)    (((pin) % 4) * 8 + 3)
#define PC_MASK        0x1F
#define PC_PULL_DOWN   0x01
#define PC_PULL_UP     0x02
#define PC_OPEN_DRAIN  0x08
#define PC_OUTPUT      0x10
#define PC_ALT_MAX     7

static int pinctrl_configure_pin(const pinctrl_soc_pin_t *pin)
{
	uint32_t port = AURIX_PINMUX_PORT(pin->pinmux);
	uint32_t num = AURIX_PINMUX_PIN(pin->pinmux);
	uint32_t func = AURIX_PINMUX_FUNC(pin->pinmux);
	mem_addr_t reg = PORT_BASE + port * PORT_STRIDE + IOCR(num);
	unsigned int key;
	uint32_t pc;

	if ((port + 1) * PORT_STRIDE > PORT_SIZE) {
		return -EINVAL;
	}

	if (pin->output) {
		if (func > PC_ALT_MAX) {
			return -EINVAL;
		}
		pc = PC_OUTPUT | (pin->open_drain ? PC_OPEN_DRAIN : 0) | func;
	} else {
		pc = pin->pull_up ? PC_PULL_UP : pin->pull_down ? PC_PULL_DOWN : 0;
	}

	/* IOCR holds four pins */
	key = irq_lock();
	sys_write32((sys_read32(reg) & ~(PC_MASK << PC_POS(num))) | (pc << PC_POS(num)), reg);
	irq_unlock(key);

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
