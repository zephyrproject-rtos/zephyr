/*
 * Copyright (c) 2019 Lexmark International, Inc.
 * Copyright (c) 2024 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/platform/hooks.h>
#include <zephyr/arch/arm/cortex_a_r/lib_helpers.h>

#include <cmsis_core.h>

void soc_reset_hook(void)
{
	/*
	 * Use normal exception vectors address range (0x0-0x1C).
	 */
	unsigned int sctlr = __get_SCTLR();

	sctlr &= ~SCTLR_V_Msk;
	__set_SCTLR(sctlr);

	/* Renode starts every core: park the ones the build does not use */
	if (MPIDR_AFFLVL(GET_MPIDR(), 0) >= CONFIG_MP_MAX_NUM_CPUS) {
		while (true) {
			__WFI();
		}
	}
}
