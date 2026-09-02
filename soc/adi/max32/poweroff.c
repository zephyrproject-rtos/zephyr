/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/poweroff.h>
#include <wrap_max32_lp.h>

void z_sys_poweroff(void)
{
	Wrap_MXC_LP_EnterPowerDownMode();

	CODE_UNREACHABLE;
}
