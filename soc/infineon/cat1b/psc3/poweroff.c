/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/toolchain.h>

#include <cy_syspm.h>

void z_sys_poweroff(void)
{
	Cy_SysPm_SystemEnterHibernate();

	CODE_UNREACHABLE;
}
