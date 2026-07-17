/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/poweroff.h>
#include <zephyr/toolchain.h>

#include "power_modes.h"

void z_sys_poweroff(void)
{
#ifdef CONFIG_SOC_IMXRT7XX_POWEROFF_FDPD
	power_enter_deep_power_down(true);
#else
	power_enter_deep_power_down(false);
#endif

	CODE_UNREACHABLE;
}
