/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/reboot.h>

#include <cmsis_core.h>
#include <cy_syspm.h>

void sys_arch_reboot(int type)
{
	if (type == SYS_REBOOT_COLD) {
		Cy_SysPm_TriggerSoftReset();
	} else {
		/* Fallback: System warmboot can only be entered through deepsleep-ram */
		NVIC_SystemReset();
	}
}
