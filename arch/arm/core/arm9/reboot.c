/*
 * Copyright (c) 2026 Takayuki Goto
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARMv5 system reset
 *
 * ARMv5 has no architectural reset mechanism, so a warm reset is emulated by
 * turning the MMU and the caches off and branching back to the reset vector.
 * SoCs with a real reset controller should override sys_arch_reboot().
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/cache.h>
#include <zephyr/arch/arm/arm9/lib_helpers.h>

extern void z_arm_reset(void);

void __weak sys_arch_reboot(int type)
{
	ARG_UNUSED(type);

	__disable_irq();

	(void)sys_cache_data_flush_and_invd_all();
	__set_SCTLR(__get_SCTLR() & ~(SCTLR_M_BIT | SCTLR_C_BIT | SCTLR_I_BIT));
	__ISB();

	z_arm_reset();

	CODE_UNREACHABLE;
}
