/*
 * Copyright (c) 2019 Lexmark International, Inc.
 * Copyright (c) 2024 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <cmsis_core.h>

void z_arm_platform_init(void)
{
	/*
	 * Use normal exception vectors address range (0x0-0x1C).
	 */
	unsigned int sctlr = __get_SCTLR();

	sctlr &= ~SCTLR_V_Msk;
	__set_SCTLR(sctlr);
}
