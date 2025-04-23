/*
 * Copyright (c) 2025 ispace, inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/pinctrl.h>

#define DT_DRV_COMPAT ti_tms570_pinmux
#define DRV_REG_ADDR  DT_INST_REG_ADDR(0)

#define REG_KICK0_OFFSET		(0x38)
#define REG_KICK1_OFFSET		(0x3C)

/* value used to unlock */
#define KICK0_VALUE			(0x83E70B13)
#define KICK1_VALUE			(0x95A4F1E0)

#define REG_PINMMR_0_OFFSET		(0x110)

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	uint32_t nibble_start;
	uint32_t mask;

	ARG_UNUSED(reg);

	/* unlock */
	sys_write32(KICK0_VALUE, DRV_REG_ADDR + REG_KICK0_OFFSET);
	sys_write32(KICK1_VALUE, DRV_REG_ADDR + REG_KICK1_OFFSET);

	for (uint8_t i = 0; i < pin_cnt; i++) {
		nibble_start = (pins[i].bit / 4) * 4;
		mask = 0xff << nibble_start;
		sys_write32(mask & BIT(pins[i].bit),
			    DRV_REG_ADDR + REG_PINMMR_0_OFFSET + (pins[i].pinmmr * 4));
	}

	/* lock */
	sys_write32(0, DRV_REG_ADDR + REG_KICK0_OFFSET);
	sys_write32(0, DRV_REG_ADDR + REG_KICK1_OFFSET);

	return 0;
}
