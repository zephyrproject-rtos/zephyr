/*
 * Copyright (c) 2020-2022 IoT.bzh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/renesas_cpg_mssr.h>
#include <zephyr/dt-bindings/clock/renesas_cpg_mssr.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include "clock_control_renesas_cpg_mssr.h"

static void rcar_cpg_reset(uint32_t base_address, uint32_t reg, uint32_t bit)
{
	rcar_cpg_write(base_address, srcr[reg], BIT(bit));
	rcar_cpg_write(base_address, SRSTCLR(reg), BIT(bit));
}

void rcar_cpg_write(uint32_t base_address, uint32_t reg, uint32_t val)
{
	sys_write32(~val, base_address + CPGWPR);
	sys_write32(val, base_address + reg);
	/* Wait for at least one cycle of the RCLK clock (@ ca. 32 kHz) */
	k_sleep(K_USEC(35));
}

int rcar_cpg_mstp_clock_endisable(uint32_t base_address, uint32_t module, bool enable)
{
	uint32_t reg = module / 100;
	uint32_t bit = module % 100;
	uint32_t bitmask = BIT(bit);
	uint32_t reg_val;
	unsigned int key;

	__ASSERT((bit < 32) && reg < ARRAY_SIZE(mstpcr), "Invalid module number for cpg clock: %d",
		 module);

	key = irq_lock();

	reg_val = sys_read32(base_address + mstpcr[reg]);
	if (enable) {
		reg_val &= ~bitmask;
	} else {
		reg_val |= bitmask;
	}

	sys_write32(reg_val, base_address + mstpcr[reg]);
	if (!enable) {
		rcar_cpg_reset(base_address, reg, bit);
	}

	irq_unlock(key);

	return 0;
}
