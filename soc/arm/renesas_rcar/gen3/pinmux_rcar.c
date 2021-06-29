/*
 * Copyright (c) 2021 IoT.bzh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rcar_pinmux

#include <devicetree.h>
#include <soc.h>
#include <device.h>

#define PINMUX_REG_BASE  DT_INST_REG_ADDR(0)
#define PINMUX_RCAR_PMMR 0x0
#define PINMUX_RCAR_GPSR 0x100
#define PINMUX_RCAR_IPSR 0x200

/* Any write to IPSR or GPSR must be precede to a write to PMMR with
 * the inverse value.
 */
static void pinmux_rcar_write(uint32_t offs, uint32_t val)
{
	sys_write32(~val, PINMUX_REG_BASE + PINMUX_RCAR_PMMR);
	sys_write32(val, PINMUX_REG_BASE + offs);
}

/* Set the pin either in gpio or peripheral */
static void pinmux_rcar_set_gpsr(uint8_t bank, uint8_t pos, bool peripheral)
{
	uint32_t reg = sys_read32(PINMUX_REG_BASE + PINMUX_RCAR_GPSR +
				  bank * sizeof(uint32_t));

	if (peripheral) {
		reg |= BIT(pos);
	} else {
		reg &= ~BIT(pos);
	}
	pinmux_rcar_write(PINMUX_RCAR_GPSR + bank * sizeof(uint32_t), reg);
}

/* Set peripheral function */
static void pinmux_rcar_set_ipsr(uint8_t bank, uint8_t shift, uint8_t val)
{
	uint32_t reg = sys_read32(PINMUX_REG_BASE + PINMUX_RCAR_IPSR +
				  bank * sizeof(uint32_t));

	reg &= ~(0xF << shift);
	reg |= (val << shift);
	pinmux_rcar_write(PINMUX_RCAR_IPSR + bank * sizeof(uint32_t), reg);
}

void pinmux_rcar_set_pingroup(const struct rcar_pin *pins, size_t num_pins)
{
	size_t i;

	for (i = 0; i < num_pins; i++, pins++) {
		pinmux_rcar_set_gpsr(pins->gpsr_bank, pins->gpsr_num,
				     pins->gpsr_val);
		pinmux_rcar_set_ipsr(pins->ipsr_bank, pins->ipsr_shift,
				     pins->ipsr_val);
	}
}
