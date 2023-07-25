/*
 * Copyright (c) 2019 Lexmark International, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/arch/arm/aarch32/cortex_a_r/cmsis.h>

void z_arm_platform_init(void)
{
	/*
	 * Use normal exception vectors address range (0x0-0x1C).
	 */
	unsigned int sctlr = __get_SCTLR();

	sctlr &= ~SCTLR_V_Msk;
	__set_SCTLR(sctlr);
}
