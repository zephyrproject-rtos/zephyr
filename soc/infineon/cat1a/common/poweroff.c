/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/sys/poweroff.h>

#include <soc.h>
#include <cmsis_core.h>

void z_sys_poweroff(void)
{
	pm_state_set(PM_STATE_SOFT_OFF, 0);

	/* Reached only if a peripheral never released, and this must not return. */
	for (;;) {
		__WFI();
	}
}
