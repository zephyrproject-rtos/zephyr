/*
 * Copyright (c) 2025 Realtek Semiconductor Corporation, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/toolchain.h>
#include "rts5912_ulpm.h"

void z_sys_poweroff(void)
{
	if (IS_ENABLED(CONFIG_SOC_RTS5912_ULPM)) {
		rts5912_ulpm_enable();

		/* Spin and wait for ULPM */
		while (1) {
			;
		}
		CODE_UNREACHABLE;
	}
}
